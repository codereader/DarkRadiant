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
 * ComponentEditor subclass for KILL component type.
 *
 * A KILL component requires that an AI must be killed. It can use all
 * specifiers except SPEC_GROUP.
 */
class KillComponentEditor
: public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_KILL().getName(),
				ComponentEditorPtr(new KillComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the kill target
	SpecifierEditCombo* _targetCombo;

	// The spin button to specify the amount of AI to be killed
	wxSpinCtrl* _amount;

	/**
	 * Construct a default KillComponentEditor.
	 */
	KillComponentEditor() :
		_component(NULL),
		_amount(NULL)
	{ }

	/**
	 * Construct a KillComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	KillComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const
	{
		return ComponentEditorPtr(new KillComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

}

}
