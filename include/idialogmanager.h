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
		RESULT_CANCELED = 0,
		RESULT_OK,
	};

	/**
	 * Run the dialog an enter the main loop (block the application).
	 * Returns the Dialog::Result, corresponding to the user's action.
	 */
	virtual Result run() = 0;
};
typedef boost::shared_ptr<IDialog> IDialogPtr;

class IDialogManager
{
public:
	// Virtual destructor
	virtual ~IDialogManager() {}

	// Create a new dialog
	virtual IDialogPtr createDialog() = 0;
};

} // namespace ui

#endif /* _IDIALOG_MANAGER_H_ */
