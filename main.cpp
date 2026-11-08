//=================================================================
//  CGPA CALCULATOR - main entry point (Visually Enhanced)
//
//  CLI application that tracks courses per semester, converts
//  letter grades to the 4.00 scale, and calculates semester GPA
//  and cumulative CGPA. Semesters and courses are held in a
//  custom linked list (LinkedList<T> in cgpa.h). Data is
//  persisted to cgpa_data.json.
//=================================================================

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include "cgpa.h"
#include "storage.h"
#include "report.h"
#include "utils.h"
#include "menu.h"

using namespace std;

const string DATA_FILE = "cgpa_data.json";

//---------------------------------------------------------------
// ANSI Color Palette & Styles
//---------------------------------------------------------------
namespace Theme {
    const string RESET   = "\033[0m";
    const string BOLD    = "\033[1m";
    const string CYAN    = "\033[36m";
    const string GREEN   = "\033[32m";
    const string YELLOW  = "\033[33m";
    const string RED     = "\033[31m";
    const string GRAY    = "\033[90m";
}

//---------------------------------------------------------------
// Shared helpers
//---------------------------------------------------------------

// write the record to disk and report the outcome with colors
void saveNow(const CgpaRecord& rec)
{
    if (saveRecord(rec, DATA_FILE))
        cout << "\n  " << Theme::GREEN << "✔ Saved successfully to " << DATA_FILE << Theme::RESET << endl;
    else
        cout << "\n  " << Theme::RED << "✖ ** Warning: could not write " << DATA_FILE << Theme::RESET << endl;
}

// labels for the clickable semester menu - menuSelect numbers them
LinkedList<string> semesterLabels(const CgpaRecord& rec)
{
    LinkedList<string> labels;
    for (int i = 0; i < rec.count(); i++)
    {
        const Semester* s = rec.semesters.at(i);
        if (s == 0)
            continue;
        ostringstream os;
        os << pad(s->name, 28)
           << "(" << s->courses.size() << " course(s), GPA "
           << fixed << setprecision(2) << rec.semesterGpa(i) << ")";
        labels.append(os.str());
    }
    return labels;
}

// labels for the clickable course menu
LinkedList<string> courseLabels(const Semester& s)
{
    LinkedList<string> labels;
    for (const LinkedList<Course>::Node* n = s.courses.head();
         n != 0; n = n->next)
    {
        ostringstream os;
        os << pad(n->data.name, 30)
           << fixed << setprecision(2)
           << setw(6) << n->data.credits << " cr  "
           << setw(3) << n->data.grade;
        labels.append(os.str());
    }
    return labels;
}

// read a number, Enter keeps the current value
double readNumberKeep(const string& prompt, double current,
                      double lo, double hi)
{
    while (true)
    {
        string s = readLine(prompt);
        if (s.empty())
            return current;            // Enter = keep
        istringstream iss(s);
        double v;
        char extra;
        if (iss >> v && !(iss >> extra) && v >= lo && v <= hi)
            return v;
        cout << Theme::RED << "  ** Invalid entry, enter a number between "
             << lo << " and " << hi
             << " (Enter = keep " << current << ")." << Theme::RESET << endl;
    }
}

//---------------------------------------------------------------
// Menu actions
//---------------------------------------------------------------
void addSemesterFlow(CgpaRecord& rec)
{
    clearScreen();
    showBanner("ADD SEMESTER");

    if (rec.count() >= 20)
    {
        cout << Theme::RED << "  ** Semester limit reached (20)." << Theme::RESET << endl;
        pause();
        return;
    }

    ostringstream def;
    def << "Semester " << rec.count() + 1;
    string name = readLine("  Semester name [" + def.str() + "] : ");
    if (name.empty())
        name = def.str();

    int idx = rec.addSemester(name);
    cout << "\n  Enter courses for " << Theme::CYAN << name << Theme::RESET
         << "  (blank course name to finish)\n" << endl;

    while (true)
    {
        string cname = readLine("  Course name  : ");
        if (cname.empty())
            break;

        double cr = readNumber("  Credit hours (0.50 - 30.00) : ",
                               0.5, 30.0);

        string grade;
        double pts = -1.0;
        while (pts < 0.0)
        {
            grade = toUpper(readLine("  Grade (A, A-, B+, ... F) : "));
            pts = gradeToPoint(grade);
            if (pts < 0.0)
                cout << Theme::RED << "  ** Invalid grade. Valid : "
                     << "A  A-  B+  B  B-  C+  C  C-  D  F" << Theme::RESET << endl;
        }

        rec.addCourse(idx, cname, cr, grade);
        cout << "    -> " << Theme::GREEN << grade << Theme::RESET << " = " << fixed << setprecision(2)
             << pts << " pts  x  " << cr << " cr  =  "
             << (pts * cr) << " quality points" << endl;
    }

    if (rec.semesters.at(idx)->courses.empty())
    {
        rec.removeSemester(idx);            // no courses -> discard
        cout << "\n  " << Theme::YELLOW << "Semester discarded (no courses entered)." << Theme::RESET << endl;
        pause();
        return;
    }

    showSemesterTable(*rec.semesters.at(idx));
    saveNow(rec);
    pause();
}

void viewSemesterFlow(const CgpaRecord& rec)
{
    clearScreen();
    showBanner("VIEW SEMESTER GPA");

    if (rec.empty())
    {
        cout << "  " << Theme::YELLOW << "No semester data entered yet." << Theme::RESET << endl;
        pause();
        return;
    }
    int n = menuSelect("\n  Select semester (0 = cancel) : ",
                       semesterLabels(rec), true);
    if (n == 0)
        return;

    showSemesterTable(*rec.semesters.at(n - 1));
    pause();
}

void viewOverallFlow(const CgpaRecord& rec)
{
    clearScreen();
    showBanner("OVERALL CGPA");
    showOverallTable(rec);
    pause();
}

void gradeScaleFlow()
{
    clearScreen();
    showBanner("GRADE SCALE (4.00)");
    showGradeScale();
    pause();
}

//---------------------------------------------------------------
// Edit record - rename a semester / change a course
//---------------------------------------------------------------
void editSemesterFlow(CgpaRecord& rec)
{
    clearScreen();
    showBanner("EDIT SEMESTER NAME");

    int n = menuSelect("\n  Select semester (0 = cancel) : ",
                       semesterLabels(rec), true);
    if (n == 0)
        return;

    Semester* s = rec.semesters.at(n - 1);
    string oldName = s->name;
    cout << "\n  Current name : " << Theme::CYAN << oldName << Theme::RESET << endl;

    string name = readLine("  New name [Enter = keep] : ");
    if (name.empty())
    {
        cout << "  " << Theme::GRAY << "(no change)" << Theme::RESET << endl;
        pause();
        return;
    }

    if (rec.renameSemester(n - 1, name))
    {
        cout << "  " << Theme::GREEN << "✔ Semester renamed: \"" << oldName
             << "\" -> \"" << name << "\"." << Theme::RESET << endl;
        saveNow(rec);
    }
    else
        cout << Theme::RED << "  ** Could not rename semester." << Theme::RESET << endl;
    pause();
}

void editCourseFlow(CgpaRecord& rec)
{
    clearScreen();
    showBanner("EDIT COURSE");

    int n = menuSelect("\n  Select semester (0 = cancel) : ",
                       semesterLabels(rec), true);
    if (n == 0)
        return;

    Semester* s = rec.semesters.at(n - 1);
    if (s->courses.empty())
    {
        cout << "\n  " << Theme::YELLOW << "No courses in this semester." << Theme::RESET << endl;
        pause();
        return;
    }

    cout << "\n  " << Theme::CYAN << s->name << " - courses" << Theme::RESET << endl;

    int c = menuSelect("\n  Select course (0 = cancel) : ",
                       courseLabels(*s), true);
    if (c == 0)
        return;

    Course* course = s->courses.at(c - 1);
    cout << "\n  Current values" << endl;
    cout << "    Name    : " << course->name << endl;
    cout << "    Credits : " << fixed << setprecision(2)
         << course->credits << endl;
    cout << "    Grade   : " << course->grade << " ("
         << course->points << " pts)" << endl;

    string name = readLine("  New name [Enter = keep] : ");
    if (name.empty())
        name = course->name;

    ostringstream cp;
    cp << "  Credit hours [" << fixed << setprecision(2)
       << course->credits << "] (0.50 - 30.00, Enter = keep) : ";
    double cr = readNumberKeep(cp.str(), course->credits, 0.5, 30.0);

    string grade = course->grade;
    while (true)
    {
        string g = readLine("  Grade [" + course->grade +
                            "] (A, A-, ... F, Enter = keep) : ");
        if (g.empty())
            break;
        if (gradeToPoint(g) < 0.0)
        {
            cout << Theme::RED << "  ** Invalid grade. Valid : "
                 << "A  A-  B+  B  B-  C+  C  C-  D  F" << Theme::RESET << endl;
            continue;
        }
        grade = g;
        break;
    }

    if (rec.updateCourse(n - 1, c - 1, name, cr, grade))
    {
        cout << Theme::GREEN << "  ✔ Course updated successfully." << Theme::RESET << endl;
        showSemesterTable(*rec.semesters.at(n - 1));
        saveNow(rec);
    }
    else
        cout << Theme::RED << "  ** Could not update course." << Theme::RESET << endl;
    pause();
}

void editFlow(CgpaRecord& rec)
{
    clearScreen();
    showBanner("EDIT RECORD");

    if (rec.empty())
    {
        cout << "  " << Theme::YELLOW << "No semester data entered yet." << Theme::RESET << endl;
        pause();
        return;
    }

    cout << "  What do you want to edit?" << endl;

    LinkedList<string> items;
    items.append("Semester name");
    items.append("Course details (name / credit hours / grade)");

    int sel = menuSelect("\n  Selection : ", items, true);
    if (sel == 0)
        return;
    if (sel == 1)
        editSemesterFlow(rec);
    else
        editCourseFlow(rec);
}

//---------------------------------------------------------------
// Delete record - remove a semester / remove one course
//---------------------------------------------------------------
void deleteSemesterFlow(CgpaRecord& rec)
{
    clearScreen();
    showBanner("DELETE SEMESTER");

    int n = menuSelect("\n  Select semester to delete (0 = cancel) : ",
                       semesterLabels(rec), true);
    if (n == 0)
        return;

    Semester* s = rec.semesters.at(n - 1);
    string name    = s->name;
    int    courses = s->courses.size();

    cout << "\n  Semester : " << Theme::CYAN << name << Theme::RESET << endl;
    cout << "  Courses  : " << courses << endl;

    if (!confirm("  Delete this semester? (Y/N) : "))
    {
        cout << "  " << Theme::GRAY << "Delete cancelled." << Theme::RESET << endl;
        pause();
        return;
    }

    if (rec.removeSemester(n - 1))
    {
        cout << Theme::GREEN << "  ✔ Semester \"" << name << "\" deleted." << Theme::RESET << endl;
        saveNow(rec);
    }
    else
        cout << Theme::RED << "  ** Could not delete semester." << Theme::RESET << endl;
    pause();
}

void deleteCourseFlow(CgpaRecord& rec)
{
    clearScreen();
    showBanner("DELETE COURSE");

    int n = menuSelect("\n  Select semester (0 = cancel) : ",
                       semesterLabels(rec), true);
    if (n == 0)
        return;

    Semester* s = rec.semesters.at(n - 1);
    if (s->courses.empty())
    {
        cout << "\n  " << Theme::YELLOW << "No courses in this semester." << Theme::RESET << endl;
        pause();
        return;
    }

    cout << "\n  " << Theme::CYAN << s->name << " - courses" << Theme::RESET << endl;

    int c = menuSelect("\n  Select course to delete (0 = cancel) : ",
                       courseLabels(*s), true);
    if (c == 0)
        return;

    Course* course = s->courses.at(c - 1);
    string name = course->name;
    cout << "\n  Course : " << Theme::CYAN << name << Theme::RESET << endl;

    if (!confirm("  Delete this course? (Y/N) : "))
    {
        cout << "  " << Theme::GRAY << "Delete cancelled." << Theme::RESET << endl;
        pause();
        return;
    }

    if (rec.removeCourse(n - 1, c - 1))
    {
        cout << Theme::GREEN << "  ✔ Course \"" << name << "\" deleted." << Theme::RESET << endl;
        showSemesterTable(*rec.semesters.at(n - 1));
        saveNow(rec);
    }
    else
        cout << Theme::RED << "  ** Could not delete course." << Theme::RESET << endl;
    pause();
}

void deleteFlow(CgpaRecord& rec)
{
    clearScreen();
    showBanner("DELETE RECORD");

    if (rec.empty())
    {
        cout << "  " << Theme::YELLOW << "No semester data entered yet." << Theme::RESET << endl;
        pause();
        return;
    }

    cout << "  What do you want to delete?" << endl;

    LinkedList<string> items;
    items.append("A whole semester");
    items.append("A course inside a semester");

    int sel = menuSelect("\n  Selection : ", items, true);
    if (sel == 0)
        return;
    if (sel == 1)
        deleteSemesterFlow(rec);
    else
        deleteCourseFlow(rec);
}

void resetFlow(CgpaRecord& rec)
{
    clearScreen();
    showBanner("RESET DATA");

    if (rec.empty())
    {
        cout << "  " << Theme::YELLOW << "Nothing to reset." << Theme::RESET << endl;
        pause();
        return;
    }
    cout << "  This will delete all " << rec.count()
         << " semester(s) and " << rec.totalCredits()
         << " credit hours." << endl;

    if (!confirm("  Are you sure? (Y/N) : "))
    {
        cout << "  " << Theme::GRAY << "Reset cancelled." << Theme::RESET << endl;
        pause();
        return;
    }
    rec.clear();
    if (saveRecord(rec, DATA_FILE))
        cout << Theme::GREEN << "  ✔ All data cleared. " << DATA_FILE
             << " reset." << Theme::RESET << endl;
    else
        cout << Theme::RED << "  ** All data cleared (file write failed)." << Theme::RESET << endl;
    pause();
}

//---------------------------------------------------------------
// Main
//---------------------------------------------------------------
int main()
{
    CgpaRecord rec;

    //---- load saved data ----
    if (fileExists(DATA_FILE))
    {
        if (loadRecord(rec, DATA_FILE) && rec.count() > 0)
            cout << "  Loaded " << rec.count() << " semester(s) from "
                 << DATA_FILE << ".  CGPA = " << Theme::GREEN << fixed << setprecision(2)
                 << rec.cgpa() << Theme::RESET << endl;
        else if (rec.count() == 0)
            cout << "  " << DATA_FILE << " is empty - starting fresh."
                 << endl;
        else
            cout << Theme::RED << "  ** Warning: " << DATA_FILE
                 << " could not be read. Starting with empty data."
                 << Theme::RESET << endl;
        pause();
    }

    bool running = true;
    while (running)
    {
        clearScreen();
        showBanner("CGPA CALCULATOR");
        cout << endl;

        LinkedList<string> items;
        items.append("Add Semester");
        items.append("View Semester GPA");
        items.append("View Overall CGPA");
        items.append("Edit Record (Semester/Course)");
        items.append("Delete Record (Semester/Course)");
        items.append("Show Grade Scale");
        items.append("Reset Data");
        items.append("Exit");

        int sel = menuSelect("\n  Selection : ", items, false);
        if (sel == 0)
            continue;                    // ESC: just redraw the menu

        switch (sel)
        {
            case 1: addSemesterFlow(rec);    break;
            case 2: viewSemesterFlow(rec);   break;
            case 3: viewOverallFlow(rec);    break;
            case 4: editFlow(rec);           break;
            case 5: deleteFlow(rec);         break;
            case 6: gradeScaleFlow();        break;
            case 7: resetFlow(rec);          break;
            case 8: running = false;         break;
        }
    }

    if (saveRecord(rec, DATA_FILE))
        cout << "\n  " << Theme::GREEN << "✔ Data saved to " << DATA_FILE << Theme::RESET << endl;
    else
        cout << "\n  " << Theme::RED << "✖ ** Warning: could not write " << DATA_FILE << Theme::RESET << endl;
    cout << "  " << Theme::CYAN << "Goodbye!" << Theme::RESET << endl;
    return 0;
}
