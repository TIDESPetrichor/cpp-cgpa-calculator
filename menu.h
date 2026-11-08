#ifndef MENU_H
#define MENU_H

#include <string>

#include "cgpa.h"          // LinkedList

//---------------------------------------------------------------
// Interactive numbered menu for the Windows console.
//
// Draws every item as "   1. <label>" (plus a "   0. Cancel" line
// when allowCancel is set) and then waits for the user to:
//   * move the mouse over an item - it is highlighted, and
//   * click an item with the left mouse button, or
//   * type a number and press ENTER (0 = cancel when allowed), or
//   * press ESC to cancel.
//
// Returns the selected number (1..count), or 0 when cancelled.
//
// When the program is not attached to a console (piped or
// redirected input, or a window too small to hit-test) the mouse
// is unavailable: the choices are still drawn and a plain
// keyboard prompt is used instead, so behaviour there is exactly
// the same as before.
//---------------------------------------------------------------

int menuSelect(const std::string& prompt,
               const LinkedList<std::string>& items,
               bool allowCancel);

#endif // MENU_H
