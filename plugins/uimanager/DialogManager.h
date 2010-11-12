#ifndef _DIALOG_MANAGER_H_
#define _DIALOG_MANAGER_H_

#include "idialogmanager.h"
#include <list>

#include "gtkutil/dialog/Dialog.h"

namespace ui
{

class DialogManager :
	public IDialogManager
{
private:
	typedef std::list<gtkutil::DialogPtr> Dialogs;
	Dialogs _dialogs;

public:
	virtual ~DialogManager();

	// Create a new dialog
	IDialogPtr createDialog(const std::string& title,
							const Glib::RefPtr<Gtk::Window>& parent = Glib::RefPtr<Gtk::Window>());

	IDialogPtr createMessageBox(const std::string& title,
								const std::string& text,
								IDialog::MessageType type,
								const Glib::RefPtr<Gtk::Window>& parent = Glib::RefPtr<Gtk::Window>());

	IFileChooserPtr createFileChooser(const std::string& title, bool open, bool browseFolders,
									  const std::string& pattern, const std::string& defaultExt);

private:
	void cleanupOldDialogs();
};
typedef boost::shared_ptr<DialogManager> DialogManagerPtr;

} // namespace ui

#endif /* _DIALOG_MANAGER_H_ */
