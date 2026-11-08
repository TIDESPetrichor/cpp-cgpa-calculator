#include "cgpa.h"
#include <cctype>

using namespace std;

//---------------------------------------------------------------
// Standard 4.00 grade scale
//---------------------------------------------------------------
const GradeScale GRADE_SCALE[] =
{
    { "A",  4.00 },
    { "A-", 3.70 },
    { "B+", 3.30 },
    { "B",  3.00 },
    { "B-", 2.70 },
    { "C+", 2.30 },
    { "C",  2.00 },
    { "C-", 1.70 },
    { "D",  1.00 },
    { "F",  0.00 }
};

const int GRADE_SCALE_COUNT =
    sizeof(GRADE_SCALE) / sizeof(GRADE_SCALE[0]);

//---------------------------------------------------------------
// Grade conversion
//---------------------------------------------------------------
string normalizeGrade(const string& grade)
{
    string r;
    for (int i = 0; i < (int)grade.size(); i++)
    {
        char c = grade[i];
        if (c == ' ' || c == '\t')
            continue;                       // allow "A -"
        r += toupper((unsigned char)c);     // allow "a-"
    }
    return r;
}

double gradeToPoint(const string& grade)
{
    string g = normalizeGrade(grade);
    for (int i = 0; i < GRADE_SCALE_COUNT; i++)
        if (g == GRADE_SCALE[i].letter)
            return GRADE_SCALE[i].point;
    return -1.0;                            // not a valid grade
}

string pointToLetter(double gpa)
{
    double g = gpa + 0.000001;   // tolerance: 3 x 3.30 is 3.2999... in binary
    if (g >= 3.95) return "A";
    if (g >= 3.70) return "A-";
    if (g >= 3.30) return "B+";
    if (g >= 3.00) return "B";
    if (g >= 2.70) return "B-";
    if (g >= 2.30) return "C+";
    if (g >= 2.00) return "C";
    if (g >= 1.70) return "C-";
    if (g >= 1.00) return "D";
    return "F";
}

//---------------------------------------------------------------
// Helpers - walk one semester's course linked list
//---------------------------------------------------------------
namespace
{

double coursesCredits(const Semester& s)
{
    double total = 0.0;
    for (const LinkedList<Course>::Node* n = s.courses.head();
         n != 0; n = n->next)
        total += n->data.credits;
    return total;
}

double coursesPoints(const Semester& s)
{
    double total = 0.0;
    for (const LinkedList<Course>::Node* n = s.courses.head();
         n != 0; n = n->next)
        total += n->data.credits * n->data.points;
    return total;
}

} // namespace

//---------------------------------------------------------------
// CgpaRecord operations - insertion
//---------------------------------------------------------------
int CgpaRecord::addSemester(const string& name)
{
    Semester s;
    s.name = name;
    semesters.append(s);
    return semesters.size() - 1;
}

bool CgpaRecord::addCourse(int semIdx, const string& name,
                           double credits, const string& grade)
{
    Semester* s = semesters.at(semIdx);
    if (s == 0)
        return false;

    Course c;
    c.name    = name;
    c.credits = credits;
    c.grade   = normalizeGrade(grade);
    c.points  = gradeToPoint(c.grade);
    s->courses.append(c);
    return true;
}

//---------------------------------------------------------------
// CgpaRecord operations - deletion
//---------------------------------------------------------------
bool CgpaRecord::removeSemester(int semIdx)
{
    return semesters.removeAt(semIdx);      // nodes freed by list
}

bool CgpaRecord::removeCourse(int semIdx, int courseIdx)
{
    Semester* s = semesters.at(semIdx);
    if (s == 0)
        return false;
    return s->courses.removeAt(courseIdx);
}

//---------------------------------------------------------------
// CgpaRecord operations - editing
//---------------------------------------------------------------
bool CgpaRecord::renameSemester(int semIdx, const string& name)
{
    Semester* s = semesters.at(semIdx);
    if (s == 0 || name.empty())
        return false;
    s->name = name;
    return true;
}

bool CgpaRecord::updateCourse(int semIdx, int courseIdx,
                              const string& name, double credits,
                              const string& grade)
{
    Semester* s = semesters.at(semIdx);
    if (s == 0)
        return false;

    Course* c = s->courses.at(courseIdx);
    if (c == 0)
        return false;

    string g   = normalizeGrade(grade);
    double pts = gradeToPoint(g);
    if (pts < 0.0)                          // unknown letter grade
        return false;

    if (!name.empty())
        c->name = name;
    c->credits = credits;
    c->grade   = g;
    c->points  = pts;
    return true;
}

void CgpaRecord::clear()
{
    semesters.clear();                      // destructor frees nodes
}

//---------------------------------------------------------------
// CgpaRecord operations - maths
//---------------------------------------------------------------
double CgpaRecord::semesterCredits(int i) const
{
    const Semester* s = semesters.at(i);
    if (s == 0)
        return 0.0;
    return coursesCredits(*s);
}

double CgpaRecord::semesterPoints(int i) const
{
    const Semester* s = semesters.at(i);
    if (s == 0)
        return 0.0;
    return coursesPoints(*s);
}

double CgpaRecord::semesterGpa(int i) const
{
    double cr = semesterCredits(i);
    if (cr <= 0.0)
        return 0.0;
    return semesterPoints(i) / cr;
}

double CgpaRecord::totalCredits() const
{
    double total = 0.0;
    for (const LinkedList<Semester>::Node* n = semesters.head();
         n != 0; n = n->next)
        total += coursesCredits(n->data);
    return total;
}

double CgpaRecord::totalPoints() const
{
    double total = 0.0;
    for (const LinkedList<Semester>::Node* n = semesters.head();
         n != 0; n = n->next)
        total += coursesPoints(n->data);
    return total;
}

double CgpaRecord::cgpa() const
{
    double cr = totalCredits();
    if (cr <= 0.0)
        return 0.0;
    return totalPoints() / cr;
}
