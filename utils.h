#ifndef UTILS_H
#define UTILS_H

#include <string>

//---------------------------------------------------------------
// Console input / formatting helpers
//---------------------------------------------------------------

std::string readLine(const std::string& prompt);   // exits on EOF
double      readNumber(const std::string& prompt,
                       double lo, double hi);      // validated
int         readInt(const std::string& prompt,
                    int lo, int hi);               // validated
bool        confirm(const std::string& prompt);    // Y/N -> bool
void        pause();                               // press ENTER
void        clearScreen();

std::string pad(const std::string& s, int w);      // left, truncate
std::string center(const std::string& s, int w);
std::string line(char c, int n);
std::string toUpper(const std::string& s);
std::string trim(const std::string& s);
std::string dateNow();

#endif // UTILS_H
