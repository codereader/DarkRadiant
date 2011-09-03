#pragma once

#include "gtkutil/window/BlockingTransientWindow.h"
#include "imd5model.h"

#include "AnimationPreview.h"
#include "icommandsystem.h"

namespace ui
{

class MD5AnimationViewer :
	public gtkutil::BlockingTransientWindow
{
private:
	// Animation preview widget
	AnimationPreviewPtr _preview;

public:
	MD5AnimationViewer();

	static void Show(const cmd::ArgumentList& args);

protected:
	// Override BlockingTransientWindow::_postShow()
	void _postShow();

private:
	// gtkmm callbacks
	void _onOK();
	void _onCancel();

	Gtk::Widget& createButtons();
};

}
