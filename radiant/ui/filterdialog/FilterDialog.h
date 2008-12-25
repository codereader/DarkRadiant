#ifndef _FILTER_DIALOG_H_
#define _FILTER_DIALOG_H_

#include "gtkutil/window/BlockingTransientWindow.h"

namespace ui {

class FilterDialog :
	public gtkutil::BlockingTransientWindow
{
	// Private constructor
	FilterDialog();

public:
	/** 
	 * greebo: Shows the dialog (command target)
	 */
	static void showDialog();
};

} // namespace ui

#endif /* _FILTER_DIALOG_H_ */
