#pragma once

#include "igui.h"
#include "wxutil/preview/GuiView.h"
#include "DarkmodTxt.h"

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

	// The file containing the mission info this view is previewing
	map::DarkmodTxtPtr _file;

public:
	MissionInfoGuiView(wxWindow* parent);

	virtual void setGui(const gui::IGuiPtr& gui) override;

	void setMissionInfoFile(const map::DarkmodTxtPtr& file);

	// Loads the values from the attached DarkmodTxt file into the GUI
	void update();

protected:
	virtual void setGLViewPort();
};

} // namespace
