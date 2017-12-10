#pragma once

#include "GameSetupPage.h"

class wxTextCtrl;
namespace wxutil { class PathEntry; }

namespace ui
{

/**
* A Setup Page for generic idTech-based games like
* Doom3, Quake3, Quake4, etc.
*/
class GameSetupPageIdTech :
	public GameSetupPage
{
private:
	wxTextCtrl* _fsGameEntry;
	wxTextCtrl* _fsGameBaseEntry;
	wxutil::PathEntry* _enginePathEntry;

public:
	GameSetupPageIdTech(wxWindow* parent, const game::IGamePtr& game);

	static const char* TYPE();

	const char* getType() override;
	void validateSettings() override;
	void onPageShown() override;

private:
	void constructPaths();
};

}
