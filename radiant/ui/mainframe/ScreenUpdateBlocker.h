#pragma once

#include "imainframe.h"
#include <memory>

#include "wxutil/ModalProgressDialog.h"

namespace ui
{

class ScreenUpdateBlocker :
	public IScopedScreenUpdateBlocker,
	public wxutil::ModalProgressDialog
{
private:
	std::unique_ptr<wxWindowDisabler> _disabler;

	// Once we received a call to setProgress() further calls to pulse() are forbidden
	bool _pulseAllowed;

public:
	// Pass the window title and the text message to the constructor
	ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay = false);

	void pulse();
	void setProgress(float progress);
	void setMessage(const std::string& message);
	void setMessageAndProgress(const std::string& message, float progress);

	~ScreenUpdateBlocker();

private:
	void doSetProgress(float progress);
	void doSetMessage(const std::string& message);

	// Called whenever the main window is changing its "active" state property.
	void onMainWindowFocus(wxFocusEvent& ev);
	void onCloseEvent(wxCloseEvent& ev);
};

} // namespace ui
