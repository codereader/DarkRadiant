#ifndef EFFECTARGUMENTITEM_H_
#define EFFECTARGUMENTITEM_H_

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "ResponseEffect.h"

class EffectArgumentItem
{
protected:
	// The argument this row is referring to
	ResponseEffect::Argument& _arg;
	GtkWidget* _labelBox;
	GtkWidget* _descBox;
	GtkTooltips* _tooltips;

public:
	EffectArgumentItem(ResponseEffect::Argument& arg, GtkTooltips* tooltips) :
		_arg(arg),
		_tooltips(tooltips)
	{
		// Pack the label into a eventbox
		_labelBox = gtk_event_box_new();
		GtkWidget* label = gtkutil::LeftAlignedLabel(_arg.title + ":");
		gtk_container_add(GTK_CONTAINER(_labelBox), label);
		
		gtk_tooltips_set_tip(_tooltips, _labelBox, arg.desc.c_str(), "");
		
		// Pack the description widget into a eventbox		
		_descBox = gtk_event_box_new();
		GtkWidget* descLabel = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(descLabel), "<b>?</b>");
		gtk_container_add(GTK_CONTAINER(_descBox), descLabel);
		
		gtk_tooltips_set_tip(_tooltips, _descBox, arg.desc.c_str(), "");
	}
	
	/** greebo: This retrieves the string representation of the
	 * 			current value of this row. This has to be
	 * 			implemented by the derived classes.
	 */
	virtual std::string getValue() = 0;
	
	// Retrieve the label widget
	virtual GtkWidget* getLabelWidget() {
		return _labelBox;
	}
	
	// Retrieve the edit widgets (abstract)
	virtual GtkWidget* getEditWidget() = 0;
	
	virtual GtkWidget* getHelpWidget() {
		return _descBox;
	}
	
	/** greebo: This saves the value to the according response effect.
	 */
	virtual void save() {
		// Save the value to the effect 
		_arg.value = getValue();
	}
};

/** greebo: This is an item querying a simple string
 */
class StringArgument :
	public EffectArgumentItem
{
protected:
	GtkWidget* _entry;

public:
	StringArgument(ResponseEffect::Argument& arg, GtkTooltips* tooltips) :
		EffectArgumentItem(arg, tooltips)
	{
		_entry = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(_entry), arg.value.c_str()); 
	}
	
	virtual GtkWidget* getEditWidget() {
		return _entry;
	}
	
	virtual std::string getValue() {
		return gtk_entry_get_text(GTK_ENTRY(_entry));
	}
};

/** greebo: This is an item querying a float (derives from string)
 */
class FloatArgument :
	public StringArgument
{
public:
	FloatArgument(ResponseEffect::Argument& arg, GtkTooltips* tooltips) :
		StringArgument(arg, tooltips)
	{}
};

/** greebo: This is an item querying a vector (derives from string)
 */
class VectorArgument :
	public StringArgument
{
public:
	VectorArgument(ResponseEffect::Argument& arg, GtkTooltips* tooltips) :
		StringArgument(arg, tooltips)
	{}
};

/** greebo: This is an item querying an entity name (entry/dropdown combo)
 */
class EntityArgument :
	public EffectArgumentItem
{
public:
	EntityArgument(ResponseEffect::Argument& arg, GtkTooltips* tooltips) :
		EffectArgumentItem(arg, tooltips)
	{}
	
	std::string getValue() {
		return "TestEntity";
	}
	
	virtual GtkWidget* getEditWidget() {
		return gtk_entry_new();
	}
};

#endif /*EFFECTARGUMENTITEM_H_*/
