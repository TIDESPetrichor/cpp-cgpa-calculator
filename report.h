#ifndef REPORT_H
#define REPORT_H

#include <string>

class CgpaRecord;
struct Semester;

//---------------------------------------------------------------
// Console rendering: banners and formatted summary tables
//---------------------------------------------------------------

void showBanner(const std::string& title);
void showSemesterTable(const Semester& sem);          // per-semester
void showOverallTable(const CgpaRecord& rec);         // all semesters + CGPA
void showGradeScale();

#endif // REPORT_H
