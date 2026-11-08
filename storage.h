#ifndef STORAGE_H
#define STORAGE_H

#include <string>

class CgpaRecord;

//---------------------------------------------------------------
// Local persistence - simple JSON file (cgpa_data.json)
//   { "version": 1,
//     "semesters": [ { "name": "...",
//                      "courses": [ { "name": "...",
//                                     "credits": 3.00,
//                                     "grade": "A" } ] } ] }
//---------------------------------------------------------------

bool saveRecord(const CgpaRecord& rec, const std::string& file);
bool loadRecord(CgpaRecord& rec, const std::string& file);
bool fileExists(const std::string& file);

#endif // STORAGE_H
