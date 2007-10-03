#include "Modifiers.h"

#include "iregistry.h"

#include "stream/textstream.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

// Constructor, loads the modifier nodes from the registry
Modifiers::Modifiers() :
	_modifierState(0)
{}

void Modifiers::loadModifierDefinitions() {
	
	xml::NodeList modifiers = GlobalRegistry().findXPath("user/ui/input//modifiers");
	
	if (modifiers.size() > 0) {
		// Find all button definitions
		xml::NodeList modifierList = modifiers[0].getNamedChildren("modifier");
		
		if (modifierList.size() > 0) {
			globalOutputStream() << "EventManager: Modifiers found: " 
								 << static_cast<int>(modifierList.size()) << "\n";
			for (unsigned int i = 0; i < modifierList.size(); i++) {
				const std::string name = modifierList[i].getAttributeValue("name");
				
				int bitIndex;
				try {
					bitIndex = boost::lexical_cast<int>(modifierList[i].getAttributeValue("bitIndex"));
				}
				catch (boost::bad_lexical_cast e) {
					bitIndex = -1;
				}
				
				if (name != "" && bitIndex >= 0) {
					// Save the modifier ID into the map
					_modifierBitIndices[name] = static_cast<unsigned int>(bitIndex);
				} 
				else {
					globalOutputStream() << "EventManager: Warning: Invalid modifier definition found.\n";
				}
			}
		}
		else {
			// No Button definitions found!
			globalOutputStream() << "EventManager: Critical: No modifiers definitions found!\n";
		}
	}
	else {
		// No Button definitions found!
		globalOutputStream() << "EventManager: Critical: No modifiers definitions found!\n";
	}
}

unsigned int Modifiers::getModifierFlags(const std::string& modifierStr) {
	StringParts parts;
	boost::algorithm::split(parts, modifierStr, boost::algorithm::is_any_of("+"));
	
	// Do we have any modifiers at all?
	if (parts.size() > 0) {
		unsigned int returnValue = 0;
		
		// Cycle through all the modifier names and construct the bitfield
		for (unsigned int i = 0; i < parts.size(); i++) {
			if (parts[i] == "") continue;
			
			// Try to find the modifierBitIndex
			int bitIndex = getModifierBitIndex(parts[i]);
						
			// Was anything found? 
			if (bitIndex >= 0) {
				unsigned int bitValue = (1 << static_cast<unsigned int>(bitIndex));
				returnValue |= bitValue;
			}
		}
		
		return returnValue;
	}
	else {
		return 0;
	}
}

GdkModifierType Modifiers::getGdkModifierType(const unsigned int& modifierFlags) {
	unsigned int returnValue = 0;
	
	if ((modifierFlags & (1 << getModifierBitIndex("CONTROL"))) != 0) {
		returnValue |= GDK_CONTROL_MASK;
	}
	
	if ((modifierFlags & (1 << getModifierBitIndex("SHIFT"))) != 0) {
		returnValue |= GDK_SHIFT_MASK;
	}
	
	if ((modifierFlags & (1 << getModifierBitIndex("ALT"))) != 0) {
		returnValue |= GDK_MOD1_MASK;
	}
	
	return static_cast<GdkModifierType>(returnValue); 
}

int Modifiers::getModifierBitIndex(const std::string& modifierName) {
	ModifierBitIndexMap::iterator it = _modifierBitIndices.find(modifierName);
   	if (it != _modifierBitIndices.end()) {
   		return it->second;
   	}
   	else {
   		globalOutputStream() << "EventManager: Warning: Modifier " << modifierName.c_str() << " not found, returning -1\n";
   		return -1;
   	}
}

// Returns a bit field with the according modifier flags set 
unsigned int Modifiers::getKeyboardFlags(const unsigned int& state) {
	unsigned int returnValue = 0;
	
	if ((state & GDK_CONTROL_MASK) != 0) {
    	returnValue |= (1 << getModifierBitIndex("CONTROL"));
	}
	
	if ((state & GDK_SHIFT_MASK) != 0) {
    	returnValue |= (1 << getModifierBitIndex("SHIFT"));
	}
	
	if ((state & GDK_MOD1_MASK) != 0) {
    	returnValue |= (1 << getModifierBitIndex("ALT"));
	}
	
	return returnValue;
}

// Returns a string for the given modifier flags set (e.g. "SHIFT+CONTROL") 
std::string Modifiers::getModifierStr(const unsigned int& modifierFlags, bool forMenu) {
	std::string returnValue = "";
	
	const std::string controlStr = (forMenu) ? "Ctrl" : "CONTROL";
	const std::string shiftStr = (forMenu) ? "Shift" : "SHIFT";
	const std::string altStr = (forMenu) ? "Alt" : "ALT";
	const std::string connector = (forMenu) ? "-" : "+";
	
	if ((modifierFlags & (1 << getModifierBitIndex("CONTROL"))) != 0) {
		returnValue += (returnValue != "") ? connector : "";
		returnValue += controlStr;
	}
	
	if ((modifierFlags & (1 << getModifierBitIndex("SHIFT"))) != 0) {
		returnValue += (returnValue != "") ? connector : "";
		returnValue += shiftStr;
	}
	
	if ((modifierFlags & (1 << getModifierBitIndex("ALT"))) != 0) {
		returnValue += (returnValue != "") ? connector : "";
		returnValue += altStr;
	}
	
	return returnValue;
}

unsigned int Modifiers::getState() const {
	return _modifierState;
}

void Modifiers::setState(unsigned int state) {
	_modifierState = state;
}

void Modifiers::updateState(GdkEventKey* event, bool keyPress) {
	unsigned int mask = 0;
	
	int ctrlMask = 1 << getModifierBitIndex("CONTROL");
	int shiftMask = 1 << getModifierBitIndex("SHIFT");
	int altMask = 1 << getModifierBitIndex("ALT");
	
	mask |= (event->keyval == GDK_Control_L || event->keyval == GDK_Control_R) ? ctrlMask : 0;
	mask |= (event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R) ? shiftMask : 0;
	mask |= (event->keyval == GDK_Alt_L || event->keyval == GDK_Alt_R) ? altMask : 0;
	
	if (keyPress) {
		_modifierState |= mask;
	}
	else {
		_modifierState &= ~mask;
	}
}
