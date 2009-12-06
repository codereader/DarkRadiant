#ifndef _UI_DIALOG_H_
#define _UI_DIALOG_H_

#include "idialogmanager.h"
#include "gtkutil/window/BlockingTransientWindow.h"

namespace ui
{

class DialogManager;

class Dialog :
	public IDialog,
	public gtkutil::BlockingTransientWindow
{
private:
	// The unique ID of this dialog
	const std::size_t _id;

	// The owning manager
	DialogManager& _owner;

	IDialog::Result _result;

public:
	Dialog(std::size_t id, DialogManager& owner);

	std::size_t getId() const;

	virtual void setTitle(const std::string& title);

	// Enter the main loop
	virtual Result run();

	// Frees this dialog and all its allocated resources.  Once a dialog as been destroyed, 
	// calling any methods on this object results in undefined behavior.
	virtual void destroy();
};
typedef boost::shared_ptr<Dialog> DialogPtr;

} // namespace ui

#endif /* _UI_DIALOG_H_ */
