#pragma once

#include "wxutil/preview/GuiView.h"

namespace gui
{

/**
 * greebo: Specialisation of the generic GuiView with regard to readables.
 * The viewport is cropped to the size of the "backgroundImage" windowDef.
 */
class ReadableGuiView :
	public wxutil::GuiView
{
protected:
	Vector2 _bgDims;

	std::vector<std::string> backgroundDefList;

public:
	ReadableGuiView(wxWindow* parent);

	virtual void setGui(const IGuiPtr& gui) override;

protected:
	virtual void setGLViewPort();
};

} // namespace
