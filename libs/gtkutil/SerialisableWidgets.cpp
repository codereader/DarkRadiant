#include "SerialisableWidgets.h"

#include "string/string.h"

#include "TreeModel.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

namespace gtkutil
{

// Adjustment

SerialisableAdjustment::SerialisableAdjustment(double value, double lower, double upper) :
	Gtk::Adjustment(value, lower, upper)
{}

void SerialisableAdjustment::importFromString(const std::string& str)
{
	set_value(strToDouble(str));
}

std::string SerialisableAdjustment::exportToString() const
{
	return doubleToStr(get_value());
}

SerialisableAdjustmentWrapper::SerialisableAdjustmentWrapper(Gtk::Adjustment* adjustment) :
	_adjustment(adjustment)
{}

void SerialisableAdjustmentWrapper::importFromString(const std::string& str)
{
	_adjustment->set_value(strToDouble(str));
}

std::string SerialisableAdjustmentWrapper::exportToString() const
{
	return doubleToStr(_adjustment->get_value());
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
	set_value(strToDouble(str));
}

std::string SerialisableSpinButton::exportToString() const
{
	return doubleToStr(get_value());
}

SerialisableSpinButtonWrapper::SerialisableSpinButtonWrapper(Gtk::SpinButton* spin) :
	_spin(spin)
{}

void SerialisableSpinButtonWrapper::importFromString(const std::string& str)
{
	_spin->set_value(strToDouble(str));
}

std::string SerialisableSpinButtonWrapper::exportToString() const
{
	return doubleToStr(_spin->get_value());
}

// Scale widget

SerialisableScaleWidget::SerialisableScaleWidget() : 
	Gtk::Range()
{}

void SerialisableScaleWidget::importFromString(const std::string& str)
{
	set_value(strToDouble(str));
}

std::string SerialisableScaleWidget::exportToString() const
{
	return doubleToStr(get_value());
}

SerialisableScaleWidgetWrapper::SerialisableScaleWidgetWrapper(Gtk::Range* range) :
	_range(range)
{}

void SerialisableScaleWidgetWrapper::importFromString(const std::string& str)
{
	_range->set_value(strToDouble(str));
}

std::string SerialisableScaleWidgetWrapper::exportToString() const
{
	return doubleToStr(_range->get_value());
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

// SerialisableComboBox_Index

SerialisableComboBox_Index::SerialisableComboBox_Index() :
	SerialisableComboBox()
{}

void SerialisableComboBox_Index::importFromString(const std::string& str)
{
	int activeId = strToInt(str);
	set_active(activeId);

	int newId = get_active();

	if (activeId != newId)
	{
		std::cerr << "SerialisableComboBox_Index::importFromString(): "
				<< "warning: requested index " << activeId 
				<< " was not set, current index is " << newId << std::endl;
	}
}

std::string SerialisableComboBox_Index::exportToString() const
{
	return intToStr(get_active());
}

SerialisableComboBox_IndexWrapper::SerialisableComboBox_IndexWrapper(Gtk::ComboBoxText* combo) :
	_combo(combo)
{}

void SerialisableComboBox_IndexWrapper::importFromString(const std::string& str)
{
	int activeId = strToInt(str);
	_combo->set_active(activeId);

	int newId = _combo->get_active();

	if (activeId != newId)
	{
		std::cerr << "SerialisableComboBox_Index::importFromString(): "
				<< "warning: requested index " << activeId 
				<< " was not set, current index is " << newId << std::endl;
	}
}

std::string SerialisableComboBox_IndexWrapper::exportToString() const
{
	return intToStr(_combo->get_active());
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
