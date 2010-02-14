#ifndef GuiView_h__
#define GuiView_h__

#include "gtkutil/ifc/Widget.h"
#include "gtkutil/GLWidget.h"
#include "GuiRenderer.h"

namespace gui
{

/**
 * greebo: Use this class to add a GUI display to your dialogs.
 * It is owning a GL widget as well as a GuiRenderer which
 * is taking care of rendering the GUI elements to GL.
 */
class GuiView :
	public gtkutil::Widget
{
private:
	// The top-level widget for packing this into a parent container
	GtkWidget* _widget;

	// The GL widget
	gtkutil::GLWidgetPtr _glWidget;

	// The GUI renderer is submitting stuff to GL
	GuiRenderer _renderer;

	// The GUI to render
	// GuiPtr _gui;

public:
	GuiView();

	// Sets the GUI to render 
	// (TODO: replace this with a setGui(const GuiPtr& gui) method)
	// This class shouldn't be responsible of loading the GUI file
	void setGui(const std::string& gui)
	{
		// TODO
	}

protected:
	// Widget implementation
	virtual GtkWidget* _getWidget() const
	{
		return _widget;
	}
};
typedef boost::shared_ptr<GuiView> GuiViewPtr;

}

#endif // GuiView_h__
