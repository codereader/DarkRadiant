#ifndef EFFECTARGUMENTITEM_H_
#define EFFECTARGUMENTITEM_H_

#include "ResponseEffect.h"
#include <gtkmm/liststore.h>

class StimTypes;

namespace Gtk
{
	class EventBox;
	class ComboBoxEntry;
	class CheckButton;
	class Entry;
	class ComboBox;
	class Widget;
}

class EffectArgumentItem
{
protected:
	// The argument this row is referring to
	ResponseEffect::Argument& _arg;

	Gtk::EventBox* _labelBox;
	Gtk::EventBox* _descBox;

public:
	EffectArgumentItem(ResponseEffect::Argument& arg);

	// destructor
	virtual ~EffectArgumentItem() {}

	/** greebo: This retrieves the string representation of the
	 * 			current value of this row. This has to be
	 * 			implemented by the derived classes.
	 */
	virtual std::string getValue() = 0;

	// Retrieve the label widget
	virtual Gtk::Widget& getLabelWidget();

	// Retrieve the edit widgets (abstract)
	virtual Gtk::Widget& getEditWidget() = 0;

	// Retrieves the help widget (a question mark with a tooltip)
	virtual Gtk::Widget& getHelpWidget();

	/** greebo: This saves the value to the according response effect.
	 */
	virtual void save();
};

/** greebo: This is an item querying a simple string
 */
class StringArgument :
	public EffectArgumentItem
{
protected:
	Gtk::Entry* _entry;

public:
	StringArgument(ResponseEffect::Argument& arg);

	virtual Gtk::Widget& getEditWidget();
	virtual std::string getValue();
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

class BooleanArgument :
	public EffectArgumentItem
{
	Gtk::CheckButton* _checkButton;
public:
	BooleanArgument(ResponseEffect::Argument& arg);

	virtual Gtk::Widget& getEditWidget();
	virtual std::string getValue();
};

/** greebo: This is an item querying an entity name (entry/dropdown combo)
 */
class EntityArgument :
	public EffectArgumentItem
{
	const Glib::RefPtr<Gtk::ListStore>& _entityStore;
	Gtk::ComboBoxEntry* _comboBox;
public:
	// Pass the entity liststore to this item so that the auto-completion
	// of the entity combo box works correctly
	EntityArgument(ResponseEffect::Argument& arg,
				   const Glib::RefPtr<Gtk::ListStore>& entityStore);

	virtual Gtk::Widget& getEditWidget();
	virtual std::string getValue();
};

/** greebo: This is an item querying an stimtype (dropdown combo)
 */
class StimTypeArgument :
	public EffectArgumentItem
{
private:
	const StimTypes& _stimTypes;
	Gtk::ComboBox* _comboBox;
public:
	// Pass the reference to the StimType helper class
	StimTypeArgument(ResponseEffect::Argument& arg,
				     const StimTypes& stimTypes);

	virtual Gtk::Widget& getEditWidget();
	virtual std::string getValue();
};

#endif /*EFFECTARGUMENTITEM_H_*/
