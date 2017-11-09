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

	std::string _enginePath;
	std::string _modBasePath;
	std::string _modPath;

public:
	GameSetupPageIdTech(wxWindow* parent, const game::IGamePtr& game);

	static const char* TYPE();

	const char* getType() override;
	void validateSettings() override;
	void onPageShown() override;

	std::string getEnginePath() override;
	std::string getModBasePath() override;
	std::string getModPath() override;

private:
	void constructPaths();
};

}
