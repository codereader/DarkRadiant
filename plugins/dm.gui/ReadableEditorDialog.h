#ifndef _FIXUP_MAP_DIALOG_H_
#define _FIXUP_MAP_DIALOG_H_

#include "icommandsystem.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include "GuiView.h"

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

#endif /* _FIXUP_MAP_DIALOG_H_ */
