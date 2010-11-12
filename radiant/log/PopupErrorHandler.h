#ifndef _POPUP_ERROR_HANDLER_H_
#define _POPUP_ERROR_HANDLER_H_

#include "debugging/debugging.h"
#include "gtkutil/dialog/MessageBox.h"

namespace radiant
{

class PopupErrorHandler
{
public:
	static void HandleError(const std::string& title, const std::string& msg)
	{
		gtkutil::MessageBox box(title, msg, ui::IDialog::MESSAGE_ASK);

		if (box.run() == ui::IDialog::RESULT_YES)
		{
			DEBUGGER_BREAKPOINT();
		}
	}
};

} // namespace radiant

#endif /* _POPUP_ERROR_HANDLER_H_ */
