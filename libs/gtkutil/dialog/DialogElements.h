#ifndef _DIALOG_ELEMENTS_H_
#define _DIALOG_ELEMENTS_H_

#include "../ifc/Widget.h"
#include "../SerialisableWidgets.h"

#include <gtk/gtklabel.h>
#include <gtk/gtkcombobox.h>

#include <boost/enable_shared_from_this.hpp>

namespace gtkutil
{

/**
 * greebo: Each dialog element has a label and a string-serialisable value
 * The getWidget() method retrieves the widget carrying the actual value,
 * use the getLabel() method to retrieve the descriptive label carrying the title.
 */
class DialogElement :
	public StringSerialisable,
	public virtual Widget
{
protected:
	GtkWidget* _label;

	// The widget carrying the value
	GtkWidget* _widget;

	// Each subclass is responsible of setting this member to a
	// StringSerialisable object wrapping the value _widget.
	StringSerialisablePtr _serialisable;

protected:
	// Protected constructor, to be called by subclasses
	DialogElement(const std::string& label) :
		_label(gtk_label_new("")),
		_widget(NULL)
	{
		gtk_label_set_markup(GTK_LABEL(_label), label.c_str());
	}

public:
	// Retrieve the label
	virtual GtkWidget* getLabel() const
	{
		return _label;
	}

	// Implement the string serialisable interface, wrapping to the _serialisable member
	virtual std::string exportToString() const 
	{
		return (_serialisable != NULL) ? _serialisable->exportToString() : "";
	}

	virtual void importFromString(const std::string& str)
	{
		if (_serialisable != NULL) 
		{
			_serialisable->importFromString(str);
		}
	}

protected:
	// Widget implementation
	virtual GtkWidget* _getWidget() const
	{
		return _widget;
	}

	void setWidget(GtkWidget* widget)
	{
		_widget = widget;
	}
};

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

	// Override DialogElement implementation of StringSerialisable, wrapping to private 
	virtual std::string exportToString() const 
	{
		return SerialisableComboBox_Text::exportToString();
	}

	virtual void importFromString(const std::string& str)
	{
		SerialisableComboBox_Text::importFromString(str);
	}

protected:
   virtual GtkWidget* _getWidget() const
   {
	   // Wrap to private implementation
	   return SerialisableComboBox_Text::_getWidget();
   }
};
typedef boost::shared_ptr<DialogComboBox> DialogComboBoxPtr;

} // namespace gtkutil

#endif /* _DIALOG_ELEMENTS_H_ */
