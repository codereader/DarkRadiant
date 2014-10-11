#pragma once

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

class wxSpinCtrl;

namespace objectives 
{

namespace ce 
{

/**
 * ComponentEditor subclass for COMP_AI_FIND_BODY component type.
 *
 * An COMP_AI_FIND_BODY component doesn't use specifiers.
 */
class AIFindBodyComponentEditor :
	public ComponentEditorBase
{
private:

	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_AI_FIND_BODY().getName(),
				ComponentEditorPtr(new AIFindBodyComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the body to be found
	SpecifierEditCombo* _bodyCombo;

	// The spin button to specify the amount of AI to be knocked out
	wxSpinCtrl* _amount;

	/**
	 * Construct a default AIFindBodyComponentEditor.
	 */
	AIFindBodyComponentEditor() :
		_component(NULL),
		_amount(NULL)
	{}

	/**
	 * Construct a AIFindBodyComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	AIFindBodyComponentEditor(wxWindow* parent, Component& component);

public:
	// ComponentEditor implementation 

	ComponentEditorPtr create(wxWindow* parent, Component& component) const
	{
		return ComponentEditorPtr(new AIFindBodyComponentEditor(parent, component));
	}

	void writeToComponent() const;
};

} // namespace ce

} // namespace objectives
