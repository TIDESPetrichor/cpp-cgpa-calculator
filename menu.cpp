#include "menu.h"
#include "utils.h"

#include <windows.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

namespace {

//---------------------------------------------------------------
// bar drawn behind the item the mouse pointer is over
//---------------------------------------------------------------
const WORD HIGHLIGHT_ATTR =
    BACKGROUND_BLUE |
    FOREGROUND_INTENSITY |
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

//---------------------------------------------------------------
// drawing
//---------------------------------------------------------------

// " 1. Add Semester"
string itemLine(int number, const string& label)
{
    ostringstream os;
    os << right << "  " << setw(2) << number << ". " << label;
    return os.str();
}

// print the whole menu (always called, so the keyboard fallback
// still shows the choices)
void drawItems(const LinkedList<string>& items, bool allowCancel)
{
    int n = 0;
    for (const LinkedList<string>::Node* p = items.head();
         p != 0; p = p->next)
    {
        n++;
        cout << itemLine(n, p->data) << endl;
    }
    if (allowCancel)
        cout << itemLine(0, "Cancel") << endl;
    cout.flush();
}

// widest line the menu prints - used to detect wrapping, which
// would break the row numbers we need for mouse hit-testing
int maxLineLength(const LinkedList<string>& items, bool allowCancel)
{
    int widest = 0;
    int n = 0;
    for (const LinkedList<string>::Node* p = items.head();
         p != 0; p = p->next)
    {
        n++;
        int len = (int)itemLine(n, p->data).size();
        if (len > widest)
            widest = len;
    }
    if (allowCancel)
    {
        int len = (int)itemLine(0, "Cancel").size();
        if (len > widest)
            widest = len;
    }
    return widest;
}

// repaint one menu line, highlighted or normal, then put the
// cursor back where the typed digits belong
void paintLine(HANDLE hOut, const LinkedList<string>& items,
               bool allowCancel, int row0, int lineIdx, bool selected,
               int width, WORD normalAttr, COORD promptPos,
               const string& typed)
{
    string label;
    int    number;

    if (allowCancel && lineIdx == items.size())
    {
        number = 0;
        label  = "Cancel";
    }
    else
    {
        const string* s = items.at(lineIdx);
        if (s == 0)
            return;
        number = lineIdx + 1;
        label  = *s;
    }

    // pad so nothing of the old line is left behind
    string text = itemLine(number, label);
    if ((int)text.size() < width)
        text += string((size_t)(width - (int)text.size()), ' ');
    else
        text = text.substr(0, (size_t)width);

    COORD pos;
    pos.X = 0;
    pos.Y = (SHORT)(row0 + lineIdx);

    DWORD written = 0;
    SetConsoleCursorPosition(hOut, pos);
    SetConsoleTextAttribute(hOut, selected ? HIGHLIGHT_ATTR : normalAttr);
    WriteConsoleA(hOut, text.c_str(), (DWORD)text.size(), &written, 0);
    SetConsoleTextAttribute(hOut, normalAttr);

    cout.flush();
    COORD back = promptPos;
    back.X = (SHORT)(promptPos.X + (int)typed.size());
    SetConsoleCursorPosition(hOut, back);
}

} // namespace

//---------------------------------------------------------------
// menuSelect
//---------------------------------------------------------------
int menuSelect(const string& prompt,
               const LinkedList<string>& items,
               bool allowCancel)
{
    const int count = items.size();
    if (count <= 0)
        return 0;

    const int lo = allowCancel ? 0 : 1;     // same range readInt had
    const int hi = count;

    HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD inMode = 0;
    CONSOLE_SCREEN_BUFFER_INFO start;

    bool usable =
        hIn  != 0 && hIn  != INVALID_HANDLE_VALUE &&
        hOut != 0 && hOut != INVALID_HANDLE_VALUE &&
        GetConsoleMode(hIn, &inMode) != 0 &&
        GetConsoleScreenBufferInfo(hOut, &start) != 0;

    int width = 80;
    if (usable)
    {
        width = start.srWindow.Right - start.srWindow.Left + 1;
        // every line must start at column 0 and fit on one row,
        // otherwise the rows cannot be hit-tested with the mouse
        usable = start.dwCursorPosition.X == 0 &&
                 maxLineLength(items, allowCancel) < width;
    }

    //---- always draw the choices ----
    drawItems(items, allowCancel);

    if (!usable)
        return readInt(prompt, lo, hi);     // no console: keyboard only

    //---- which screen row did each item land on? ----
    const int row0   = start.dwCursorPosition.Y;
    const int nLines = count + (allowCancel ? 1 : 0);

    CONSOLE_SCREEN_BUFFER_INFO after;
    bool rowsOk =
        GetConsoleScreenBufferInfo(hOut, &after) != 0 &&
        after.dwCursorPosition.X == 0 &&
        after.dwCursorPosition.Y == row0 + nLines;

    //---- switch the mouse on (quick edit would swallow clicks) ----
    DWORD oldMode = inMode;
    bool  mouseOn = false;
    if (rowsOk)
    {
        DWORD newMode = inMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
        newMode &= ~ENABLE_QUICK_EDIT_MODE;
        mouseOn = SetConsoleMode(hIn, newMode) != 0;
    }

    if (!mouseOn)
        return readInt(prompt, lo, hi);     // keyboard, no mouse

    //---- interactive: click or type a number + ENTER ----
    cout << prompt;
    cout.flush();

    CONSOLE_SCREEN_BUFFER_INFO promptInfo;
    COORD promptPos;
    promptPos.X = 0;
    promptPos.Y = 0;
    if (GetConsoleScreenBufferInfo(hOut, &promptInfo))
        promptPos = promptInfo.dwCursorPosition;

    WORD normalAttr = start.wAttributes;

    CONSOLE_CURSOR_INFO oldCursor;
    GetConsoleCursorInfo(hOut, &oldCursor);
    CONSOLE_CURSOR_INFO hidden = oldCursor;
    hidden.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &hidden);

    string typed;                           // digits typed so far
    int    hover  = -1;                     // highlighted line, -1 = none
    int    result = 0;                      // 0 = cancelled
    bool   done   = false;

    while (!done)
    {
        INPUT_RECORD rec;
        DWORD got = 0;
        if (ReadConsoleInputA(hIn, &rec, 1, &got) == 0 || got == 0)
            break;                          // input gone: treat as cancel

        if (rec.EventType == KEY_EVENT)
        {
            const KEY_EVENT_RECORD& k = rec.Event.KeyEvent;
            if (!k.bKeyDown)
                continue;

            for (WORD r = 0; r < k.wRepeatCount && !done; r++)
            {
                char ch = k.uChar.AsciiChar;

                if (k.wVirtualKeyCode == VK_RETURN)
                {
                    int v = -1;
                    if (!typed.empty())
                    {
                        istringstream iss(typed);
                        char extra;
                        if (!((iss >> v) && !(iss >> extra)))
                            v = -1;         // not a plain number
                    }
                    if (v < lo || v > hi)
                    {
                        // out of range: beep and wipe the digits so the
                        // next attempt starts clean (otherwise "9" then
                        // "1" would become "91" and never validate)
                        cout << '\a' << flush;
                        COORD p = promptPos;
                        SetConsoleCursorPosition(hOut, p);
                        cout << string(typed.size(), ' ') << flush;
                        SetConsoleCursorPosition(hOut, p);
                        typed.clear();
                    }
                    else
                    {
                        result = v;
                        done   = true;
                    }
                }
                else if (k.wVirtualKeyCode == VK_ESCAPE)
                {
                    result = 0;
                    done   = true;
                }
                else if (k.wVirtualKeyCode == VK_BACK)
                {
                    if (!typed.empty())
                    {
                        typed.erase(typed.end() - 1);
                        cout << "\b \b" << flush;
                    }
                }
                else if (ch >= '0' && ch <= '9' && typed.size() < 6)
                {
                    typed += ch;
                    cout << ch << flush;
                }
                // any other key is ignored
            }
        }
        else if (rec.EventType == MOUSE_EVENT)
        {
            const MOUSE_EVENT_RECORD& m = rec.Event.MouseEvent;
            int idx = (int)m.dwMousePosition.Y - row0;
            if (idx < 0 || idx > (allowCancel ? count : count - 1))
                idx = -1;                   // not on a menu line

            if (m.dwEventFlags == MOUSE_MOVED)
            {
                if (idx != hover)
                {
                    if (hover >= 0)
                        paintLine(hOut, items, allowCancel, row0,
                                  hover, false, width, normalAttr,
                                  promptPos, typed);
                    if (idx >= 0)
                        paintLine(hOut, items, allowCancel, row0,
                                  idx, true, width, normalAttr,
                                  promptPos, typed);
                    hover = idx;
                }
            }
            else if (m.dwEventFlags == 0 &&
                     (m.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0 &&
                     idx >= 0)
            {
                // left button pressed on an item
                result = (allowCancel && idx == count) ? 0 : idx + 1;
                done   = true;
            }
        }
        // resize / focus / menu events are ignored
    }

    //---- put the console back the way it was ----
    if (hover >= 0)
        paintLine(hOut, items, allowCancel, row0, hover,
                  false, width, normalAttr, promptPos, typed);

    SetConsoleMode(hIn, oldMode);
    SetConsoleCursorInfo(hOut, &oldCursor);
    SetConsoleTextAttribute(hOut, normalAttr);

    cout.flush();
    COORD endPos = promptPos;
    endPos.X = (SHORT)(promptPos.X + (int)typed.size());
    SetConsoleCursorPosition(hOut, endPos);
    cout << endl << flush;

    return result;
}
