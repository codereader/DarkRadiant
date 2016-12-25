#pragma once

#include "ConversationCommandInfo.h"
#include "Conversation.h"
#include <memory>

class wxWindow;
class wxStaticText;
class wxTextCtrl;
class wxCheckBox;
class wxChoice;
class wxPanel;

namespace ui
{

class CommandEditor;

class CommandArgumentItem
{
protected:
	// The reference to the editor instance, some arguments need this 
	// for cross-referencing actors and commands
	CommandEditor& _owner;

	// The argument this row is referring to
	const conversation::ArgumentInfo& _argInfo;
	wxStaticText* _labelBox;
	wxStaticText* _descBox;

public:
	CommandArgumentItem(CommandEditor& owner, wxWindow* parent, const conversation::ArgumentInfo& argInfo);

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
	StringArgument(CommandEditor& owner, wxWindow* parent, const conversation::ArgumentInfo& argInfo);

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
	FloatArgument(CommandEditor& owner, wxWindow* parent, const conversation::ArgumentInfo& argInfo) :
		StringArgument(owner, parent, argInfo)
	{}
};

/**
 * greebo: This is an item querying a vector (derives from string)
 */
class VectorArgument :
	public StringArgument
{
public:
	VectorArgument(CommandEditor& owner, wxWindow* parent, const conversation::ArgumentInfo& argInfo) :
		StringArgument(owner, parent, argInfo)
	{}
};

class BooleanArgument :
	public CommandArgumentItem
{
protected:
	wxCheckBox* _checkButton;
public:
	BooleanArgument(CommandEditor& owner, wxWindow* parent, const conversation::ArgumentInfo& argInfo);

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
	ActorArgument(CommandEditor& owner, wxWindow* parent,
		const conversation::ArgumentInfo& argInfo,
		const conversation::Conversation::ActorMap& actors);

	virtual wxWindow* getEditWidget();
	virtual std::string getValue();
	virtual void setValueFromString(const std::string& value);
};

/**
* greebo: This is an item querying a sound shader
*/
class SoundShaderArgument :
	public StringArgument
{
private:
	wxPanel* _soundShaderPanel;

public:
	SoundShaderArgument(CommandEditor& owner, wxWindow* parent, const conversation::ArgumentInfo& argInfo);

	virtual wxWindow* getEditWidget() override;
	virtual std::string getValue() override;
	virtual void setValueFromString(const std::string& value) override;

private:
	void pickSoundShader();
};

/**
* greebo: This is an item querying an animation
*/
class AnimationArgument :
	public StringArgument
{
private:
	wxPanel* _animPanel;

public:
	AnimationArgument(CommandEditor& owner, wxWindow* parent, const conversation::ArgumentInfo& argInfo);

	virtual wxWindow* getEditWidget() override;
	virtual std::string getValue() override;
	virtual void setValueFromString(const std::string& value) override;

private:
	void pickAnimation();
};

} // namespace ui
