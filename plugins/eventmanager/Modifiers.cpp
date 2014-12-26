#include "Modifiers.h"

#include "i18n.h"

#include <wx/event.h>
#include "wxutil/MouseButton.h"

// Returns a bit field with the according modifier flags set
unsigned int Modifiers::getKeyboardFlagsFromMouseButtonState(unsigned int state)
{
    return state & (wxutil::Modifier::CONTROL | wxutil::Modifier::ALT | wxutil::Modifier::SHIFT);
}

// Returns a string for the given modifier flags set (e.g. "SHIFT+CONTROL")
std::string Modifiers::getModifierStr(const unsigned int modifierFlags, bool forMenu) 
{
	std::string returnValue = "";

	const std::string controlStr = (forMenu) ? _("Ctrl") : "CONTROL";
	const std::string shiftStr = (forMenu) ? _("Shift") : "SHIFT";
	const std::string altStr = (forMenu) ? _("Alt") : "ALT";
	const std::string connector = (forMenu) ? "-" : "+";

	if ((modifierFlags & wxutil::Modifier::CONTROL) != 0) {
		returnValue += (returnValue != "") ? connector : "";
		returnValue += controlStr;
	}

    if ((modifierFlags & wxutil::Modifier::SHIFT) != 0) {
		returnValue += (returnValue != "") ? connector : "";
		returnValue += shiftStr;
	}

    if ((modifierFlags & wxutil::Modifier::ALT) != 0) {
		returnValue += (returnValue != "") ? connector : "";
		returnValue += altStr;
	}

	return returnValue;
}
