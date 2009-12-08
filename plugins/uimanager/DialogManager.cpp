#include "DialogManager.h"

#include "itextstream.h"
#include "iradiant.h"

#include "gtkutil/dialog/MessageBox.h"

namespace ui
{

DialogManager::~DialogManager()
{
	if (!_dialogs.empty())
	{
		globalOutputStream() << "DialogManager: " << _dialogs.size() 
			<< " dialogs still in memory at shutdown." << std::endl;
		_dialogs.clear();
	}
}

IDialogPtr DialogManager::createDialog(const std::string& title, GtkWindow* parent)
{
	cleanupOldDialogs();

	if (parent == NULL)
	{
		parent = GlobalRadiant().getMainWindow();
	}

	// Allocate a new dialog
	gtkutil::DialogPtr dialog(new gtkutil::Dialog(title, parent));

	_dialogs.push_back(dialog);

	return dialog;
}

IDialogPtr DialogManager::createMessageBox(const std::string& title, 
										   const std::string& text, 
										   IDialog::MessageType type, 
										   GtkWindow* parent)
{
	cleanupOldDialogs();

	// Use the main window if no parent specified
	if (parent == NULL)
	{
		parent = GlobalRadiant().getMainWindow();
	}

	// Allocate a new dialog
	gtkutil::MessageBoxPtr box(new gtkutil::MessageBox(title, text, type, parent));

	// Store it in the local map so that references are held
	_dialogs.push_back(box);

	return box;
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
