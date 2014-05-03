#include "SerialisableWidgets.h"

#include "string/convert.h"

#include "TreeModel.h"

#include <iostream>

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
		std::cerr << "SerialisableComboBox_Index::importFromString(): "
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
		std::cerr << "SerialisableComboBox_Index::importFromString(): "
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

}


namespace gtkutil
{

// Adjustment

SerialisableAdjustment::SerialisableAdjustment(double value, double lower, double upper) :
	Gtk::Adjustment(value, lower, upper)
{}

void SerialisableAdjustment::importFromString(const std::string& str)
{
	set_value(string::convert<double>(str));
}

std::string SerialisableAdjustment::exportToString() const
{
	return string::to_string(get_value());
}

SerialisableAdjustmentWrapper::SerialisableAdjustmentWrapper(Gtk::Adjustment* adjustment) :
	_adjustment(adjustment)
{}

void SerialisableAdjustmentWrapper::importFromString(const std::string& str)
{
	_adjustment->set_value(string::convert<double>(str));
}

std::string SerialisableAdjustmentWrapper::exportToString() const
{
	return string::to_string(_adjustment->get_value());
}

// Text entry

SerialisableTextEntry::SerialisableTextEntry() :
	Gtk::Entry()
{}

void SerialisableTextEntry::importFromString(const std::string& str)
{
	set_text(str);
}

std::string SerialisableTextEntry::exportToString() const
{
	return std::string(get_text());
}

SerialisableTextEntryWrapper::SerialisableTextEntryWrapper(Gtk::Entry* entry) :
	_entry(entry)
{}

void SerialisableTextEntryWrapper::importFromString(const std::string& str)
{
	_entry->set_text(str);
}

std::string SerialisableTextEntryWrapper::exportToString() const
{
	return _entry->get_text();
}

// Spin button

SerialisableSpinButton::SerialisableSpinButton(double value,
											   double min,
											   double max,
											   double step,
											   guint digits)
											   : Gtk::SpinButton()
{
	Gtk::Adjustment* adj = Gtk::manage(new Gtk::Adjustment(value, min, max, step));
	configure(*adj, 0.0, digits);
}

void SerialisableSpinButton::importFromString(const std::string& str)
{
	set_value(string::convert<double>(str));
}

std::string SerialisableSpinButton::exportToString() const
{
	return string::to_string(get_value());
}

SerialisableSpinButtonWrapper::SerialisableSpinButtonWrapper(Gtk::SpinButton* spin) :
	_spin(spin)
{}

void SerialisableSpinButtonWrapper::importFromString(const std::string& str)
{
	_spin->set_value(string::convert<double>(str));
}

std::string SerialisableSpinButtonWrapper::exportToString() const
{
	return string::to_string(_spin->get_value());
}

// Scale widget

SerialisableScaleWidget::SerialisableScaleWidget() :
	Gtk::Range()
{}

void SerialisableScaleWidget::importFromString(const std::string& str)
{
	set_value(string::convert<double>(str));
}

std::string SerialisableScaleWidget::exportToString() const
{
	return string::to_string(get_value());
}

SerialisableScaleWidgetWrapper::SerialisableScaleWidgetWrapper(Gtk::Range* range) :
	_range(range)
{}

void SerialisableScaleWidgetWrapper::importFromString(const std::string& str)
{
	_range->set_value(string::convert<double>(str));
}

std::string SerialisableScaleWidgetWrapper::exportToString() const
{
	return string::to_string(_range->get_value());
}

// Toggle button

SerialisableToggleButton::SerialisableToggleButton() :
	Gtk::ToggleButton()
{}

SerialisableToggleButton::SerialisableToggleButton(const std::string& label) :
	Gtk::ToggleButton(label)
{}

void SerialisableToggleButton::importFromString(const std::string& str)
{
	set_active(str == "1");
}

std::string SerialisableToggleButton::exportToString() const
{
	return get_active() ? "1" : "0";
}

SerialisableToggleButtonWrapper::SerialisableToggleButtonWrapper(Gtk::ToggleButton* button) :
	_button(button)
{}

void SerialisableToggleButtonWrapper::importFromString(const std::string& str)
{
	_button->set_active(str == "1");
}

std::string SerialisableToggleButtonWrapper::exportToString() const
{
	return _button->get_active() ? "1" : "0";
}

// Check Button

SerialisableCheckButton::SerialisableCheckButton() :
	Gtk::CheckButton()
{}

SerialisableCheckButton::SerialisableCheckButton(const std::string& label) :
	Gtk::CheckButton(label)
{}

void SerialisableCheckButton::importFromString(const std::string& str)
{
	set_active(str == "1");
}

std::string SerialisableCheckButton::exportToString() const
{
	return get_active() ? "1" : "0";
}

SerialisableCheckButtonWrapper::SerialisableCheckButtonWrapper(Gtk::CheckButton* button) :
	_button(button)
{}

void SerialisableCheckButtonWrapper::importFromString(const std::string& str)
{
	_button->set_active(str == "1");
}

std::string SerialisableCheckButtonWrapper::exportToString() const
{
	return _button->get_active() ? "1" : "0";
}

// SerialisableComboBox_Index

SerialisableComboBox_Index::SerialisableComboBox_Index() :
	SerialisableComboBox()
{}

void SerialisableComboBox_Index::importFromString(const std::string& str)
{
	int activeId = string::convert<int>(str);
	set_active(activeId);

	int newId = get_active_row_number();

	if (activeId != newId)
	{
		std::cerr << "SerialisableComboBox_Index::importFromString(): "
				<< "warning: requested index " << activeId
				<< " was not set, current index is " << newId << std::endl;
	}
}

std::string SerialisableComboBox_Index::exportToString() const
{
	return string::to_string(get_active_row_number());
}

SerialisableComboBox_IndexWrapper::SerialisableComboBox_IndexWrapper(Gtk::ComboBoxText* combo) :
	_combo(combo)
{}

void SerialisableComboBox_IndexWrapper::importFromString(const std::string& str)
{
	int activeId = string::convert<int>(str);
	_combo->set_active(activeId);

	int newId = _combo->get_active_row_number();

	if (activeId != newId)
	{
		std::cerr << "SerialisableComboBox_Index::importFromString(): "
				<< "warning: requested index " << activeId
				<< " was not set, current index is " << newId << std::endl;
	}
}

std::string SerialisableComboBox_IndexWrapper::exportToString() const
{
	return string::to_string(_combo->get_active_row_number());
}

// SerialisableComboBox_Text

SerialisableComboBox_Text::SerialisableComboBox_Text() :
	SerialisableComboBox()
{}

void SerialisableComboBox_Text::importFromString(const std::string& str)
{
	set_active_text(str);
}

std::string SerialisableComboBox_Text::exportToString() const
{
	return get_active_text();
}

SerialisableComboBox_TextWrapper::SerialisableComboBox_TextWrapper(Gtk::ComboBoxText* combo) :
	_combo(combo)
{}

void SerialisableComboBox_TextWrapper::importFromString(const std::string& str)
{
	_combo->set_active_text(str);
}

std::string SerialisableComboBox_TextWrapper::exportToString() const
{
	return _combo->get_active_text();
}

}
