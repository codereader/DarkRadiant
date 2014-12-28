#pragma once

#include "ConversationCommandInfo.h"
#include "Conversation.h"
#include <memory>

class wxWindow;
class wxStaticText;
class wxTextCtrl;
class wxCheckBox;
class wxChoice;

namespace ui
{

class CommandArgumentItem
{
protected:
	// The argument this row is referring to
	const conversation::ArgumentInfo& _argInfo;
	wxStaticText* _labelBox;
	wxStaticText* _descBox;

public:
	CommandArgumentItem(wxWindow* parent, const conversation::ArgumentInfo& argInfo);

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
	virtual wxWindow* getLabelWidget();

	// Retrieve the edit widgets (abstract)
	virtual wxWindow* getEditWidget() = 0;

	// Retrieves the help widget (a question mark with a tooltip)
	virtual wxWindow* getHelpWidget();
};
typedef std::shared_ptr<CommandArgumentItem> CommandArgumentItemPtr;

/**
 * greebo: This is an item querying a simple string
 */
class StringArgument :
	public CommandArgumentItem
{
protected:
	wxTextCtrl* _entry;

public:
	StringArgument(wxWindow* parent, const conversation::ArgumentInfo& argInfo);

	virtual wxWindow* getEditWidget();
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
	FloatArgument(wxWindow* parent, const conversation::ArgumentInfo& argInfo) :
		StringArgument(parent, argInfo)
	{}
};

/**
 * greebo: This is an item querying a vector (derives from string)
 */
class VectorArgument :
	public StringArgument
{
public:
	VectorArgument(wxWindow* parent, const conversation::ArgumentInfo& argInfo) :
		StringArgument(parent, argInfo)
	{}
};

class BooleanArgument :
	public CommandArgumentItem
{
protected:
	wxCheckBox* _checkButton;
public:
	BooleanArgument(wxWindow* parent, const conversation::ArgumentInfo& argInfo);

	virtual wxWindow* getEditWidget();
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
	wxChoice* _comboBox;
public:
	// Pass the reference to the helper class
	ActorArgument(wxWindow* parent, 
		const conversation::ArgumentInfo& argInfo,
		const conversation::Conversation::ActorMap& actors);

	virtual wxWindow* getEditWidget();
	virtual std::string getValue();
	virtual void setValueFromString(const std::string& value);
};

} // namespace ui
