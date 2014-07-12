#pragma once

#include "debugging/debugging.h"
#include "wxutil/dialog/MessageBox.h"

namespace radiant
{

class PopupErrorHandler
{
public:
	static void HandleError(const std::string& title, const std::string& msg)
	{
		if (wxutil::Messagebox::Show(title, msg, ui::IDialog::MESSAGE_ASK) == ui::IDialog::RESULT_YES)
		{
			DEBUGGER_BREAKPOINT();
		}
	}
};

} // namespace radiant

