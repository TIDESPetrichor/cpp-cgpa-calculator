#include "storage.h"
#include "cgpa.h"

#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

//---------------------------------------------------------------
// JSON writer
//---------------------------------------------------------------
static string escapeJson(const string& s)
{
    string out;
    for (int i = 0; i < (int)s.size(); i++)
    {
        char c = s[i];
        switch (c)
        {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

bool saveRecord(const CgpaRecord& rec, const string& file)
{
    ofstream out(file.c_str(), ios::trunc);
    if (!out)
        return false;

    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"semesters\": [\n";

    // walk the semester linked list
    int i = 0;
    for (const LinkedList<Semester>::Node* sn = rec.semesters.head();
         sn != 0; sn = sn->next, i++)
    {
        const Semester& s = sn->data;
        out << "    {\n";
        out << "      \"name\": \"" << escapeJson(s.name) << "\",\n";
        out << "      \"courses\": [\n";

        // walk this semester's course linked list
        int j = 0;
        for (const LinkedList<Course>::Node* cn = s.courses.head();
             cn != 0; cn = cn->next, j++)
        {
            const Course& c = cn->data;
            out << "        { \"name\": \"" << escapeJson(c.name)
                << "\", \"credits\": " << fixed << setprecision(2)
                << c.credits
                << ", \"grade\": \"" << escapeJson(c.grade) << "\" }";
            if (j + 1 < s.courses.size())
                out << ",";
            out << "\n";
        }
        out << "      ]\n";
        out << "    }";
        if (i + 1 < rec.count())
            out << ",";
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";
    return (bool)out;
}

//---------------------------------------------------------------
// Minimal schema-aware JSON reader (no external libraries).
// Values are read in any key order; unknown keys are skipped.
//---------------------------------------------------------------
namespace {

const char* STR_ERR = "\x01";   // parse-error sentinel for strings

struct Scanner
{
    const string& s;
    size_t pos;

    Scanner(const string& src) : s(src), pos(0) {}

    void skipWs()
    {
        while (pos < s.size() &&
               (s[pos] == ' ' || s[pos] == '\t' ||
                s[pos] == '\r' || s[pos] == '\n'))
            pos++;
    }

    char peek()
    {
        skipWs();
        return pos < s.size() ? s[pos] : '\0';
    }

    bool expect(char c)
    {
        skipWs();
        if (pos < s.size() && s[pos] == c)
        {
            pos++;
            return true;
        }
        return false;
    }

    string parseString()
    {
        skipWs();
        if (pos >= s.size() || s[pos] != '"')
            return STR_ERR;
        pos++;
        string out;
        while (pos < s.size() && s[pos] != '"')
        {
            char c = s[pos++];
            if (c == '\\' && pos < s.size())
            {
                char e = s[pos++];
                switch (e)
                {
                    case 'n':  out += '\n'; break;
                    case 't':  out += '\t'; break;
                    case 'r':  out += '\r'; break;
                    default:   out += e;    break;   // \" \\ \/ \/
                }
            }
            else
                out += c;
        }
        if (pos >= s.size())
            return STR_ERR;
        pos++;                          // closing quote
        return out;
    }

    bool parseNumber(double& out)
    {
        skipWs();
        size_t start = pos;
        while (pos < s.size() &&
               (isdigit((unsigned char)s[pos]) || s[pos] == '-' ||
                s[pos] == '+' || s[pos] == '.'))
            pos++;
        if (pos == start)
            return false;
        istringstream iss(s.substr(start, pos - start));
        return (bool)(iss >> out);
    }

    // skip any value (used for unknown keys)
    bool skipValue()
    {
        char c = peek();
        if (c == '"')
            return parseString() != STR_ERR;
        if (c == '{' || c == '[')
        {
            char open  = c;
            char close = (c == '{') ? '}' : ']';
            int depth = 0;
            while (pos < s.size())
            {
                char ch = s[pos];
                if (ch == '"')
                {
                    if (parseString() == STR_ERR)
                        return false;
                    continue;
                }
                pos++;
                if (ch == open)
                    depth++;
                else if (ch == close)
                {
                    depth--;
                    if (depth == 0)
                        return true;
                }
            }
            return false;
        }
        double d;
        return parseNumber(d);
    }
};

bool parseCourse(Scanner& sc, Course& c)
{
    if (!sc.expect('{'))
        return false;
    if (sc.peek() != '}')
    {
        while (true)
        {
            string key = sc.parseString();
            if (key == STR_ERR || !sc.expect(':'))
                return false;
            if (key == "name")
            {
                c.name = sc.parseString();
                if (c.name == STR_ERR)
                    return false;
            }
            else if (key == "credits")
            {
                if (!sc.parseNumber(c.credits))
                    return false;
            }
            else if (key == "grade")
            {
                c.grade = sc.parseString();
                if (c.grade == STR_ERR)
                    return false;
            }
            else if (!sc.skipValue())
                return false;

            if (sc.peek() == ',')
            {
                sc.expect(',');
                continue;
            }
            break;
        }
    }
    if (!sc.expect('}'))
        return false;
    c.points = gradeToPoint(c.grade);    // recompute from letter
    return true;
}

bool parseSemester(Scanner& sc, Semester& sem)
{
    if (!sc.expect('{'))
        return false;
    if (sc.peek() != '}')
    {
        while (true)
        {
            string key = sc.parseString();
            if (key == STR_ERR || !sc.expect(':'))
                return false;
            if (key == "name")
            {
                sem.name = sc.parseString();
                if (sem.name == STR_ERR)
                    return false;
            }
            else if (key == "courses")
            {
                if (!sc.expect('['))
                    return false;
                if (sc.peek() != ']')
                {
                    while (true)
                    {
                        Course c;
                        if (!parseCourse(sc, c))
                            return false;
                        sem.courses.append(c);
                        if (sc.peek() == ',')
                        {
                            sc.expect(',');
                            continue;
                        }
                        break;
                    }
                }
                if (!sc.expect(']'))
                    return false;
            }
            else if (!sc.skipValue())
                return false;

            if (sc.peek() == ',')
            {
                sc.expect(',');
                continue;
            }
            break;
        }
    }
    return sc.expect('}');
}

} // namespace

bool loadRecord(CgpaRecord& rec, const string& file)
{
    ifstream in(file.c_str());
    if (!in)
        return false;
    ostringstream buf;
    buf << in.rdbuf();
    string text = buf.str();
    if (text.empty())
        return false;

    Scanner sc(text);
    rec.semesters.clear();

    if (!sc.expect('{'))
        return false;
    if (sc.peek() == '}')
        return true;                    // empty object

    while (true)
    {
        string key = sc.parseString();
        if (key == STR_ERR || !sc.expect(':'))
            return false;

        if (key == "semesters")
        {
            if (!sc.expect('['))
                return false;
            if (sc.peek() != ']')
            {
                while (true)
                {
                    Semester sem;
                    if (!parseSemester(sc, sem))
                        return false;
                    rec.semesters.append(sem);
                    if (sc.peek() == ',')
                    {
                        sc.expect(',');
                        continue;
                    }
                    break;
                }
            }
            if (!sc.expect(']'))
                return false;
        }
        else if (!sc.skipValue())
            return false;

        if (sc.peek() == ',')
        {
            sc.expect(',');
            continue;
        }
        break;
    }
    return sc.expect('}');
}

bool fileExists(const string& file)
{
    ifstream in(file.c_str());
    return (bool)in;
}
