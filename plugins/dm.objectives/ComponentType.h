#ifndef COMPONENTTYPE_H_
#define COMPONENTTYPE_H_

#include <set>
#include <map>
#include <string>

namespace objectives
{

class ComponentType;

/**
 * Set of ComponentTypes.
 */
typedef std::set<ComponentType> ComponentTypeSet;

/**
 * Enumeration of Component types.
 * 
 * This is a typesafe enum containing all of the available Component types,
 * such as COMP_KILL and COMP_KO. Named static instance functions are provided
 * for all of the types, and a std::map is also available to look up a 
 * ComponentType instance by name (as provided by a spawnarg).
 */
class ComponentType
{
	// Static enum count
	static int enumCount;
	
	// Static map of named ComponentType instances
	typedef std::map<std::string, ComponentType> ComponentTypeMap;
	static ComponentTypeMap& getMap();
	
	// Integer ID of this ComponentType
	int _id;
	
	// Raw name of this type
	std::string _name;
	
	// User-friendly display name
	std::string _displayName;
	
private:
	
	// Construct a named ComponentType
	ComponentType(const std::string& name, const std::string& displayName);
	
public:
	
	/**
	 * Get the ComponentType identified by the given name.
	 * 
	 * @param name
	 * The text name of the ComponentType to retrieve, such as "kill" or "ko".
	 * 
	 * @return
	 * The identified ComponentType if it exists.
	 * 
	 * @exception std::runtime_error
	 * Thrown if the named ComponentType does not exist.
	 */
	static ComponentType getComponentType(const std::string& name);
	
	/**
	 * Get the name of this ComponentType.
	 * 
	 * This returns the "raw" name of the ComponentType as used in the entity
	 * spawnargs, such as "ai_alert" or "ko".
	 */
	const std::string& getName() const {
		return _name;
	}
	
	/**
	 * Get the display name of this ComponentType.
	 * 
	 * This returns a user-friendly display name which is suitable for
	 * identifying the ComponentType in a dialog, such as "AI is killed".
	 */
	const std::string& getDisplayName() const {
		return _displayName;
	}
	
	/**
	 * Get the numeric ID of this ComponentType.
	 */
	int getId() const {
		return _id;
	}
	
	/**
	 * @name ComponentType instances.
	 */
	
	//@{
	
	/** AI is killed. */
	static const ComponentType& COMP_KILL();
	
	/** AI is knocked out. */
	static const ComponentType& COMP_KO();
	
	/** AI finds a body. */
	static const ComponentType& COMP_AI_FIND_BODY();
	
	/** AI is alerted. */
	static const ComponentType& COMP_AI_ALERT();
	
	/** Object is destroyed. */
	static const ComponentType& COMP_DESTROY();
	
	/** Player possesses an item or loot. */
	static const ComponentType& COMP_ITEM();
	
	/** Item is pickpocketed from conscious AI. */
	static const ComponentType& COMP_PICKPOCKET();
	
	/** Item is in a particular objective location (defined by a brush). */
	static const ComponentType& COMP_LOCATION();
	
	/** Item is in a particular <b>info_location</b> area. */
	static const ComponentType& COMP_INFO_LOCATION();
	
	/** Custom component updated by user script. */
	static const ComponentType& COMP_CUSTOM_ASYNC();
	
	/** Custom component which periodically checks a user script. */
	static const ComponentType& COMP_CUSTOM_CLOCKED();
	
	/** Two entities are within a radius of each other. */
	static const ComponentType& COMP_DISTANCE();
	
	//@}
	
	/**
	 * @name ComponentType convenience sets.
	 */
	
	//@{
	
	/** All ComponentTypes. */
	static const ComponentTypeSet& SET_ALL();
	
	//@}
};

/**
 * Operator less for ComponentType objects.
 * 
 * This is required to allow ComponentTypes to be placed in a map or set.
 */
inline bool operator< (const ComponentType& first, const ComponentType& second)
{
	return first.getId() < second.getId();
}

}

#endif /*COMPONENTTYPE_H_*/
