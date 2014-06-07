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
 * ComponentEditor subclass for KO component type.
 *
 * A KO component requires that an AI must be knocked out. It can use all
 * specifiers except SPEC_GROUP.
 */
class KnockoutComponentEditor
: public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_KO().getName(),
				ComponentEditorPtr(new KnockoutComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the knockout target
	SpecifierEditCombo* _targetCombo;

	// The spin button to specify the amount of AI to be knocked out
	wxSpinCtrl* _amount;

	/**
	 * Construct a default KnockoutComponentEditor.
	 */
	KnockoutComponentEditor() :
		_component(NULL),
		_amount(NULL)
	{ }

	/**
	 * Construct a KnockoutComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	KnockoutComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const 
	{
		return ComponentEditorPtr(new KnockoutComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

}

}
