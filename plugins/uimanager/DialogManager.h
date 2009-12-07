#ifndef _DIALOG_MANAGER_H_
#define _DIALOG_MANAGER_H_

#include "idialogmanager.h"
#include <map>

#include "Dialog.h"

namespace ui
{

class DialogManager :
	public IDialogManager
{
private:
	std::size_t _highestIndex;

	typedef std::map<std::size_t, DialogPtr> DialogMap;
	DialogMap _dialogs;

public:
	DialogManager();

	virtual ~DialogManager();

	// Create a new dialog
	IDialogPtr createDialog(const std::string& title);

	IDialogPtr createMessageBox(const std::string& title, const std::string& text, IDialog::MessageType type);

	// Called by the Dialog classes to allow the DialogManager to clear its resources
	void notifyDestroy(std::size_t id);
};
typedef boost::shared_ptr<DialogManager> DialogManagerPtr;

} // namespace ui

#endif /* _DIALOG_MANAGER_H_ */
