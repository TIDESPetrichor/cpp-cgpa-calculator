#ifndef CGPA_H
#define CGPA_H

#include <string>

//---------------------------------------------------------------
// Singly linked list - replaces std::vector for semesters and
// courses.
//   insertion : append() / insertAt()
//   deletion  : removeAt() / clear()
//   traversal : head() then node->next
//   cleanup   : the destructor frees every node so nothing leaks,
//               and the copy constructor / assignment deep-copy
//               the list so each list owns its own nodes
//               (rule of three).
//---------------------------------------------------------------

template <typename T>
class LinkedList
{
public:
    struct Node
    {
        T      data;
        Node*  next;

        explicit Node(const T& value) : data(value), next(0) {}
    };

private:
    Node* m_head;
    Node* m_tail;
    int   m_size;

public:
    LinkedList() : m_head(0), m_tail(0), m_size(0) {}

    LinkedList(const LinkedList& other)
        : m_head(0), m_tail(0), m_size(0)
    {
        for (Node* n = other.m_head; n != 0; n = n->next)
            append(n->data);
    }

    LinkedList& operator=(const LinkedList& other)
    {
        if (this != &other)
        {
            clear();
            for (Node* n = other.m_head; n != 0; n = n->next)
                append(n->data);
        }
        return *this;
    }

    ~LinkedList()
    {
        clear();
    }

    // delete every node - O(n)
    void clear()
    {
        while (m_head != 0)
        {
            Node* victim = m_head;
            m_head = m_head->next;
            delete victim;
        }
        m_tail = 0;
        m_size = 0;
    }

    // insert at the end - O(1)
    void append(const T& value)
    {
        Node* n = new Node(value);
        if (m_tail != 0)
            m_tail->next = n;
        else
            m_head = n;
        m_tail = n;
        m_size++;
    }

    // insert at a position (0 .. size) - O(index)
    bool insertAt(int index, const T& value)
    {
        if (index < 0 || index > m_size)
            return false;
        if (index == m_size)
        {
            append(value);
            return true;
        }

        Node* n = new Node(value);
        if (index == 0)
        {
            n->next = m_head;
            m_head  = n;
        }
        else
        {
            Node* prev = m_head;
            for (int i = 0; i < index - 1; i++)
                prev = prev->next;
            n->next    = prev->next;
            prev->next = n;
        }
        m_size++;
        return true;
    }

    // delete a node - O(index)
    bool removeAt(int index)
    {
        if (index < 0 || index >= m_size)
            return false;

        Node* victim = 0;
        if (index == 0)
        {
            victim = m_head;
            m_head = m_head->next;
            if (m_head == 0)
                m_tail = 0;
        }
        else
        {
            Node* prev = m_head;
            for (int i = 0; i < index - 1; i++)
                prev = prev->next;
            victim     = prev->next;
            prev->next = victim->next;
            if (victim == m_tail)
                m_tail = prev;
        }
        delete victim;
        m_size--;
        return true;
    }

    // random access by walking the list - O(index)
    T* at(int index)
    {
        if (index < 0 || index >= m_size)
            return 0;
        Node* n = m_head;
        for (int i = 0; i < index; i++)
            n = n->next;
        return &n->data;
    }

    const T* at(int index) const
    {
        if (index < 0 || index >= m_size)
            return 0;
        const Node* n = m_head;
        for (int i = 0; i < index; i++)
            n = n->next;
        return &n->data;
    }

    Node*       head()       { return m_head; }
    const Node* head() const { return m_head; }

    int  size()  const { return m_size; }
    bool empty() const { return m_size == 0; }
};

//---------------------------------------------------------------
// Domain model for the CGPA calculator.
//   Course  - one subject: name, credit hours, letter grade
//   Semester - a named term holding a linked list of courses
//   CgpaRecord - the whole record: linked list of semesters
//---------------------------------------------------------------

struct Course
{
    std::string name;
    double      credits;   // credit hours
    std::string grade;     // letter grade, e.g. "A-"
    double      points;    // grade point on the 4.00 scale

    Course() : credits(0.0), points(0.0) {}
};

struct Semester
{
    std::string         name;
    LinkedList<Course>  courses;
};

// Standard 4.00 scale
struct GradeScale
{
    const char* letter;
    double      point;
};

extern const GradeScale GRADE_SCALE[];
extern const int        GRADE_SCALE_COUNT;

// "a-" -> "A-" ; unknown grade returns -1.0 from gradeToPoint
std::string normalizeGrade(const std::string& grade);
double      gradeToPoint(const std::string& grade);
std::string pointToLetter(double gpa);       // 3.6 -> "B" etc.

class CgpaRecord
{
public:
    LinkedList<Semester> semesters;

    //---- insertion ----
    int  addSemester(const std::string& name);              // returns index
    bool addCourse(int semIdx, const std::string& name,
                   double credits, const std::string& grade);

    //---- deletion ----
    bool removeSemester(int semIdx);
    bool removeCourse(int semIdx, int courseIdx);

    //---- editing ----
    bool renameSemester(int semIdx, const std::string& name);
    bool updateCourse(int semIdx, int courseIdx,
                      const std::string& name, double credits,
                      const std::string& grade);

    void clear();

    bool empty() const { return semesters.empty(); }
    int  count()  const { return semesters.size(); }

    // per-semester maths
    double semesterCredits(int i) const;
    double semesterPoints(int i) const;   // sum(credits x points)
    double semesterGpa(int i) const;      // points / credits

    // cumulative maths
    double totalCredits() const;
    double totalPoints() const;
    double cgpa() const;                  // totalPoints / totalCredits
};

#endif // CGPA_H
