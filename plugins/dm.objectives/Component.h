#ifndef COMPONENT_H_
#define COMPONENT_H_

namespace objectives
{

/**
 * Component type enumeration.
 */
enum ComponentType {
	COMP_KILL,			// kill or destroy an object or AI
	COMP_KO,			// knockout an AI
	COMP_AI_FIND_ITEM,
	COMP_AI_FIND_BODY,
	COMP_AI_ALERT,
	COMP_ITEM, 			// Add inventory item or imaginary loot
	COMP_LOCATION, 		// Item X is at location Y
	COMP_CUSTOM_ASYNC, 	// asynchronously updated custom objective
	COMP_CUSTOM_CLOCKED,
	COMP_INFO_LOCATION, // like location, but uses existing info_location areas instead of an info_objectivelocation entity
	COMP_DISTANCE 		// distance from origin of ent X to that of ent Y	
};

/**
 * A component of an objective. Each objective may have a number of components,
 * which are combined using boolean operations to determine whether the
 * objective is satisfied or not.
 */

class Component
{
public:

	// Is this component satisfied?
	bool state;
	
	// Invert the sense of this component
	bool inverted;
	
	// Component changes state once then latches
	bool irreversible;
	
	// The type of this component
	ComponentType type;
	
	// Default constructor
	Component()
	: state(false), inverted(false), irreversible(false), type(COMP_KILL)
	{ }
};

}

#endif /*COMPONENT_H_*/
