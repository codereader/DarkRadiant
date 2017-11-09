#pragma once

#include "igui.h"
#include "wxutil/GLWidget.h"
#include "math/Vector2.h"
#include "GuiRenderer.h"

namespace wxutil
{

/**
 * greebo: Use this class to add a GUI display to your dialogs.
 * It is owning a GL widget as well as a GuiRenderer which
 * is taking care of rendering the GUI elements to GL.
 */
class GuiView :
	public wxutil::GLWidget
{
protected:
	// The GUI renderer is submitting stuff to GL
	GuiRenderer _renderer;

	// The GUI to render
	gui::IGuiPtr _gui;

	// The dimensions of the GL widget in pixels.
	Vector2 _windowDims;

public:
	GuiView(wxWindow* parent);

	virtual ~GuiView() {}

	// Sets the GUI to render (can be NULL to clear this view)
	virtual void setGui(const gui::IGuiPtr& gui);

	// Returns the current GUI (can be NULL)
	const gui::IGuiPtr& getGui();

	/**
	 * Initialise the GL view. This clears the window and sets up the
	 * initial matrices.
	 */
	void initialiseView();

	// Triggers a redraw
	void redraw();

protected:
	// Performs the actual GL setup and drawing
	virtual void draw();

	// Calculates the visible area
	virtual void setGLViewPort();

private:
	void onSizeAllocate(wxSizeEvent& ev);
};
typedef std::shared_ptr<GuiView> GuiViewPtr;

}
