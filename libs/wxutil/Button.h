#pragma once

#include <string>
#include <wx/button.h>

#include "icommandsystem.h"

namespace wxutil
{

namespace button
{

// Connects the click event of the given button to the specified command
inline void connectToCommand(wxButton* button, const char* command)
{
	button->Bind(wxEVT_BUTTON, [=](wxCommandEvent&)
	{
		GlobalCommandSystem().execute(command);
	});
}

}

}
