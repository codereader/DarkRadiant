#ifndef _READABLE_GUI_VIEW_H_
#define _READABLE_GUI_VIEW_H_

#include "gui/GuiView.h"

namespace gui
{

/**
 * greebo: Specialisation of the generic GuiView with regard to readables.
 * The viewport is cropped to the size of the "backgroundImage" windowDef.
 */
class ReadableGuiView :
	public GuiView
{
protected:
	Vector2 _bgDims;

	std::vector<std::string> backgroundDefList;

public:
	virtual void setGui(const GuiPtr& gui);

protected:
	virtual void setGLViewPort();
};

} // namespace

#endif /* _READABLE_GUI_VIEW_H_ */
