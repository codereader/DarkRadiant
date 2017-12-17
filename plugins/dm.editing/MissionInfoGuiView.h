#pragma once

#include "igui.h"
#include "wxutil/preview/GuiView.h"

#include "DarkmodTxt.h"
#include "ReadmeTxt.h"

namespace ui
{

/**
* greebo: Specialisation of the generic GuiView with regard to the mission info dialog.
* The viewport is cropped to the size of a certain windowDef.
*/
class MissionInfoGuiView :
	public wxutil::GuiView
{
protected:
	Vector2 _bgDims;

	std::vector<std::string> backgroundDefList;

public:
	MissionInfoGuiView(wxWindow* parent);

	virtual void setGui(const gui::IGuiPtr& gui) override;

	// Loads values into the GUI (state) variables
	virtual void updateGuiState() = 0;

protected:
	virtual void setGLViewPort();

	// Returns the name of the single window def to display
	virtual std::string getTargetWindowDefName() = 0;
};

// Specialisation for displaying info in darkmod.txt
// which will filter itself to the "ModToInstallParent" windowDef
class DarkmodTxtGuiView :
	public MissionInfoGuiView
{
private:
	// The file containing the mission info this view is previewing
	map::DarkmodTxtPtr _file;

public:
	DarkmodTxtGuiView(wxWindow* parent);

	void updateGuiState() override;

	void setMissionInfoFile(const map::DarkmodTxtPtr& file);

protected:
	virtual std::string getTargetWindowDefName() override;
};

// Specialisation for displaying info in readme.txt
// which will filter itself to the "ModInstallationNotesParchment" windowDef
class ReadmeTxtGuiView :
	public MissionInfoGuiView
{
private:
	// The file containing the mission info this view is previewing
	map::ReadmeTxtPtr _file;

public:
	ReadmeTxtGuiView(wxWindow* parent);

	void updateGuiState() override;

	void setMissionInfoFile(const map::ReadmeTxtPtr& file);

protected:
	virtual std::string getTargetWindowDefName() override;
};

} // namespace
