#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <cctype>

using namespace std;

//=================================================================
//  CHECKROLL & PERSONNEL INFORMATION SYSTEM
//  SRI PERI SDN BHD (203570-H)
//
//  Data structures used:
//    Arrays   : employee master file, parallel pay arrays,
//               monthly payroll result array
//    Pointers : record search, record traversal,
//               pointer index table (menu 20)
//    Queue    : circular queue for daily attendance transactions
//    Stack    : LIFO undo buffer for the Edit List (menu 16)
//=================================================================

const int MAX_EMP   = 50;    // master array capacity
const int QUEUE_CAP = 200;   // circular queue capacity
const int STACK_CAP = 30;    // undo stack capacity

struct Employee
{
    int    empNo;
    string id;
    string name;
    string position;
    string department;
    double rate;             // hourly rate
};

struct Attendance           // queue element
{
    string empId;
    string date;             // dd/mm/yyyy
    double hours;
    double ot;
};

struct PayRecord            // result of "Calculate Net Pay"
{
    int    idx;              // index into employees[]
    double hours, ot, gross, epf, socso, net;
};

//---- master file (array of structs) ----
Employee employees[MAX_EMP];
int empCount = 0;

//---- parallel arrays (indexed with employees[]) ----
double mHours[MAX_EMP];
double mOt[MAX_EMP];
double yGross[MAX_EMP];

//---- monthly payroll result array ----
PayRecord payroll[MAX_EMP];
int payrollCount = 0;

//---- pointer index table (menu 20) ----
Employee* empIndex[MAX_EMP];
int indexCount = 0;

//---- undo stack (LIFO) ----
Employee undoStack[STACK_CAP];
int undoTop = 0;

//---- circular queue ----
Attendance qBuf[QUEUE_CAP];
int qFront = 0;
int qRear  = -1;
int qCount = 0;

//=================================================================
//  CIRCULAR QUEUE OPERATIONS
//=================================================================
bool enqueue(const Attendance& a)
{
    if (qCount >= QUEUE_CAP)
        return false;
    qRear = (qRear + 1) % QUEUE_CAP;
    qBuf[qRear] = a;
    qCount++;
    return true;
}

bool dequeue(Attendance& out)
{
    if (qCount <= 0)
        return false;
    out = qBuf[qFront];
    qFront = (qFront + 1) % QUEUE_CAP;
    qCount--;
    return true;
}

//=================================================================
//  STRING / LAYOUT HELPERS
//=================================================================
string pad(const string& s, int w)
{
    string r = s;
    if ((int)r.size() > w)
        r = r.substr(0, w);
    while ((int)r.size() < w)
        r += ' ';
    return r;
}

string rpad(const string& s, int w)
{
    string r = s;
    if ((int)r.size() > w)
        r = r.substr(0, w);
    while ((int)r.size() < w)
        r = ' ' + r;
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
        r[i] = toupper(r[i]);
    return r;
}

//=================================================================
//  INPUT HELPERS  (all input read line by line)
//=================================================================
string readLine(const string& prompt)
{
    cout << prompt;
    string s;
    if (!getline(cin, s))
    {
        cout << endl << "  [Input closed - program ends]" << endl;
        exit(0);
    }
    size_t a = s.find_first_not_of(" \t\r");
    if (a == string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r");
    return s.substr(a, b - a + 1);
}

bool parseInt(const string& s, int& out)
{
    istringstream iss(s);
    int v;
    char extra;
    if (iss >> v && !(iss >> extra))
    {
        out = v;
        return true;
    }
    return false;
}

bool parseDouble(const string& s, double& out)
{
    istringstream iss(s);
    double v;
    char extra;
    if (iss >> v && !(iss >> extra))
    {
        out = v;
        return true;
    }
    return false;
}

double readNumber(const string& prompt, double lo, double hi)
{
    while (true)
    {
        string s = readLine(prompt);
        double v;
        if (parseDouble(s, v) && v >= lo && v <= hi)
            return v;
        cout << "  ** Invalid entry, try again." << endl;
    }
}

int readMenu(const string& prompt)
{
    int n;
    if (parseInt(readLine(prompt), n))
        return n;
    return -1;
}

bool confirm(const string& prompt)
{
    string s = readLine(prompt);
    if (s.empty())
        return false;
    return toupper(s[0]) == 'Y';
}

void pause()
{
    readLine("\n  Press ENTER to continue...");
}

void clearScreen()
{
    system("cls");
}

//=================================================================
//  DATE / TIME
//=================================================================
string timeNow()
{
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    char b[16];
    strftime(b, sizeof(b), "%H:%M:%S", lt);
    return b;
}

string dateNow()
{
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    char b[32];
    strftime(b, sizeof(b), "%d %b %Y", lt);
    return b;
}

//=================================================================
//  SCREEN: HEADER + MAIN MENU  (DOS style panels)
//=================================================================
void drawHeader()
{
    cout << "+" << line('=', 76) << "+" << endl;
    cout << "| " << timeNow()
         << center("SRI PERI SDN BHD (203570-H)", 55)
         << dateNow() << " |" << endl;
    cout << "| "
         << center("CHECKROLL & PERSONNEL INFORMATION SYSTEM", 74)
         << " |" << endl;
    cout << "+" << line('=', 76) << "+" << endl;
}

string menuRow(const string& l, const string& r)
{
    return "| " + pad(l, 26) + " | " + pad(r, 45) + " |";
}

string panelBorder()
{
    return "+" + line('-', 28) + "+" + line('-', 47) + "+";
}

void drawMainMenu()
{
    cout << menuRow(center("MASTER", 26), center("PROCESS", 45)) << endl;
    cout << menuRow("01. Masters", "10. Calculate Net Pay") << endl;
    cout << menuRow("", "11. Month End Reset") << endl;
    cout << menuRow("", "12. Year End Reset") << endl;
    cout << panelBorder() << endl;
    cout << menuRow(center("TRANS", 26), center("UTILITY", 45)) << endl;
    cout << menuRow("07. Data Entry/Transaction", "16. Edit List") << endl;
    cout << menuRow("09. Input Forms", "17. Report - Monthly [ PAYREP ]") << endl;
    cout << menuRow("", "18. Report - Yearly") << endl;
    cout << menuRow("", "19. Report - Personnel") << endl;
    cout << menuRow("", "20. Initialise Index") << endl;
    cout << panelBorder() << endl;
}

void banner(const string& title)
{
    cout << endl;
    cout << "+" << line('-', 70) << "+" << endl;
    cout << "| " << center(title, 68) << " |" << endl;
    cout << "+" << line('-', 70) << "+" << endl;
}

//=================================================================
//  RECORD HELPERS
//=================================================================
Employee* findEmployee(const string& id)
{
    Employee* p = employees;              // pointer walk through array
    for (int i = 0; i < empCount; i++, p++)
        if (p->id == id)
            return p;
    return NULL;
}

void showEmployee(Employee* p)
{
    cout << "  ------------------------------------------------" << endl;
    cout << "  Emp No    : " << p->empNo << "     ID : " << p->id << endl;
    cout << "  Name      : " << p->name << endl;
    cout << "  Position  : " << p->position
         << "    Department : " << p->department << endl;
    cout << "  Hourly Rt : RM " << fixed << setprecision(2) << p->rate << endl;
    cout << "  ------------------------------------------------" << endl;
}

void pushUndo(const Employee& e)
{
    if (undoTop == STACK_CAP)             // stack full: drop oldest entry
    {
        for (int i = 1; i < STACK_CAP; i++)
            undoStack[i - 1] = undoStack[i];
        undoTop--;
    }
    undoStack[undoTop++] = e;
}

void addSeed(const string& id, const string& name,
             const string& pos, const string& dept, double rate)
{
    Employee& e = employees[empCount];
    e.empNo     = empCount + 1;
    e.id        = id;
    e.name      = name;
    e.position  = pos;
    e.department = dept;
    e.rate      = rate;
    empCount++;
}

void seedData()
{
    addSeed("EMP001", "AHMAD BIN ALI",  "Operator",        "PRODUCTION",   8.50);
    addSeed("EMP002", "SITI NURHALIZA", "Clerk",           "ADMIN",       12.00);
    addSeed("EMP003", "RAJU A/L MUTHU", "Forklift Driver", "WAREHOUSE",   10.00);
    addSeed("EMP004", "JOHN TAN",       "Technician",      "MAINTENANCE", 15.00);
    addSeed("EMP005", "FATIMAH ZAHRA",  "Supervisor",      "PRODUCTION",  18.00);

    // a few transactions already waiting in the queue
    Attendance a;
    a.date = dateNow();
    a.empId = "EMP001"; a.hours = 8.0; a.ot = 2.0; enqueue(a);
    a.empId = "EMP001"; a.hours = 8.0; a.ot = 0.0; enqueue(a);
    a.empId = "EMP002"; a.hours = 8.0; a.ot = 0.0; enqueue(a);
    a.empId = "EMP004"; a.hours = 8.0; a.ot = 3.0; enqueue(a);
    a.empId = "EMP005"; a.hours = 8.0; a.ot = 1.0; enqueue(a);
}

//=================================================================
//  01. MASTERS  - search / list / delete
//=================================================================
void searchEmployee()
{
    string id = toUpper(readLine("\n  Enter Employee ID : "));
    if (id.empty())
    {
        cout << "  (cancelled)" << endl;
        return;
    }
    Employee* p = findEmployee(id);
    if (p == NULL)
        cout << "  ** Employee " << id << " not found." << endl;
    else
        showEmployee(p);
}

void listEmployees()
{
    cout << endl;
    if (empCount == 0)
    {
        cout << "  Master file is empty." << endl;
        return;
    }
    cout << left << setw(4) << "NO" << setw(8) << "ID" << setw(24) << "NAME"
         << setw(17) << "POSITION" << setw(13) << "DEPARTMENT" << right
         << setw(9) << "RATE" << endl;
    cout << line('-', 75) << endl;

    Employee* p = employees;              // pointer traversal
    for (int i = 0; i < empCount; i++, p++)
        cout << left << setw(4) << (i + 1)
             << setw(8) << p->id
             << setw(24) << p->name.substr(0, 23)
             << setw(17) << p->position.substr(0, 16)
             << setw(13) << p->department.substr(0, 12)
             << right << fixed << setprecision(2) << p->rate << endl;
    cout << line('-', 75) << endl;
    cout << "  Total records : " << empCount << endl;
}

void deleteEmployee()
{
    string id = toUpper(readLine("  Enter Employee ID to delete : "));
    if (id.empty())
    {
        cout << "  (cancelled)" << endl;
        return;
    }
    Employee* p = findEmployee(id);
    if (p == NULL)
    {
        cout << "  ** Employee " << id << " not found." << endl;
        return;
    }
    showEmployee(p);
    if (!confirm("  Delete this record? (Y/N) : "))
    {
        cout << "  Delete cancelled." << endl;
        return;
    }

    int idx = (int)(p - employees);       // pointer arithmetic
    for (int i = idx; i < empCount - 1; i++)
    {
        employees[i] = employees[i + 1];
        mHours[i] = mHours[i + 1];
        mOt[i]    = mOt[i + 1];
        yGross[i] = yGross[i + 1];
    }
    empCount--;
    mHours[empCount] = 0.0;
    mOt[empCount]    = 0.0;
    yGross[empCount] = 0.0;
    payrollCount = 0;                     // force recalculation
    indexCount   = 0;                     // index is now stale
    undoTop      = 0;                     // clear undo stack
    cout << "  ** Record deleted." << endl;
}

void menuMasters()
{
    bool back = false;
    while (!back)
    {
        clearScreen();
        cout << "  +==============================" << "+" << endl;
        cout << "  |       MASTER FILE MENU       |" << endl;
        cout << "  +==============================" << "+" << endl;
        cout << "   1. Search Employee (by ID)" << endl;
        cout << "   2. List All Employees" << endl;
        cout << "   3. Delete Employee" << endl;
        cout << "   0. Return to Main Menu" << endl;

        int sel = readMenu("\n  Selection : ");
        switch (sel)
        {
            case 0: back = true; break;
            case 1: searchEmployee(); pause(); break;
            case 2: listEmployees();  pause(); break;
            case 3: deleteEmployee(); pause(); break;
            default:
                cout << "  ** Invalid option." << endl;
                pause();
        }
    }
}

//=================================================================
//  07. DATA ENTRY / TRANSACTION  - queue attendance records
//=================================================================
void menuTransaction()
{
    clearScreen();
    banner("TRANS - DATA ENTRY / TRANSACTION");
    cout << "  Daily attendance is pushed into a CIRCULAR QUEUE." << endl;
    cout << "  Queue capacity : " << QUEUE_CAP << " records" << endl;

    while (true)
    {
        if (qCount >= QUEUE_CAP)
        {
            cout << "\n  ** Queue is full." << endl;
            break;
        }
        string id = toUpper(readLine(
            "\n  Employee ID (blank = finish entry) : "));
        if (id.empty())
            break;

        Employee* p = findEmployee(id);
        if (p == NULL)
        {
            cout << "  ** Employee " << id << " not found." << endl;
            continue;
        }
        cout << "  -> " << p->name << "  (" << p->department << ")" << endl;

        string date = readLine(
            "  Date [dd/mm/yyyy] (Enter = " + dateNow() + ") : ");
        if (date.empty())
            date = dateNow();

        double hrs = readNumber("  Hours worked (0.00 - 24.00) : ", 0.0, 24.0);
        double ot  = readNumber("  Overtime hours (0.00 - 16.00) : ", 0.0, 16.0);

        Attendance a;
        a.empId = id;
        a.date  = date;
        a.hours = hrs;
        a.ot    = ot;

        if (enqueue(a))
            cout << "  ** Recorded. Pending in queue : " << qCount << endl;
        else
        {
            cout << "  ** Queue is full." << endl;
            break;
        }
    }
    cout << "\n  Queue status : " << qCount << " record(s) pending." << endl;
    pause();
}

//=================================================================
//  09. INPUT FORMS  - add new employee into the master array
//=================================================================
void menuInputForms()
{
    clearScreen();
    banner("TRANS - INPUT FORMS (NEW EMPLOYEE)");

    while (true)
    {
        if (empCount >= MAX_EMP)
        {
            cout << "\n  ** Master array is full (" << MAX_EMP
                 << " records)." << endl;
            break;
        }
        string id = toUpper(readLine(
            "\n  Employee ID (blank = cancel) : "));
        if (id.empty())
        {
            cout << "  (cancelled)" << endl;
            break;
        }
        if (findEmployee(id) != NULL)
        {
            cout << "  ** ID " << id << " already exists." << endl;
            continue;
        }
        string name = readLine("  Name       : ");
        if (name.empty())
        {
            cout << "  (cancelled)" << endl;
            break;
        }
        string pos  = readLine("  Position   : ");
        string dept = readLine("  Department : ");
        double rate = readNumber("  Hourly Rate (1.00 - 500.00) : ",
                                 1.0, 500.0);

        Employee& e = employees[empCount];
        e.empNo      = empCount + 1;
        e.id         = id;
        e.name       = name;
        e.position   = pos.empty()  ? "-" : pos;
        e.department = dept.empty() ? "-" : dept;
        e.rate       = rate;
        empCount++;
        indexCount = 0;                   // index is now stale

        cout << "  ** Employee " << id << " added.  (Total: "
             << empCount << ")" << endl;

        if (!confirm("  Add another record? (Y/N) : "))
            break;
    }
    pause();
}

//=================================================================
//  10. PROCESS - CALCULATE NET PAY  (drain queue -> payroll)
//=================================================================
void menuNetPay()
{
    clearScreen();
    banner("PROCESS - CALCULATE NET PAY");

    if (qCount == 0)
    {
        cout << "  Queue is empty - no pending transactions." << endl;
        pause();
        return;
    }

    Attendance a;
    int processed = 0, skipped = 0;
    while (dequeue(a))                    // dequeue from front
    {
        Employee* p = findEmployee(a.empId);
        if (p == NULL)
        {
            skipped++;
            continue;
        }
        int idx = (int)(p - employees);   // pointer arithmetic
        mHours[idx] += a.hours;
        mOt[idx]    += a.ot;
        processed++;
    }

    payrollCount = 0;
    double totalGross = 0.0, totalNet = 0.0;
    for (int i = 0; i < empCount; i++)
    {
        if (mHours[i] == 0.0 && mOt[i] == 0.0)
            continue;
        double rate  = employees[i].rate;
        double gross = mHours[i] * rate + mOt[i] * rate * 1.5;
        double epf   = gross * 0.11;      // employee EPF 11%
        double socso = gross * 0.005;     // SOCSO 0.5%
        double net   = gross - epf - socso;

        PayRecord& r = payroll[payrollCount++];
        r.idx   = i;
        r.hours = mHours[i];
        r.ot    = mOt[i];
        r.gross = gross;
        r.epf   = epf;
        r.socso = socso;
        r.net   = net;

        totalGross += gross;
        totalNet   += net;
    }

    cout << "\n  Transactions processed : " << processed << endl;
    if (skipped > 0)
        cout << "  Skipped (unknown ID)   : " << skipped << endl;
    cout << "  Employees on payroll   : " << payrollCount << endl;
    cout << "  Total gross            : RM "
         << fixed << setprecision(2) << totalGross << endl;
    cout << "  Total net pay          : RM " << totalNet << endl;
    cout << "\n  Queue drained. Use UTILITY 17 for the PAYREP report." << endl;
    pause();
}

//=================================================================
//  11. PROCESS - MONTH END RESET
//=================================================================
void menuMonthReset()
{
    clearScreen();
    banner("PROCESS - MONTH END RESET");

    double monthGross = 0.0;
    for (int i = 0; i < empCount; i++)
        monthGross += mHours[i] * employees[i].rate
                    + mOt[i] * employees[i].rate * 1.5;

    cout << "  Month gross to close   : RM "
         << fixed << setprecision(2) << monthGross << endl;
    cout << "  Records in queue       : " << qCount << endl;
    cout << "  Payroll records        : " << payrollCount << endl;

    if (!confirm("\n  Close the month and update year-to-date? (Y/N) : "))
    {
        cout << "  Reset cancelled." << endl;
        pause();
        return;
    }

    // fold month gross into the year array, then clear the month
    for (int i = 0; i < empCount; i++)
        yGross[i] += mHours[i] * employees[i].rate
                   + mOt[i] * employees[i].rate * 1.5;

    for (int i = 0; i < empCount; i++)
    {
        mHours[i] = 0.0;
        mOt[i]    = 0.0;
    }
    payrollCount = 0;

    Attendance a;                         // drain the queue too
    while (dequeue(a))
        ;

    cout << "  ** Month closed. Year-to-date updated." << endl;
    pause();
}

//=================================================================
//  12. PROCESS - YEAR END RESET
//=================================================================
void menuYearReset()
{
    clearScreen();
    banner("PROCESS - YEAR END RESET");

    double yearTotal = 0.0;
    for (int i = 0; i < empCount; i++)
        yearTotal += yGross[i];

    cout << "  Year-to-date gross : RM "
         << fixed << setprecision(2) << yearTotal << endl;

    if (!confirm("  Reset all year-end counters? (Y/N) : "))
    {
        cout << "  Reset cancelled." << endl;
        pause();
        return;
    }
    for (int i = 0; i < empCount; i++)
        yGross[i] = 0.0;
    cout << "  ** Year-end counters cleared." << endl;
    pause();
}

//=================================================================
//  16. UTILITY - EDIT LIST  (with undo stack)
//=================================================================
void undoEdit()
{
    if (undoTop == 0)
    {
        cout << "  ** Nothing to undo." << endl;
        return;
    }
    Employee old = undoStack[--undoTop];  // pop from stack (LIFO)
    int idx = -1;
    for (int i = 0; i < empCount; i++)
        if (employees[i].empNo == old.empNo)
        {
            idx = i;
            break;
        }
    if (idx < 0)
    {
        cout << "  ** Record no longer exists." << endl;
        return;
    }
    employees[idx] = old;
    cout << "  ** Last edit undone for " << old.id << endl;
}

void editRecord(int idx)
{
    while (true)
    {
        showEmployee(&employees[idx]);
        cout << "  1. Edit Name         2. Edit Position" << endl;
        cout << "  3. Edit Department   4. Edit Hourly Rate" << endl;
        cout << "  9. Undo last edit    0. Back" << endl;

        int sel = readMenu("  Selection : ");
        if (sel == 0)
            return;
        if (sel == 9)
        {
            undoEdit();
            continue;
        }

        string label;
        string newVal;
        double newRate = 0.0;
        bool isRate = false;

        switch (sel)
        {
            case 1: label = "New Name       : "; break;
            case 2: label = "New Position   : "; break;
            case 3: label = "New Department : "; break;
            case 4: isRate = true; break;
            default:
                cout << "  ** Invalid option." << endl;
                continue;
        }

        if (isRate)
            newRate = readNumber("  New Hourly Rate (1.00 - 500.00) : ",
                                 1.0, 500.0);
        else
        {
            newVal = readLine("  " + label);
            if (newVal.empty())
            {
                cout << "  (no change)" << endl;
                continue;
            }
        }

        pushUndo(employees[idx]);         // save old value on stack
        switch (sel)
        {
            case 1: employees[idx].name       = newVal; break;
            case 2: employees[idx].position   = newVal; break;
            case 3: employees[idx].department  = newVal; break;
            case 4: employees[idx].rate        = newRate; break;
        }
        cout << "  ** Record updated." << endl;
    }
}

void menuEditList()
{
    clearScreen();
    banner("UTILITY - EDIT LIST");

    if (empCount == 0)
    {
        cout << "  Master file is empty." << endl;
        pause();
        return;
    }
    for (int i = 0; i < empCount; i++)
        cout << "  " << setw(2) << (i + 1) << ". "
             << pad(employees[i].id, 8)
             << employees[i].name << " ("
             << employees[i].department << ")" << endl;

    double n = readNumber(
        "\n  Select employee number (0 = return) : ", 0, empCount);
    if (n == 0)
        return;
    editRecord((int)n - 1);
}

//=================================================================
//  17. UTILITY - REPORT MONTHLY  [ PAYREP ]
//=================================================================
void reportMonthly()
{
    clearScreen();
    banner("UTILITY - REPORT MONTHLY [ PAYREP ]");
    cout << "  SRI PERI SDN BHD (203570-H)          " << dateNow() << endl;
    cout << line('-', 77) << endl;

    if (payrollCount == 0)
    {
        cout << "  No payroll data - run PROCESS 10 first." << endl;
        pause();
        return;
    }

    cout << left << setw(8) << "ID" << setw(21) << "NAME"
         << right << setw(6) << "HRS" << setw(5) << "OT"
         << setw(10) << "GROSS" << setw(9) << "EPF"
         << setw(8) << "SOCSO" << setw(10) << "NET PAY" << endl;
    cout << line('-', 77) << endl;

    double tGross = 0.0, tEpf = 0.0, tSocso = 0.0, tNet = 0.0;
    for (int i = 0; i < payrollCount; i++)
    {
        PayRecord& r = payroll[i];
        Employee& e = employees[r.idx];
        cout << left << setw(8) << e.id
             << setw(21) << e.name.substr(0, 20)
             << right << fixed << setprecision(1)
             << setw(6) << r.hours << setw(5) << r.ot
             << setprecision(2)
             << setw(10) << r.gross << setw(9) << r.epf
             << setw(8) << r.socso << setw(10) << r.net << endl;
        tGross += r.gross;
        tEpf   += r.epf;
        tSocso += r.socso;
        tNet   += r.net;
    }
    cout << line('-', 77) << endl;
    cout << left << setw(40) << "TOTAL"
         << right << setprecision(2)
         << setw(10) << tGross << setw(9) << tEpf
         << setw(8) << tSocso << setw(10) << tNet << endl;
    cout << line('-', 77) << endl;
    pause();
}

//=================================================================
//  18. UTILITY - REPORT YEARLY
//=================================================================
void reportYearly()
{
    clearScreen();
    banner("UTILITY - REPORT YEARLY");
    cout << "  SRI PERI SDN BHD (203570-H)          " << dateNow() << endl;
    cout << line('-', 50) << endl;
    cout << left << setw(8) << "ID" << setw(30) << "NAME"
         << right << setw(12) << "YTD GROSS" << endl;
    cout << line('-', 50) << endl;

    double total = 0.0;
    Employee* p = employees;
    for (int i = 0; i < empCount; i++, p++)
    {
        cout << left << setw(8) << p->id
             << setw(30) << p->name.substr(0, 29)
             << right << fixed << setprecision(2)
             << setw(12) << yGross[i] << endl;
        total += yGross[i];
    }
    cout << line('-', 50) << endl;
    cout << left << setw(38) << "TOTAL"
         << right << setw(12) << total << endl;
    cout << line('-', 50) << endl;
    pause();
}

//=================================================================
//  19. UTILITY - REPORT PERSONNEL
//=================================================================
void reportPersonnel()
{
    clearScreen();
    banner("UTILITY - REPORT PERSONNEL");
    cout << "  SRI PERI SDN BHD (203570-H)          " << dateNow() << endl;
    cout << line('-', 67) << endl;
    cout << left << setw(8) << "ID" << setw(21) << "NAME"
         << setw(17) << "POSITION" << setw(13) << "DEPARTMENT"
         << right << setw(8) << "RATE" << endl;
    cout << line('-', 67) << endl;

    Employee* p = employees;              // pointer traversal
    for (int i = 0; i < empCount; i++, p++)
        cout << left << setw(8) << p->id
             << setw(21) << p->name.substr(0, 20)
             << setw(17) << p->position.substr(0, 16)
             << setw(13) << p->department.substr(0, 12)
             << right << fixed << setprecision(2)
             << setw(8) << p->rate << endl;
    cout << line('-', 67) << endl;
    cout << "  Total personnel : " << empCount << endl;
    pause();
}

//=================================================================
//  20. UTILITY - INITIALISE INDEX  (pointer index table)
//=================================================================
void menuInitIndex()
{
    clearScreen();
    banner("UTILITY - INITIALISE INDEX");

    if (empCount == 0)
    {
        cout << "  Master file is empty." << endl;
        pause();
        return;
    }

    indexCount = empCount;
    for (int i = 0; i < indexCount; i++)
        empIndex[i] = &employees[i];      // store pointers

    // selection sort of the pointer table, by ID
    for (int i = 0; i < indexCount - 1; i++)
    {
        int best = i;
        for (int j = i + 1; j < indexCount; j++)
            if (empIndex[j]->id < empIndex[best]->id)
                best = j;
        if (best != i)
        {
            Employee* t = empIndex[i];
            empIndex[i] = empIndex[best];
            empIndex[best] = t;
        }
    }

    cout << "  " << setw(4) << "NO" << setw(16) << "ADDRESS"
         << setw(9) << "ID" << setw(26) << "NAME" << endl;
    cout << line('-', 57) << endl;
    for (int i = 0; i < indexCount; i++)
    {
        ostringstream os;
        os << (const void*)empIndex[i];
        cout << "  " << setw(4) << (i + 1)
             << setw(16) << os.str()
             << setw(9) << empIndex[i]->id
             << setw(26) << empIndex[i]->name.substr(0, 25) << endl;
    }
    cout << line('-', 57) << endl;
    cout << "  Index built : " << indexCount << " pointer entries." << endl;
    pause();
}

//=================================================================
//  MAIN
//=================================================================
int main()
{
    seedData();
    bool running = true;

    while (running)
    {
        clearScreen();
        drawHeader();
        drawMainMenu();

        int sel = readMenu(
            "\n  Enter Selection ( 01-99, '00' - Quit ) : ");

        switch (sel)
        {
            case 0:  running = false;            break;
            case 1:  menuMasters();              break;
            case 7:  menuTransaction();          break;
            case 9:  menuInputForms();           break;
            case 10: menuNetPay();               break;
            case 11: menuMonthReset();           break;
            case 12: menuYearReset();            break;
            case 16: menuEditList();             break;
            case 17: reportMonthly();            break;
            case 18: reportYearly();             break;
            case 19: reportPersonnel();          break;
            case 20: menuInitIndex();            break;
            default:
                cout << "\n  ** Selection not available. Choose 00 - 20."
                     << endl;
                pause();
        }
    }

    cout << "\n  Thank you. Goodbye!" << endl;
    return 0;
}
