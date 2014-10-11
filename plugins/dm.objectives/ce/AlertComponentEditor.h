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
 * ComponentEditor subclass for COMP_ALERT component type.
 *
 * A COMP_ALERT component requires that an AI must be alerted or not alerted.
 */
class AlertComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_ALERT().getName(),
				ComponentEditorPtr(new AlertComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the kill target
	SpecifierEditCombo* _targetCombo;

	// An entry box for the amount and the alert level
	wxSpinCtrl* _amount;
	wxSpinCtrl* _alertLevel;

	/**
	 * Construct a default AlertComponentEditor.
	 */
	AlertComponentEditor() :
		_component(NULL),
		_amount(NULL),
		_alertLevel(NULL)
	{}

	/**
	 * Construct a AlertComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	AlertComponentEditor(wxWindow* parent, Component& component);

public:

	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const
	{
		return ComponentEditorPtr(new AlertComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives
