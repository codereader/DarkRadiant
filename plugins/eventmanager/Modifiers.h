#ifndef MODIFIERS_H_
#define MODIFIERS_H_

#include <map>
#include <vector>
#include <string>
#include "gdk/gdkkeysyms.h"
#include "gdk/gdkevents.h"

/* greebo: This class handles the Modifier definitions and translations between
 * GDK modifier flags and the internally used modifierflags. 
 */

class Modifiers
{
	// The association of "SHIFT" to its bit index
	typedef std::map<const std::string, unsigned int> ModifierBitIndexMap;

	// Needed for boost::algorithm::split		
	typedef std::vector<std::string> StringParts;

	// The list of all modifier bit indices
	ModifierBitIndexMap _modifierBitIndices;
	
	// The current modifier state
	unsigned int _modifierState;
	
public:
	// Constructor, loads the modifier definitions from the XMLRegistry
	Modifiers();
	
	// Loads the definitions from the xml nodes
	void loadModifierDefinitions();
	
	unsigned int getModifierFlags(const std::string& modifierStr);
	
	GdkModifierType getGdkModifierType(const unsigned int& modifierFlags);

	// Returns the bit index of the given modifier name argument ("SHIFT")
	// @returns: -1, if not matching name is found (warning to globalOutput is written)
	int getModifierBitIndex(const std::string& modifierName);
	
	// Returns a bit field with the according modifier flags set for the given GDK event->state 
	unsigned int getKeyboardFlags(const unsigned int& state);
	
	// Returns a string for the given modifier flags set (e.g. "SHIFT+CONTROL") 
	std::string getModifierStr(const unsigned int& modifierFlags, bool forMenu = false);
	
	/** greebo: Retrieves the current modifier mask
	 */
	unsigned int getState() const;
	
	/** greebo: Set the state (used to reset the modifiers after a shortcut is found) 
	 */
	void setState(unsigned int state);
	
	/** greebo: Updates the internal modifierstate with the given EventKey.
	 * 
	 * @keyPress: TRUE, if the eventkey is related to an KeyPress event
	 * 			  FALSE, if according to a KeyRelease event. 
	 */
	void updateState(GdkEventKey* event, bool keyPress);
	
}; // class Modifiers

#endif /*MODIFIERS_H_*/
