#pragma once

#include "GameSetupPage.h"

class wxComboBox;
namespace wxutil { class PathEntry; }

namespace ui
{

/**
* A Setup Page for the standalone version
* of The Dark Mod (version 2.0 and later).
*/
class GameSetupPageTdm: public GameSetupPage
{
	wxComboBox* _missionEntry = nullptr;
	wxutil::PathEntry* _enginePathEntry = nullptr;

	std::string _fmFolder;

	class FmFolderHistory;
	std::unique_ptr<FmFolderHistory> _fmFolderHistory;

public:
	GameSetupPageTdm(wxWindow* parent, const game::IGamePtr& game);

	static const char* TYPE();

	const char* getType() override;
	void validateSettings() override;
	bool onPreSave() override;
	void onPageShown() override;
	void onClose() override;

private:
	void constructPaths();
	void populateAvailableMissionPaths();
};

}
