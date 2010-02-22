#ifndef _READABLE_EDITOR_DIALOG_H_
#define _READABLE_EDITOR_DIALOG_H_

#include "icommandsystem.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gui/GuiView.h"
#include <map>

namespace ui
{

class ReadableEditorDialog :
	public gtkutil::BlockingTransientWindow
{
public:
	enum Result
	{
		RESULT_OK,
		RESULT_CANCEL,
		NUM_RESULTS,
	};

private:
	gui::GuiViewPtr _guiView;

	// A container for storing enumerated widgets
	std::map<int, GtkWidget*> _widgets;

	Result _result;

public:
	ReadableEditorDialog();

	static void RunDialog(const cmd::ArgumentList& args);

protected:
	virtual void _postShow();

private:
	GtkWidget* createEditPane();
	GtkWidget* createButtonPanel();

	static void onCancel(GtkWidget* widget, ReadableEditorDialog* self);
	static void onSave(GtkWidget* widget, ReadableEditorDialog* self);
};

} // namespace ui

#endif /* _READABLE_EDITOR_DIALOG_H_ */
