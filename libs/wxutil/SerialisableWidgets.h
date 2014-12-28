#pragma once

#include <StringSerialisable.h>

#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/tglbtn.h>
#include <wx/checkbox.h>
#include <wx/choice.h>

namespace wxutil
{

/*
 * wxTextCtrl object which implements StringSerialisable.
 */
class SerialisableTextEntry :
	public wxTextCtrl,
	public StringSerialisable
{
public:
	// Main constructor
	SerialisableTextEntry(wxWindow* parent);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableTextEntry> SerialisableTextEntryPtr;

// Wrapper class to make existing wxTextCtrls serialisable
class SerialisableTextEntryWrapper :
	public StringSerialisable
{
private:
	wxTextCtrl* _entry;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableTextEntryWrapper(wxTextCtrl* entry);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableTextEntryWrapper> SerialisableTextEntryWrapperPtr;

/**
 * wxSpinCtrlDouble object which implements StringSerialisable.
 */
class SerialisableSpinButton :
	public wxSpinCtrlDouble,
	public StringSerialisable
{
public:
	// Main constructor
	SerialisableSpinButton(wxWindow* parent, double value,
						   double min, double max,
						   double step, unsigned int digits);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableSpinButton> SerialisableSpinButtonPtr;

// Wrapper class to make an existing wxSpinCtrlDouble serialisable
class SerialisableSpinButtonWrapper :
	public StringSerialisable
{
private:
	wxSpinCtrlDouble* _spin;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableSpinButtonWrapper(wxSpinCtrlDouble* spin);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableSpinButtonWrapper> SerialisableSpinButtonWrapperPtr;

/**
 * wxToggleButton object which implements StringSerialisable.
 */
class SerialisableToggleButton :
	public wxToggleButton,
	public StringSerialisable
{
public:
	// Main constructors
	SerialisableToggleButton(wxWindow* parent);
	SerialisableToggleButton(wxWindow* parent, const std::string& label);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableToggleButton> SerialisableToggleButtonPtr;

// Wrapper class to make existing wxToggleButton serialisable
class SerialisableToggleButtonWrapper :
	public StringSerialisable
{
private:
	wxToggleButton* _button;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableToggleButtonWrapper(wxToggleButton* button);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableToggleButtonWrapper> SerialisableToggleButtonWrapperPtr;

/**
 * wxCheckBox object which implements StringSerialisable.
 */
class SerialisableCheckButton :
	public wxCheckBox,
	public StringSerialisable
{
public:
	// Main constructors
	SerialisableCheckButton(wxWindow* parent);
	SerialisableCheckButton(wxWindow* parent, const std::string& label);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableCheckButton> SerialisableCheckButtonPtr;

// Wrapper class to make existing wxCheckBox serialisable
class SerialisableCheckButtonWrapper :
	public StringSerialisable
{
private:
	wxCheckBox* _button;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableCheckButtonWrapper(wxCheckBox* button);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableCheckButtonWrapper> SerialisableCheckButtonWrapperPtr;

// Base class for serialisable combo boxes (text or index)
class SerialisableComboBox :
	public wxChoice,
	public StringSerialisable
{
public:
	SerialisableComboBox(wxWindow* parent) :
		wxChoice(parent, wxID_ANY)
	{}
};
typedef std::shared_ptr<SerialisableComboBox> SerialisableComboBoxPtr;

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
	SerialisableComboBox_Index(wxWindow* parent);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableComboBox_Index> SerialisableComboBox_IndexPtr;

// Wrapper class to make existing Gtk::ComboBoxText serialisable
class SerialisableComboBox_IndexWrapper :
	public StringSerialisable
{
private:
	wxChoice* _combo;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableComboBox_IndexWrapper(wxChoice* combo);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableComboBox_IndexWrapper> SerialisableComboBox_IndexWrapperPtr;

/**
 * \brief
 * Serialisable combo box which saves the selected value as a text string.
 */
class SerialisableComboBox_Text :
	public SerialisableComboBox
{
public:
	/* Main constructor */
	SerialisableComboBox_Text(wxWindow* parent);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableComboBox_Text> SerialisableComboBox_TextPtr;

// Wrapper class to make existing Gtk::ComboBoxText serialisable
class SerialisableComboBox_TextWrapper :
	public StringSerialisable
{
private:
	wxChoice* _combo;
public:
	// Main constructor, wrapping around an existing widget
	SerialisableComboBox_TextWrapper(wxChoice* combo);

	/* StringSerialisable implementation */
	void importFromString(const std::string& str);
	std::string exportToString() const;
};
typedef std::shared_ptr<SerialisableComboBox_TextWrapper> SerialisableComboBox_TextWrapperPtr;

} // namespace
