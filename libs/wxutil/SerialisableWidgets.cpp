#include "SerialisableWidgets.h"

#include "string/convert.h"
#include "itextstream.h"

namespace wxutil
{

// Text entry

SerialisableTextEntry::SerialisableTextEntry(wxWindow* parent) :
	wxTextCtrl(parent, wxID_ANY)
{}

void SerialisableTextEntry::importFromString(const std::string& str)
{
	SetValue(str);
}

std::string SerialisableTextEntry::exportToString() const
{
	return GetValue().ToStdString();
}

SerialisableTextEntryWrapper::SerialisableTextEntryWrapper(wxTextCtrl* entry) :
	_entry(entry)
{}

void SerialisableTextEntryWrapper::importFromString(const std::string& str)
{
	_entry->SetValue(str);
}

std::string SerialisableTextEntryWrapper::exportToString() const
{
	return _entry->GetValue().ToStdString();
}

// Spin button

SerialisableSpinButton::SerialisableSpinButton(wxWindow* parent, 
	double value, double min, double max, double step, unsigned int digits) : 
	wxSpinCtrlDouble(parent, wxID_ANY)
{
	SetRange(min, max);
	SetValue(value);
	SetIncrement(step);
	SetDigits(digits);
}

void SerialisableSpinButton::importFromString(const std::string& str)
{
	SetValue(string::convert<double>(str));
}

std::string SerialisableSpinButton::exportToString() const
{
	return string::to_string(GetValue());
}

SerialisableSpinButtonWrapper::SerialisableSpinButtonWrapper(wxSpinCtrlDouble* spin) :
	_spin(spin)
{}

void SerialisableSpinButtonWrapper::importFromString(const std::string& str)
{
	_spin->SetValue(string::convert<double>(str));
}

std::string SerialisableSpinButtonWrapper::exportToString() const
{
	return string::to_string(_spin->GetValue());
}
// Toggle button

SerialisableToggleButton::SerialisableToggleButton(wxWindow* parent) :
	wxToggleButton(parent, wxID_ANY, "")
{}

SerialisableToggleButton::SerialisableToggleButton(wxWindow* parent, const std::string& label) :
	wxToggleButton(parent, wxID_ANY, label)
{}

void SerialisableToggleButton::importFromString(const std::string& str)
{
	SetValue(str == "1");
}

std::string SerialisableToggleButton::exportToString() const
{
	return GetValue() ? "1" : "0";
}

SerialisableToggleButtonWrapper::SerialisableToggleButtonWrapper(wxToggleButton* button) :
	_button(button)
{}

void SerialisableToggleButtonWrapper::importFromString(const std::string& str)
{
	_button->SetValue(str == "1");
}

std::string SerialisableToggleButtonWrapper::exportToString() const
{
	return _button->GetValue() ? "1" : "0";
}

// Check Button

SerialisableCheckButton::SerialisableCheckButton(wxWindow* parent) :
	wxCheckBox(parent, wxID_ANY, "")
{}

SerialisableCheckButton::SerialisableCheckButton(wxWindow* parent, const std::string& label) :
	wxCheckBox(parent, wxID_ANY, label)
{}

void SerialisableCheckButton::importFromString(const std::string& str)
{
	SetValue(str == "1");
}

std::string SerialisableCheckButton::exportToString() const
{
	return GetValue() ? "1" : "0";
}

SerialisableCheckButtonWrapper::SerialisableCheckButtonWrapper(wxCheckBox* button) :
	_button(button)
{}

void SerialisableCheckButtonWrapper::importFromString(const std::string& str)
{
	_button->SetValue(str == "1");
}

std::string SerialisableCheckButtonWrapper::exportToString() const
{
	return _button->GetValue() ? "1" : "0";
}

// SerialisableComboBox_Index

SerialisableComboBox_Index::SerialisableComboBox_Index(wxWindow* parent) :
	SerialisableComboBox(parent)
{}

void SerialisableComboBox_Index::importFromString(const std::string& str)
{
	int activeId = string::convert<int>(str);
	SetSelection(activeId);

	int newId = GetSelection();

	if (activeId != newId)
	{
        rConsoleError() << "SerialisableComboBox_Index::importFromString(): "
				<< "warning: requested index " << activeId
				<< " was not set, current index is " << newId << std::endl;
	}
}

std::string SerialisableComboBox_Index::exportToString() const
{
	return string::to_string(GetSelection());
}

SerialisableComboBox_IndexWrapper::SerialisableComboBox_IndexWrapper(wxChoice* combo) :
	_combo(combo)
{}

void SerialisableComboBox_IndexWrapper::importFromString(const std::string& str)
{
	int activeId = string::convert<int>(str);
	_combo->SetSelection(activeId);

	int newId = _combo->GetSelection();

	if (activeId != newId)
	{
        rConsoleError() << "SerialisableComboBox_Index::importFromString(): "
				<< "warning: requested index " << activeId
				<< " was not set, current index is " << newId << std::endl;
	}
}

std::string SerialisableComboBox_IndexWrapper::exportToString() const
{
	return string::to_string(_combo->GetSelection());
}

// SerialisableComboBox_Text

SerialisableComboBox_Text::SerialisableComboBox_Text(wxWindow* parent) :
	SerialisableComboBox(parent)
{}

void SerialisableComboBox_Text::importFromString(const std::string& str)
{
	SetSelection(FindString(str));
}

std::string SerialisableComboBox_Text::exportToString() const
{
	return GetString(GetSelection()).ToStdString();
}

SerialisableComboBox_TextWrapper::SerialisableComboBox_TextWrapper(wxChoice* combo) :
	_combo(combo)
{}

void SerialisableComboBox_TextWrapper::importFromString(const std::string& str)
{
	_combo->SetSelection(_combo->FindString(str));
}

std::string SerialisableComboBox_TextWrapper::exportToString() const
{
	return _combo->GetString(_combo->GetSelection()).ToStdString();
}

} // namespace
