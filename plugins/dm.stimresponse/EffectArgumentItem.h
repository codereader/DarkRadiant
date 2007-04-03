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
	GtkWidget* _label;
	GtkWidget* _desc;

public:
	EffectArgumentItem(ResponseEffect::Argument& arg) :
		_arg(arg)
	{
		_label = gtkutil::LeftAlignedLabel(_arg.title + ":");
		
		_desc = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(_desc), "<b>?</b>");
	}
	
	/** greebo: This retrieves the string representation of the
	 * 			current value of this row. This has to be
	 * 			implemented by the derived classes.
	 */
	virtual std::string getValue() = 0;
	
	// Retrieve the label widget
	virtual GtkWidget* getLabelWidget() {
		return _label;
	}
	
	// Retrieve the edit widgets (abstract)
	virtual GtkWidget* getEditWidget() = 0;
	
	virtual GtkWidget* getHelpWidget() {
		return _desc;
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
	StringArgument(ResponseEffect::Argument& arg) :
		EffectArgumentItem(arg)
	{
		_entry = gtk_entry_new(); 
	}
	
	virtual GtkWidget* getEditWidget() {
		return _entry;
	}
	
	virtual std::string getValue() {
		return "string";
	}
};

/** greebo: This is an item querying a float (derives from string)
 */
class FloatArgument :
	public StringArgument
{
public:
	FloatArgument(ResponseEffect::Argument& arg) :
		StringArgument(arg)
	{}
};

/** greebo: This is an item querying a vector (derives from string)
 */
class VectorArgument :
	public StringArgument
{
public:
	VectorArgument(ResponseEffect::Argument& arg) :
		StringArgument(arg)
	{}
};

/** greebo: This is an item querying an entity name (entry/dropdown combo)
 */
class EntityArgument :
	public EffectArgumentItem
{
public:
	EntityArgument(ResponseEffect::Argument& arg) :
		EffectArgumentItem(arg)
	{}
	
	std::string getValue() {
		return "TestEntity";
	}
	
	virtual GtkWidget* getEditWidget() {
		return NULL;
	}
};

#endif /*EFFECTARGUMENTITEM_H_*/
