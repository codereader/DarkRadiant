#pragma once

#include <string>
#include <wx/button.h>

#include "icommandsystem.h"

namespace wxutil
{

namespace button
{

/// Connects the click event of the given button to the specified command
///
/// @internal While connecting buttons to commands can be convenient, it bypasses
/// compile-time type checking and can make IDE navigation more difficult (since you can't
/// jump to the definition of a command like you can with an actual C++ function). Consider
/// if it's possible to connect the button directly to the actual method which implements
/// the command instead of using this function.
inline void connectToCommand(wxButton* button, const char* command)
{
	button->Bind(wxEVT_BUTTON, [=](wxCommandEvent&)
	{
		GlobalCommandSystem().execute(command);
	});
}

}

}
