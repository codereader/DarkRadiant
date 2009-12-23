#ifndef _DIALOG_ELEMENTS_H_
#define _DIALOG_ELEMENTS_H_

#include "idialogmanager.h"
#include "../ifc/Widget.h"
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
 * The getWidget() method retrieves the widget carrying the actual value,
 * use the getLabel() method to retrieve the descriptive label carrying the title.
 */
class DialogElement :
	public StringSerialisable,
	public Widget
{
protected:
	GtkWidget* _label;

	// The widget carrying the value
	GtkWidget* _widget;

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
		_label(LeftAlignedLabel(label)),
		_widget(NULL)
	{}

public:
	// Retrieve the label
	virtual GtkWidget* getLabel() const
	{
		return _label;
	}

protected:
	// Widget implementation
	virtual GtkWidget* _getWidget() const
	{
		return _widget;
	}

	// Subclasses needed to call this to allow _getWidget() to work
	void setWidget(GtkWidget* widget)
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
		SerialisableTextEntry(gtk_entry_new())
	{
		DialogElement::setWidget(SerialisableTextEntry::getWidget());
	}

	// Implementation of StringSerialisable, wrapping to base class 
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
	DialogSpinButton(const std::string& label, double min, double max, double step) :
		DialogElement(label),
		SerialisableSpinButton(gtk_spin_button_new_with_range(min, max, step))
	{
		DialogElement::setWidget(SerialisableSpinButton::getWidget());
	}

	// Implementation of StringSerialisable, wrapping to base class 
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
		DialogElement::setWidget(PathEntry::getWidget());
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
	public SerialisableToggleButton
{
public:
	DialogCheckBox(const std::string& label) :
		DialogElement(""), // empty label, the description is included in the toggle button
		SerialisableToggleButton(gtk_check_button_new_with_label(label.c_str()))
	{
		DialogElement::setWidget(SerialisableToggleButton::getWidget());
	}

	// Implementation of StringSerialisable, wrapping to base class 
	virtual std::string exportToString() const 
	{
		return SerialisableToggleButton::exportToString();
	}

	virtual void importFromString(const std::string& str)
	{
		SerialisableToggleButton::importFromString(str);
	}
};

// -----------------------------------------------------------------------

class DialogLabel :
	public DialogElement
{
protected:
	GtkWidget* _label;
public:
	DialogLabel(const std::string& label) :
		DialogElement(), // no standard label
		_label(gtkutil::LeftAlignedLabel(label))
	{
		DialogElement::setWidget(_label);
	}

	// Implementation of StringSerialisable, wrapping to base class 
	virtual std::string exportToString() const 
	{
		return gtk_label_get_text(GTK_LABEL(_label));
	}

	virtual void importFromString(const std::string& str)
	{
		gtk_label_set_markup(GTK_LABEL(_label), str.c_str());
	}
};

// -----------------------------------------------------------------------

/**
 * Creates a new GTK ComboBox carrying the values passed to the constructor
 * Complies with the DialogElement interface.
 * To avoid _getWidget() conflicts in the public interface, the SerialisableComboBox_Text
 * is inherited in a private fashion.
 */
class DialogComboBox :
	public DialogElement,
	private SerialisableComboBox_Text
{
public:
	DialogComboBox(const std::string& label, const ui::IDialog::ComboBoxOptions& options) :
		DialogElement(label)
	{
		// Retrieve the widget from the private base class
		GtkWidget* comboBox = SerialisableComboBox_Text::_getWidget();

		// Pass the widget to the DialogElement base class
		DialogElement::setWidget(comboBox);

		// Add the options to the combo box
		for (ui::IDialog::ComboBoxOptions::const_iterator i = options.begin();
			 i != options.end(); ++i)
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), i->c_str());
		}
	}

	// Implementation of StringSerialisable, wrapping to private 
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
