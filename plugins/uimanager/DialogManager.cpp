#include "DialogManager.h"

#include "itextstream.h"
#include "imainframe.h"

#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/FileChooser.h"

namespace ui
{

DialogManager::~DialogManager()
{
	if (!_dialogs.empty())
	{
		rMessage() << "DialogManager: " << _dialogs.size()
			<< " dialogs still in memory at shutdown." << std::endl;

		_dialogs.clear();
	}
}

IDialogPtr DialogManager::createDialog(const std::string& title, wxWindow* parent)
{
	cleanupOldDialogs();

	// Allocate a new dialog
	wxutil::DialogPtr dialog(new wxutil::Dialog(title, parent));

	_dialogs.push_back(dialog);

	return dialog;
}

IDialogPtr DialogManager::createMessageBox(const std::string& title,
										   const std::string& text,
										   IDialog::MessageType type,
										   wxWindow* parent)
{
	cleanupOldDialogs();

	// Allocate a new dialog, use the main window if no parent specified
	wxutil::MessageboxPtr box(new wxutil::Messagebox(title, text, type, parent));

	// Store it in the local map so that references are held
	_dialogs.push_back(box);

	return box;
}

IFileChooserPtr DialogManager::createFileChooser(const std::string& title,
	bool open, bool browseFolders, const std::string& pattern, const std::string& defaultExt)
{
	return IFileChooserPtr(new gtkutil::FileChooser(
		GlobalMainFrame().getTopLevelWindow(),
		title, open, browseFolders, pattern, defaultExt));
}

void DialogManager::cleanupOldDialogs()
{
	for (Dialogs::iterator i = _dialogs.begin(); i != _dialogs.end(); /* in-loop increment */)
	{
		if (i->unique())
		{
			_dialogs.erase(i++);
		}
		else
		{
			++i;
		}
	}
}

} // namespace ui
