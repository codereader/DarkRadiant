#pragma once

#include "icommandsystem.h"
#include "wxutil/dialog/DialogBase.h"

namespace ui
{

class GameSetupDialog :
	public wxutil::DialogBase
{
private:
	GameSetupDialog(wxWindow* parent);

public:
	/** greebo: The command target to show the Game settings preferences.
	*/
	static void Show(const cmd::ArgumentList& args);
};

}
