#ifndef _DIALOG_MANAGER_H_
#define _DIALOG_MANAGER_H_

#include "idialogmanager.h"

namespace ui
{

class DialogManager :
	public IDialogManager
{
public:
	// Create a new dialog
	IDialogPtr createDialog();
};
typedef boost::shared_ptr<DialogManager> DialogManagerPtr;

} // namespace ui

#endif /* _DIALOG_MANAGER_H_ */
