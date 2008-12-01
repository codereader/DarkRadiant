#ifndef COMMAND_ARGUMENT_ITEM_H_
#define COMMAND_ARGUMENT_ITEM_H_

#include "ConversationCommandInfo.h"
#include <boost/shared_ptr.hpp>

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTooltips GtkTooltips;
typedef struct _GtkListStore GtkListStore;

namespace ui {

class CommandArgumentItem
{
protected:
	// The argument this row is referring to
	const conversation::ArgumentInfo& _argInfo;
	GtkWidget* _labelBox;
	GtkWidget* _descBox;
	GtkTooltips* _tooltips;

public:
	CommandArgumentItem(const conversation::ArgumentInfo& argInfo, GtkTooltips* tooltips);
	
	/**
	 * greebo: This retrieves the string representation of the current 
	 * value of this row. This has to be implemented by the subclasses.
	 */
	virtual std::string getValue() = 0;
	
	// Retrieve the label widget
	virtual GtkWidget* getLabelWidget();
	
	// Retrieve the edit widgets (abstract)
	virtual GtkWidget* getEditWidget() = 0;
	
	// Retrieves the help widget (a question mark with a tooltip)
	virtual GtkWidget* getHelpWidget();
};
typedef boost::shared_ptr<CommandArgumentItem> CommandArgumentItemPtr;

/** 
 * greebo: This is an item querying a simple string
 */
class StringArgument :
	public CommandArgumentItem
{
protected:
	GtkWidget* _entry;

public:
	StringArgument(const conversation::ArgumentInfo& argInfo, GtkTooltips* tooltips);
	
	virtual GtkWidget* getEditWidget();
	virtual std::string getValue();
};

/** 
 * greebo: This is an item querying a float (derives from string)
 */
class FloatArgument :
	public StringArgument
{
public:
	FloatArgument(const conversation::ArgumentInfo& argInfo, GtkTooltips* tooltips) :
		StringArgument(argInfo, tooltips)
	{}
};

/** 
 * greebo: This is an item querying a vector (derives from string)
 */
class VectorArgument :
	public StringArgument
{
public:
	VectorArgument(const conversation::ArgumentInfo& argInfo, GtkTooltips* tooltips) :
		StringArgument(argInfo, tooltips)
	{}
};

class BooleanArgument :
	public CommandArgumentItem
{
	GtkWidget* _checkButton;
public:
	BooleanArgument(const conversation::ArgumentInfo& argInfo, GtkTooltips* tooltips);
	
	virtual GtkWidget* getEditWidget();
	virtual std::string getValue();
};

/** 
 * greebo: This is an item querying an stimtype (dropdown combo)
 */
class ActorArgument :
	public CommandArgumentItem
{
	GtkListStore* _actorStore;
	GtkWidget* _comboBox;
public:
	// Pass the reference to the helper class  
	ActorArgument(const conversation::ArgumentInfo& argInfo, 
				   GtkTooltips* tooltips,
				   GtkListStore* actorStore);
	
	virtual GtkWidget* getEditWidget();
	virtual std::string getValue();
};

} // namespace ui

#endif /* COMMAND_ARGUMENT_ITEM_H_ */
