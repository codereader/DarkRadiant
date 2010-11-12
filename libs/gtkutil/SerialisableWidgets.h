#pragma once

#include <StringSerialisable.h>

#include <gtkmm/adjustment.h>
#include <gtkmm/entry.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/range.h>

namespace gtkutil
{

/**
 * StringSerialisable Gtk::Adjustment class.
 */
class SerialisableAdjustment :
	public Gtk::Adjustment,
	public StringSerialisable
{
public:
	// Main constructor
	SerialisableAdjustment(double value, double lower, double upper);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableAdjustment> SerialisableAdjustmentPtr;

// Wrapper class to make existing Gtk::Entries serialisable
class SerialisableAdjustmentWrapper :
	public StringSerialisable
{
private:
	Gtk::Adjustment* _adjustment;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableAdjustmentWrapper(Gtk::Adjustment* adjustment);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableAdjustmentWrapper> SerialisableAdjustmentWrapperPtr;

/*
 * Gtk::Entry object which implements StringSerialisable.
 */
class SerialisableTextEntry :
	public Gtk::Entry,
	public StringSerialisable
{
public:
	// Main constructor
	SerialisableTextEntry();

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableTextEntry> SerialisableTextEntryPtr;

// Wrapper class to make existing Gtk::Entries serialisable
class SerialisableTextEntryWrapper :
	public StringSerialisable
{
private:
	Gtk::Entry* _entry;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableTextEntryWrapper(Gtk::Entry* entry);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableTextEntryWrapper> SerialisableTextEntryWrapperPtr;

/**
 * Gtk::SpinButton object which implements StringSerialisable.
 */
class SerialisableSpinButton :
	public Gtk::SpinButton,
	public StringSerialisable
{
public:
	// Main constructor
	SerialisableSpinButton(double value,
						   double min, double max,
						   double step, guint digits);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableSpinButton> SerialisableSpinButtonPtr;

// Wrapper class to make an existing Gtk::SpinButton serialisable
class SerialisableSpinButtonWrapper :
	public StringSerialisable
{
private:
	Gtk::SpinButton* _spin;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableSpinButtonWrapper(Gtk::SpinButton* spin);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableSpinButtonWrapper> SerialisableSpinButtonWrapperPtr;

/**
 * Gtk::Range object which implements StringSerialisable.
 */
class SerialisableScaleWidget :
	public Gtk::Range,
	public StringSerialisable
{
public:
	// Main constructor
	SerialisableScaleWidget();

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableScaleWidget> SerialisableScaleWidgetPtr;

// Wrapper class to make existing Gtk::Range serialisable
class SerialisableScaleWidgetWrapper :
	public StringSerialisable
{
private:
	Gtk::Range* _range;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableScaleWidgetWrapper(Gtk::Range* range);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableScaleWidgetWrapper> SerialisableScaleWidgetWrapperPtr;

/**
 * Gtk::ToggleButton object which implements StringSerialisable.
 */
class SerialisableToggleButton :
	public Gtk::ToggleButton,
	public StringSerialisable
{
public:
	// Main constructors
	SerialisableToggleButton();
	SerialisableToggleButton(const std::string& label);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableToggleButton> SerialisableToggleButtonPtr;

// Wrapper class to make existing Gtk::ToggleButton serialisable
class SerialisableToggleButtonWrapper :
	public StringSerialisable
{
private:
	Gtk::ToggleButton* _button;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableToggleButtonWrapper(Gtk::ToggleButton* button);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableToggleButtonWrapper> SerialisableToggleButtonWrapperPtr;

/**
 * Gtk::CheckButton object which implements StringSerialisable.
 */
class SerialisableCheckButton :
	public Gtk::CheckButton,
	public StringSerialisable
{
public:
	// Main constructors
	SerialisableCheckButton();
	SerialisableCheckButton(const std::string& label);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableCheckButton> SerialisableCheckButtonPtr;

// Wrapper class to make existing Gtk::CheckButton serialisable
class SerialisableCheckButtonWrapper :
	public StringSerialisable
{
private:
	Gtk::CheckButton* _button;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableCheckButtonWrapper(Gtk::CheckButton* button);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableCheckButtonWrapper> SerialisableCheckButtonWrapperPtr;

// Base class for serialisable combo boxes (text or index)
class SerialisableComboBox :
	public Gtk::ComboBoxText,
	public StringSerialisable
{
public:
	SerialisableComboBox() :
		Gtk::ComboBoxText()
	{}
};
typedef boost::shared_ptr<SerialisableComboBox> SerialisableComboBoxPtr;

/**
 * \brief
 * StringSerialisable combo box (text values) which saves the
 * selected value as a numeric index.
 */
class SerialisableComboBox_Index :
	public SerialisableComboBox
{
public:
	/* Main constructor */
	SerialisableComboBox_Index();

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableComboBox_Index> SerialisableComboBox_IndexPtr;

// Wrapper class to make existing Gtk::ComboBoxText serialisable
class SerialisableComboBox_IndexWrapper :
	public StringSerialisable
{
private:
	Gtk::ComboBoxText* _combo;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableComboBox_IndexWrapper(Gtk::ComboBoxText* combo);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableComboBox_IndexWrapper> SerialisableComboBox_IndexWrapperPtr;

/**
 * \brief
 * Serialisable combo box which saves the selected value as a text string.
 */
class SerialisableComboBox_Text :
	public SerialisableComboBox
{
public:
	/* Main constructor */
	SerialisableComboBox_Text();

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableComboBox_Text> SerialisableComboBox_TextPtr;

// Wrapper class to make existing Gtk::ComboBoxText serialisable
class SerialisableComboBox_TextWrapper :
	public StringSerialisable
{
private:
	Gtk::ComboBoxText* _combo;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableComboBox_TextWrapper(Gtk::ComboBoxText* combo);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef boost::shared_ptr<SerialisableComboBox_TextWrapper> SerialisableComboBox_TextWrapperPtr;

} // namespace
