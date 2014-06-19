#pragma once

#include "ResponseEffect.h"

class StimTypes;
class wxTextCtrl;
class wxComboBox;
class wxChoice;
class wxWindow;
class wxCheckBox;
class wxBitmapComboBox;
class wxStaticText;

class EffectArgumentItem
{
protected:
	// The argument this row is referring to
	ResponseEffect::Argument& _arg;

	wxStaticText* _label;
	wxStaticText* _descBox;

public:
	EffectArgumentItem(wxWindow* parent, ResponseEffect::Argument& arg);

	// destructor
	virtual ~EffectArgumentItem() {}

	/** greebo: This retrieves the string representation of the
	 * 			current value of this row. This has to be
	 * 			implemented by the derived classes.
	 */
	virtual std::string getValue() = 0;

	// Retrieve the label widget
	virtual wxWindow* getLabelWidget();

	// Retrieve the edit widgets (abstract)
	virtual wxWindow* getEditWidget() = 0;

	// Retrieves the help widget (a question mark with a tooltip)
	virtual wxWindow* getHelpWidget();

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
	wxTextCtrl* _entry;

public:
	StringArgument(wxWindow* parent, ResponseEffect::Argument& arg);

	virtual wxWindow* getEditWidget();
	virtual std::string getValue();
};

/** greebo: This is an item querying a float (derives from string)
 */
class FloatArgument :
	public StringArgument
{
public:
	FloatArgument(wxWindow* parent, ResponseEffect::Argument& arg) :
		StringArgument(parent, arg)
	{}
};

/** greebo: This is an item querying a vector (derives from string)
 */
class VectorArgument :
	public StringArgument
{
public:
	VectorArgument(wxWindow* parent, ResponseEffect::Argument& arg) :
		StringArgument(parent, arg)
	{}
};

class BooleanArgument :
	public EffectArgumentItem
{
	wxCheckBox* _checkButton;
public:
	BooleanArgument(wxWindow* parent, ResponseEffect::Argument& arg);

	virtual wxWindow* getEditWidget();
	virtual std::string getValue();
};

/** greebo: This is an item querying an entity name (entry/dropdown combo)
 */
class EntityArgument :
	public EffectArgumentItem
{
	wxComboBox* _comboBox;
public:
	// Pass the entity name list to this item so that the auto-completion
	// of the entity combo box works correctly
	EntityArgument(wxWindow* parent, ResponseEffect::Argument& arg,
				   const wxArrayString& entityChoices);

	virtual wxWindow* getEditWidget();
	virtual std::string getValue();
};

/** greebo: This is an item querying an stimtype (dropdown combo)
 */
class StimTypeArgument :
	public EffectArgumentItem
{
private:
	const StimTypes& _stimTypes;

#if USE_BMP_COMBO_BOX
	wxBitmapComboBox* _comboBox;
#else
	wxComboBox* _comboBox;
#endif

public:
	// Pass the reference to the StimType helper class
	StimTypeArgument(wxWindow* parent, 
					 ResponseEffect::Argument& arg,
				     const StimTypes& stimTypes);

	virtual wxWindow* getEditWidget();
	virtual std::string getValue();
};
