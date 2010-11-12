#ifndef _DIALOG_ELEMENTS_H_
#define _DIALOG_ELEMENTS_H_

#include "idialogmanager.h"
#include "../SerialisableWidgets.h"
#include "../LeftAlignedLabel.h"
#include "../PathEntry.h"

#include <gtk/gtkcombobox.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkcheckbutton.h>

namespace gtkutil
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
	Gtk::Label* _label;

	// The widget carrying the value
	Gtk::Widget* _widget;

protected:
	/**
	 * Protected constructor, to be called by subclasses
	 * Creates an element without a label.
	 */
	DialogElement() :
		_label(NULL),
		_widget(NULL)
	{}

	/**
	 * Protected constructor, to be called by subclasses
	 * @label: the name of this element, to be displayed next to it.
	 */
	DialogElement(const std::string& label) :
		_label(Gtk::manage(new LeftAlignedLabel(label))),
		_widget(NULL)
	{}

public:
	// Retrieve the label
	virtual Gtk::Label* getLabel() const
	{
		return _label;
	}

	// Retrieve the widget carrying the value
	virtual Gtk::Widget* getValueWidget() const
	{
		return _widget;
	}

protected:
	void setValueWidget(Gtk::Widget* widget)
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
	DialogEntryBox(const std::string& label) :
		DialogElement(label),
		SerialisableTextEntry()
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
	DialogSpinButton(const std::string& label, double min, double max, double step, unsigned int digits) :
		DialogElement(label),
		SerialisableSpinButton(min, min, max, step, static_cast<guint>(digits))
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
	DialogPathEntry(const std::string& label, bool foldersOnly) :
		PathEntry(foldersOnly),
		DialogElement(label)
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
	DialogCheckBox(const std::string& label) :
		DialogElement(""), // empty label, the description is included in the toggle button
		SerialisableCheckButton(label)
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
	public Gtk::Label
{
public:
	DialogLabel(const std::string& label) :
		DialogElement(), // no standard label
		Gtk::Label(label)
	{
		set_alignment(0.0f, 0.5f);
		DialogElement::setValueWidget(this);
	}

	// Implementation of StringSerialisable
	virtual std::string exportToString() const
	{
		return get_text();
	}

	virtual void importFromString(const std::string& str)
	{
		set_markup(str);
	}
};

// -----------------------------------------------------------------------

/**
 * Creates a new GTK ComboBox carrying the values passed to the constructor
 * Complies with the DialogElement interface.
 */
class DialogComboBox :
	public DialogElement,
	public SerialisableComboBox_Text
{
public:
	DialogComboBox(const std::string& label, const ui::IDialog::ComboBoxOptions& options) :
		DialogElement(label)
	{
		// Pass ourselves as widget to the DialogElement base class
		DialogElement::setValueWidget(this);

		// Add the options to the combo box
		for (ui::IDialog::ComboBoxOptions::const_iterator i = options.begin();
			 i != options.end(); ++i)
		{
			append_text(*i);
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
typedef boost::shared_ptr<DialogComboBox> DialogComboBoxPtr;

} // namespace gtkutil

#endif /* _DIALOG_ELEMENTS_H_ */
