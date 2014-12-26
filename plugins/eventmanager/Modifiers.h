#pragma once

#include <string>

class wxKeyEvent;

/** 
 * greebo: This class handles the Modifier definitions and translations between
 * modifier flags and the internally used modifierflags.
 */
class Modifiers
{
public:

	// Returns a bit field with the according modifier flags set for the given state
	unsigned int getKeyboardFlagsFromMouseButtonState(unsigned int state);

	// Returns a string for the given modifier flags set (e.g. "SHIFT+CONTROL")
	std::string getModifierStr(const unsigned int modifierFlags, bool forMenu = false);

}; // class Modifiers
