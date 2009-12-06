#ifndef _IDIALOG_MANAGER_H_
#define _IDIALOG_MANAGER_H_

#include <boost/shared_ptr.hpp>

namespace ui
{

/**
 * Abstract dialog class, represents a modal window parented to 
 * DarkRadiant's top-level window.
 */ 
class IDialog
{
public:
	virtual ~IDialog() {}

	enum Result
	{
		RESULT_CANCELLED = 0,
		RESULT_OK,
	};

	/**
	 * Run the dialog an enter the main loop (block the application).
	 * Returns the Dialog::Result, corresponding to the user's action.
	 */
	virtual Result run() = 0;

	// Frees this dialog and all its allocated resources.  Once a dialog as been destroyed, 
	// calling any methods on this object results in undefined behavior.
	virtual void destroy() = 0;
};
typedef boost::shared_ptr<IDialog> IDialogPtr;

class IDialogManager
{
public:
	// Virtual destructor
	virtual ~IDialogManager() {}

	// Create a new dialog. Note that the DialogManager will hold a reference
	// to this dialog internally until destroy() is called on the dialog object.
	// This allows scripts to reference the Dialog class without holding the 
	// shared_ptr on their own or using wrapper classes doing so.
	// Dialogs will be cleared at radiant shutdown at the latest, but it's recommended
	// to call Dialog::destroy() when you're done using it.
	virtual IDialogPtr createDialog() = 0;
};

} // namespace ui

#endif /* _IDIALOG_MANAGER_H_ */
