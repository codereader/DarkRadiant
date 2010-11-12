#ifndef COMMAND_ARGUMENT_ITEM_H_
#define COMMAND_ARGUMENT_ITEM_H_

#include "ConversationCommandInfo.h"
#include <boost/shared_ptr.hpp>

#include <gtkmm/liststore.h>

namespace Gtk
{
	class Widget;
	class EventBox;
	class Entry;
	class CheckButton;
	class ComboBox;
}

namespace ui
{

// Actor Treemodel definition
struct ActorColumns :
	public Gtk::TreeModel::ColumnRecord
{
	ActorColumns() { add(actorNumber); add(caption); }

	Gtk::TreeModelColumn<int> actorNumber;
	Gtk::TreeModelColumn<Glib::ustring> caption;
};

class CommandArgumentItem
{
protected:
	// The argument this row is referring to
	const conversation::ArgumentInfo& _argInfo;
	Gtk::EventBox* _labelBox;
	Gtk::EventBox* _descBox;

public:
	CommandArgumentItem(const conversation::ArgumentInfo& argInfo);

	// destructor
	virtual ~CommandArgumentItem() {}

	/**
	 * greebo: This retrieves the string representation of the current
	 * value of this row. This has to be implemented by the subclasses.
	 */
	virtual std::string getValue() = 0;

	/**
	 * greebo: Loads the given value into the widgets, passed in string form.
	 */
	virtual void setValueFromString(const std::string& value) = 0;

	// Retrieve the label widget
	virtual Gtk::Widget& getLabelWidget();

	// Retrieve the edit widgets (abstract)
	virtual Gtk::Widget& getEditWidget() = 0;

	// Retrieves the help widget (a question mark with a tooltip)
	virtual Gtk::Widget& getHelpWidget();
};
typedef boost::shared_ptr<CommandArgumentItem> CommandArgumentItemPtr;

/**
 * greebo: This is an item querying a simple string
 */
class StringArgument :
	public CommandArgumentItem
{
protected:
	Gtk::Entry* _entry;

public:
	StringArgument(const conversation::ArgumentInfo& argInfo);

	virtual Gtk::Widget& getEditWidget();
	virtual std::string getValue();
	virtual void setValueFromString(const std::string& value);
};

/**
 * greebo: This is an item querying a float (derives from string)
 */
class FloatArgument :
	public StringArgument
{
public:
	FloatArgument(const conversation::ArgumentInfo& argInfo) :
		StringArgument(argInfo)
	{}
};

/**
 * greebo: This is an item querying a vector (derives from string)
 */
class VectorArgument :
	public StringArgument
{
public:
	VectorArgument(const conversation::ArgumentInfo& argInfo) :
		StringArgument(argInfo)
	{}
};

class BooleanArgument :
	public CommandArgumentItem
{
protected:
	Gtk::CheckButton* _checkButton;
public:
	BooleanArgument(const conversation::ArgumentInfo& argInfo);

	virtual Gtk::Widget& getEditWidget();
	virtual std::string getValue();
	virtual void setValueFromString(const std::string& value);
};

/**
 * greebo: This is an item querying an actor (dropdown combo)
 */
class ActorArgument :
	public CommandArgumentItem
{
protected:
	const ActorColumns& _actorColumns;
	Glib::RefPtr<Gtk::ListStore> _actorStore;
	Gtk::ComboBox* _comboBox;
public:
	// Pass the reference to the helper class
	ActorArgument(const conversation::ArgumentInfo& argInfo,
				  const Glib::RefPtr<Gtk::ListStore>& actorStore,
				  const ActorColumns& actorColumns);

	virtual Gtk::Widget& getEditWidget();
	virtual std::string getValue();
	virtual void setValueFromString(const std::string& value);
};

} // namespace ui

#endif /* COMMAND_ARGUMENT_ITEM_H_ */
