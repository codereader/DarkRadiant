#pragma once

#include "icommandsystem.h"
#include "wxutil/dialog/DialogBase.h"
#include "GameSetupPage.h"

class wxChoicebook;

namespace ui
{

class GameSetupDialog :
	public wxutil::DialogBase
{
private:
	wxChoicebook* _book;

	GameSetupDialog(wxWindow* parent);

public:
	/** greebo: The command target to show the Game settings preferences.
	*/
	static void Show(const cmd::ArgumentList& args);

private:
	void initialiseControls();
};

}
