#pragma once

#include "igui.h"
#include "wxutil/preview/GuiView.h"

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

protected:
	virtual void setGLViewPort();
};

} // namespace
