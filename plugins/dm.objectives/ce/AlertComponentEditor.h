#ifndef ALERT_COMPONENT_EDITOR_H_
#define ALERT_COMPONENT_EDITOR_H_

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace Gtk { class SpinButton; }

namespace objectives {

namespace ce {

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
	Gtk::SpinButton* _amount;
	Gtk::SpinButton* _alertLevel;

public:

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
	AlertComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new AlertComponentEditor(component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* ALERT_COMPONENT_EDITOR_H_ */
