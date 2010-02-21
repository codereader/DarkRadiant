#ifndef _READABLE_EDITOR_DIALOG_H_
#define _READABLE_EDITOR_DIALOG_H_

#include "icommandsystem.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gui/GuiView.h"

namespace ui
{

class ReadableEditorDialog :
	public gtkutil::BlockingTransientWindow
{
private:
	gui::GuiViewPtr _guiView;

public:
	ReadableEditorDialog();

	static void RunDialog(const cmd::ArgumentList& args);

protected:
	virtual void _postShow();
};

} // namespace ui

#endif /* _READABLE_EDITOR_DIALOG_H_ */
