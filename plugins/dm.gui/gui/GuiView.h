#ifndef GuiView_h__
#define GuiView_h__

#include "gtkutil/ifc/Widget.h"
#include "gtkutil/GLWidget.h"
#include "GuiRenderer.h"
#include "Gui.h"

#include "GuiManager.h"

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
	GuiPtr _gui;

public:
	GuiView();

	// Sets the GUI to render (by VFS path)
	void setGui(const std::string& gui)
	{
		setGui(GuiManager::Instance().getGui(gui));
	}

	// Sets the GUI to render (can be NULL to clear this view)
	void setGui(const GuiPtr& gui);

	/** 
	 * Initialise the GL view. This clears the window and sets up the 
	 * initial matrices.
	 */
	void initialiseView();

protected:
	// Widget implementation
	virtual GtkWidget* _getWidget() const
	{
		return _widget;
	}

private:
	static void onGLDraw(GtkWidget*, GdkEventExpose*, GuiView* self);
};
typedef boost::shared_ptr<GuiView> GuiViewPtr;

}

#endif // GuiView_h__
