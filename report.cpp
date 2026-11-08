#include "report.h"
#include "cgpa.h"
#include "utils.h"

#include <iostream>
#include <iomanip>

using namespace std;

void showBanner(const string& title)
{
    cout << endl;
    cout << "+" << line('=', 60) << "+" << endl;
    cout << "| " << center(title, 58) << " |" << endl;
    cout << "|" << center(dateNow(), 60) << "|" << endl;
    cout << "+" << line('=', 60) << "+" << endl;
}

//---------------------------------------------------------------
// One semester: Course Name | Credits | Grade | Points
//---------------------------------------------------------------
void showSemesterTable(const Semester& sem)
{
    cout << endl;
    cout << "  Semester : " << sem.name << endl;
    cout << "  " << line('-', 68) << endl;
    cout << "  " << left  << setw(4)  << "NO" << setw(32) << "COURSE NAME"
         << right << setw(8)  << "CREDITS"
         << setw(7)  << "GRADE"
         << setw(8)  << "POINTS"
         << setw(9)  << "TOTAL" << endl;
    cout << "  " << line('-', 68) << endl;

    double tc = 0.0, tp = 0.0;
    int no = 1;
    for (const LinkedList<Course>::Node* n = sem.courses.head();
         n != 0; n = n->next, no++)
    {
        const Course& c = n->data;
        double quality = c.credits * c.points;
        cout << "  " << left  << setw(4)  << no
             << setw(32) << pad(c.name, 31)
             << right << fixed << setprecision(2)
             << setw(8)  << c.credits
             << setw(7)  << c.grade
             << setw(8)  << c.points
             << setw(9)  << quality << endl;
        tc += c.credits;
        tp += quality;
    }
    cout << "  " << line('-', 68) << endl;
    cout << "  " << left  << setw(36) << "TOTAL"
         << right << fixed << setprecision(2)
         << setw(8)  << tc
         << setw(24) << tp << endl;
    cout << "  " << line('-', 68) << endl;

    double gpa = (tc > 0.0) ? tp / tc : 0.0;
    cout << "  GPA = " << fixed << setprecision(2) << tp
         << " / " << tc << " = " << gpa
         << "   [ grade " << pointToLetter(gpa) << " ]" << endl;
}

//---------------------------------------------------------------
// All semesters + overall CGPA
//---------------------------------------------------------------
void showOverallTable(const CgpaRecord& rec)
{
    cout << endl;
    if (rec.empty())
    {
        cout << "  No semester data entered yet." << endl;
        return;
    }

    cout << "  " << line('-', 58) << endl;
    cout << "  " << left  << setw(4)  << "NO" << setw(24) << "SEMESTER"
         << right << setw(9)  << "CREDITS"
         << setw(13) << "QUALITY PTS"
         << setw(8)  << "GPA" << endl;
    cout << "  " << line('-', 58) << endl;

    int i = 0;
    for (const LinkedList<Semester>::Node* n = rec.semesters.head();
         n != 0; n = n->next, i++)
    {
        const Semester& s = n->data;
        cout << "  " << left  << setw(4)  << (i + 1)
             << setw(24) << pad(s.name, 23)
             << right << fixed << setprecision(2)
             << setw(9)  << rec.semesterCredits(i)
             << setw(13) << rec.semesterPoints(i)
             << setw(8)  << rec.semesterGpa(i) << endl;
    }
    cout << "  " << line('-', 58) << endl;
    cout << "  " << left  << setw(28) << "OVERALL"
         << right << fixed << setprecision(2)
         << setw(9)  << rec.totalCredits()
         << setw(13) << rec.totalPoints()
         << setw(8)  << rec.cgpa() << endl;
    cout << "  " << line('-', 58) << endl;

    cout << "  CGPA = " << fixed << setprecision(2) << rec.totalPoints()
         << " / " << rec.totalCredits()
         << " = " << rec.cgpa()
         << "   [ grade " << pointToLetter(rec.cgpa()) << " ]" << endl;
}

//---------------------------------------------------------------
// Grade scale reference
//---------------------------------------------------------------
void showGradeScale()
{
    cout << endl;
    cout << "  " << line('-', 20) << endl;
    cout << "  " << left << setw(9) << "GRADE" << "GRADE POINT" << endl;
    cout << "  " << line('-', 20) << endl;
    for (int i = 0; i < GRADE_SCALE_COUNT; i++)
        cout << "  " << left << setw(9) << GRADE_SCALE[i].letter
             << fixed << setprecision(2) << GRADE_SCALE[i].point << endl;
    cout << "  " << line('-', 20) << endl;
    cout << "  GPA  = points / credits  (per semester)" << endl;
    cout << "  CGPA = all points / all credits" << endl;
}
