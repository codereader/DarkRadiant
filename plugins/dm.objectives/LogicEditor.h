#pragma once

#include <string>
#include <wx/panel.h>

class wxTextCtrl;

namespace objectives
{

/**
 * greebo: This complex widget represents the UI elements needed to edit
 * a set of success- and failure logic strings.
 */
class LogicEditor :
	public wxPanel
{
private:
	wxTextCtrl* _successLogic;
	wxTextCtrl* _failureLogic;

public:
	/**
	 * The constructor will create the widgets.
	 */
	LogicEditor(wxWindow* parent);

	// Read accessors for the logic strings
	std::string getSuccessLogicStr();
	std::string getFailureLogicStr();

	// Write accessors for the logic strings
	void setSuccessLogicStr(const std::string& logicStr);
	void setFailureLogicStr(const std::string& logicStr);
};

} // namespace objectives
