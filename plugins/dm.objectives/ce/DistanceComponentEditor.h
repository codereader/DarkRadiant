#ifndef DISTANCE_COMPONENT_EDITOR_H_
#define DISTANCE_COMPONENT_EDITOR_H_

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace Gtk
{
	class Entry;
	class SpinButton;
}

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_DISTANCE component type.
 *
 * An COMP_DISTANCE component usually uses a two specifiers.
 */
class DistanceComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_DISTANCE().getName(),
				ComponentEditorPtr(new DistanceComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// Name of the entity in question
	Gtk::Entry* _entity;

	// Name of the location entity
	Gtk::Entry* _location;

	// The distance between these two
	Gtk::SpinButton* _distance;

	// The evaluation interval in seconds
	Gtk::SpinButton* _interval;

public:

	/**
	 * Construct a default DistanceComponentEditor.
	 */
	DistanceComponentEditor() :
		_component(NULL),
		_entity(NULL),
		_location(NULL),
		_distance(NULL),
		_interval(NULL)
	{}

	/**
	 * Construct a DistanceComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	DistanceComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new DistanceComponentEditor(component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* DISTANCE_COMPONENT_EDITOR_H_ */
