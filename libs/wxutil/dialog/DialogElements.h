#pragma once

#include "idialogmanager.h"
#include "../SerialisableWidgets.h"
#include "../PathEntry.h"

#include <memory>
#include <wx/stattext.h>

namespace wxutil
{

/**
 * greebo: Each dialog element has a label and a string-serialisable value
 * The getValueWidget() method retrieves the widget carrying the actual value,
 * use the getLabel() method to retrieve the descriptive label carrying the title.
 */
class DialogElement :
	public StringSerialisable
{
protected:
	// The label
	wxStaticText* _label;

	// The widget carrying the value
	wxWindow* _widget;

protected:
	/**
	 * Protected constructor, to be called by subclasses
	 * @label: the name of this element, to be displayed next to it.
	 */
	DialogElement(wxWindow* parent, const std::string& label) :
		_label(new wxStaticText(parent, wxID_ANY, label)),
		_widget(NULL)
	{}

public:
	// Retrieve the label
	virtual wxStaticText* getLabel() const
	{
		return _label;
	}

	// Retrieve the widget carrying the value
	virtual wxWindow* getValueWidget() const
	{
		return _widget;
	}

protected:
	void setValueWidget(wxWindow* widget)
	{
		_widget = widget;
	}
};

// -----------------------------------------------------------------------

class DialogEntryBox :
	public DialogElement,
	public SerialisableTextEntry
{
public:
	DialogEntryBox(wxWindow* parent, const std::string& label) :
		DialogElement(parent, label),
		SerialisableTextEntry(parent)
	{
		setValueWidget(this); // this as SerialisableTextEntry
	}

	virtual std::string exportToString() const
	{
		return SerialisableTextEntry::exportToString();
	}

	virtual void importFromString(const std::string& str)
	{
		SerialisableTextEntry::importFromString(str);
	}
};

// -----------------------------------------------------------------------

class DialogSpinButton :
	public DialogElement,
	public SerialisableSpinButton
{
public:
	DialogSpinButton(wxWindow* parent, const std::string& label, double min, double max, double step, unsigned int digits) :
		DialogElement(parent, label),
		SerialisableSpinButton(parent, min, min, max, step, digits)
	{
		DialogElement::setValueWidget(this);
	}

	virtual std::string exportToString() const
	{
		return SerialisableSpinButton::exportToString();
	}

	virtual void importFromString(const std::string& str)
	{
		SerialisableSpinButton::importFromString(str);
	}
};

// -----------------------------------------------------------------------

class DialogPathEntry :
	public PathEntry,
	public DialogElement
{
public:
	DialogPathEntry(wxWindow* parent, const std::string& label, bool foldersOnly) :
		PathEntry(parent, foldersOnly),
		DialogElement(parent, label)
	{
		DialogElement::setValueWidget(this); // this as *PathEntry
	}

	// Implementation of StringSerialisable, wrapping to base class
	virtual std::string exportToString() const
	{
		return PathEntry::getValue();
	}

	virtual void importFromString(const std::string& str)
	{
		PathEntry::setValue(str);
	}
};

// -----------------------------------------------------------------------

class DialogCheckBox :
	public DialogElement,
	public SerialisableCheckButton
{
public:
	DialogCheckBox(wxWindow* parent, const std::string& label) :
		DialogElement(parent, ""), // empty label, the description is included in the toggle button
		SerialisableCheckButton(parent, label)
	{
		DialogElement::setValueWidget(this); // this as SerialisableCheckButton
	}

	virtual std::string exportToString() const
	{
		return SerialisableCheckButton::exportToString();
	}

	virtual void importFromString(const std::string& str)
	{
		SerialisableCheckButton::importFromString(str);
	}
};

// -----------------------------------------------------------------------

class DialogLabel :
	public DialogElement,
	public wxStaticText
{
public:
	DialogLabel(wxWindow* parent, const std::string& label) :
		DialogElement(parent, label), // no standard label
		wxStaticText(parent, wxID_ANY, label)
	{
		DialogElement::setValueWidget(this);
	}

	// Implementation of StringSerialisable
	virtual std::string exportToString() const
	{
		return GetLabel().ToStdString();
	}

	virtual void importFromString(const std::string& str)
	{
		SetLabel(str);
	}
};

// -----------------------------------------------------------------------

/**
 * Creates a new ComboBox carrying the values passed to the constructor
 * Complies with the DialogElement interface.
 */
class DialogComboBox :
	public DialogElement,
	public SerialisableComboBox_Text
{
public:
	DialogComboBox(wxWindow* parent, const std::string& label, const ui::IDialog::ComboBoxOptions& options) :
		DialogElement(parent, label),
		SerialisableComboBox_Text(parent)
	{
		// Pass ourselves as widget to the DialogElement base class
		DialogElement::setValueWidget(this);

		// Add the options to the combo box
		for (ui::IDialog::ComboBoxOptions::const_iterator i = options.begin();
			 i != options.end(); ++i)
		{
			this->Append(*i);
		}
	}

	virtual std::string exportToString() const
	{
		return SerialisableComboBox_Text::exportToString();
	}

	virtual void importFromString(const std::string& str)
	{
		SerialisableComboBox_Text::importFromString(str);
	}
};
typedef std::shared_ptr<DialogComboBox> DialogComboBoxPtr;

} // namespace wxutil
