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

public:
	EffectArgumentItem(ResponseEffect::Argument& arg) :
		_arg(arg)
	{}
	
	/** greebo: This retrieves the string representation of the
	 * 			current value of this row. This has to be
	 * 			implemented by the derived classes.
	 */
	virtual std::string getValue() = 0;
	
	/** greebo: Cast this item onto a GtkWidget* so that it can 
	 * 			be packed into a parent container. 
	 */
	virtual operator GtkWidget*() = 0;
	
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
	GtkWidget* _entry;
	GtkWidget* _hbox;
public:
	StringArgument(ResponseEffect::Argument& arg) :
		EffectArgumentItem(arg)
	{
		GtkWidget* label = gtkutil::LeftAlignedLabel(_arg.title);
		_entry = gtk_entry_new();
		
		_hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(_hbox), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(_hbox), 
			gtkutil::LeftAlignment(_entry, 12, 1.0f), 
			TRUE, TRUE, 0
		);
	}
	
	virtual operator GtkWidget*() {
		return _hbox;
	}
	
	virtual std::string getValue() {
		return "string";
	}
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
	
	virtual operator GtkWidget*() {
		return NULL;
	}
};

#endif /*EFFECTARGUMENTITEM_H_*/
