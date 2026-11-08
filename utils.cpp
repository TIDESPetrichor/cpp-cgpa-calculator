#include "utils.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <cctype>

using namespace std;

//---------------------------------------------------------------
// Input
//---------------------------------------------------------------
string readLine(const string& prompt)
{
    cout << prompt;
    string s;
    if (!getline(cin, s))
    {
        cout << endl << "  [Input closed - program ends]" << endl;
        exit(0);
    }
    return trim(s);
}

double readNumber(const string& prompt, double lo, double hi)
{
    while (true)
    {
        string s = readLine(prompt);
        istringstream iss(s);
        double v;
        char extra;
        if (iss >> v && !(iss >> extra) && v >= lo && v <= hi)
            return v;
        cout << "  ** Invalid entry, enter a number between "
             << lo << " and " << hi << "." << endl;
    }
}

int readInt(const string& prompt, int lo, int hi)
{
    double v = readNumber(prompt, (double)lo, (double)hi);
    return (int)(v + 0.5);
}

bool confirm(const string& prompt)
{
    string s = readLine(prompt);
    if (s.empty())
        return false;
    return toupper((unsigned char)s[0]) == 'Y';
}

void pause()
{
    readLine("\n  Press ENTER to continue...");
}

void clearScreen()
{
    system("cls");
}

//---------------------------------------------------------------
// Formatting
//---------------------------------------------------------------
string pad(const string& s, int w)
{
    string r = s;
    if ((int)r.size() > w)
        r = r.substr(0, w);
    while ((int)r.size() < w)
        r += ' ';
    return r;
}

string center(const string& s, int w)
{
    if ((int)s.size() >= w)
        return s.substr(0, w);
    int left = (w - (int)s.size()) / 2;
    return string(left, ' ') + s + string(w - (int)s.size() - left, ' ');
}

string line(char c, int n)
{
    return string(n, c);
}

string toUpper(const string& s)
{
    string r = s;
    for (int i = 0; i < (int)r.size(); i++)
        r[i] = toupper((unsigned char)r[i]);
    return r;
}

string trim(const string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string dateNow()
{
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    char b[32];
    strftime(b, sizeof(b), "%d %b %Y", lt);
    return b;
}
