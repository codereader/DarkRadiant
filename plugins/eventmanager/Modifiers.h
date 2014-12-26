#pragma once

#include <string>

class wxMouseEvent;
class wxKeyEvent;

/** 
 * greebo: This class handles the Modifier definitions and translations between
 * modifier flags and the internally used modifierflags.
 */
class Modifiers
{
	// The current modifier state
	unsigned int _modifierState;

public:
	// Constructor, loads the modifier definitions from the XMLRegistry
	Modifiers();

	unsigned int getModifierFlags(const std::string& modifierStr);

	// Returns a bit field with the according modifier flags set for the given state
	unsigned int getKeyboardFlagsFromMouseButtonState(unsigned int state);

	unsigned int getKeyboardFlags(wxMouseEvent& ev);
	unsigned int getKeyboardFlags(wxKeyEvent& ev);

	// Returns a string for the given modifier flags set (e.g. "SHIFT+CONTROL")
	std::string getModifierStr(const unsigned int modifierFlags, bool forMenu = false);

	/** greebo: Retrieves the current modifier mask
	 */
	unsigned int getState() const;

	/** greebo: Clear the state (used to reset the modifiers after a shortcut is found)
	 */
	void clearState();

	/** greebo: Updates the internal modifierstate with the given EventKey.
	 *
	 * @keyPress: TRUE, if the eventkey is related to an KeyPress event
	 * 			  FALSE, if according to a KeyRelease event.
	 */
	void updateState(wxKeyEvent& ev, bool keyPress);

}; // class Modifiers
