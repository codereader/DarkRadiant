#pragma once

#include <string>

/** 
 * greebo: This class handles the Modifier definitions and translations between
 * modifier flags and the internally used modifierflags.
 */
class Modifiers
{
public:
	// Returns a string for the given modifier flags set (e.g. "SHIFT+CONTROL")
	std::string getModifierStr(const unsigned int modifierFlags, bool forMenu = false);

}; // class Modifiers
