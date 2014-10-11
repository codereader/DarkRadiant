#include "LogicEditor.h"

#include "i18n.h"

#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>

namespace objectives 
{

LogicEditor::LogicEditor(wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{
	wxFlexGridSizer* table = new wxFlexGridSizer(2, 2, 6, 12);
	table->AddGrowableCol(1);
	SetSizer(table);

	// Create the text entry fields
	_successLogic = new wxTextCtrl(this, wxID_ANY);
	_failureLogic = new wxTextCtrl(this, wxID_ANY);

	// Create the labels for each text entry field
	wxStaticText* successLogicLabel = new wxStaticText(this, wxID_ANY, _("Success Logic:"));
	wxStaticText* failureLogicLabel = new wxStaticText(this, wxID_ANY, _("Failure Logic:"));

	table->Add(successLogicLabel, 0, wxBOTTOM | wxALIGN_CENTER_VERTICAL, 6);
	table->Add(_successLogic, 0, wxBOTTOM | wxEXPAND, 6);

	table->Add(failureLogicLabel, 0, wxBOTTOM | wxALIGN_CENTER_VERTICAL, 6);
	table->Add(_failureLogic, 0, wxBOTTOM | wxEXPAND, 6);
}

std::string LogicEditor::getSuccessLogicStr()
{
	return _successLogic->GetValue().ToStdString();
}

std::string LogicEditor::getFailureLogicStr()
{
	return _failureLogic->GetValue().ToStdString();
}

void LogicEditor::setSuccessLogicStr(const std::string& logicStr)
{
	_successLogic->SetValue(logicStr);
}

void LogicEditor::setFailureLogicStr(const std::string& logicStr)
{
	_failureLogic->SetValue(logicStr);
}

} // namespace objectives
