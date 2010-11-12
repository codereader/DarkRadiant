#ifndef SPECIFIERTYPE_H_
#define SPECIFIERTYPE_H_

#include <map>
#include <string>
#include <set>

namespace objectives
{

class SpecifierType;

/**
 * Set of SpecifierTypes type.
 */
typedef std::set<SpecifierType> SpecifierTypeSet;

/**
 * Type-safe enumeration of valid specifier types.
 *
 * Each Component can have up to 2 <b>specifiers</b>, which provide
 * information about the target of the component depending on the component
 * type. For example, a KILL component must specify an AI or group of AI
 * to be killed.
 *
 * Each specifier consists of two parts, a specifier <b>type</b> and a
 * specifier <b>value</b>. This enumeration provides the available specifier
 * types, which are used by the game to determine the meaning of the
 * associated specifier value.
 *
 * This class uses the typesafe enum idiom. A number of static const instances
 * are defined, each of which can provide its ID and its name. Additionally the
 * instances are registered in a map so that SpecifierType objects can be obtained
 * for a given string name (as provided by a spawnarg).
 */
class SpecifierType
{
	// Static enum count
	static int enumCount;

	// Static map of named SpecifierType instances
	typedef std::map<std::string, SpecifierType> SpecifierTypeMap;
	static SpecifierTypeMap& getMap();

	// The integer number of this SpecifierType
	int _id;

	// The string name of this SpecifierType (used as a spawnarg)
	std::string _name;

	// User-friendly display name of this SpecifierType
	std::string _displayName;

private:

	// Construct a named SpecifierType type
	SpecifierType(const std::string& name, const std::string& displayName);

public:

	/**
	 * Retrieve the SpecifierType instance corresponding to the given name.
	 *
	 * @param name
	 * The name of the SpecifierType instance to look up (e.g. "KILL").
	 *
	 * @return
	 * The named SpecifierType if found.
	 *
	 * @exception ObjectivesException
	 * Thrown if the named SpecifierType type does not exist.
	 */
	static const SpecifierType& getSpecifierType(const std::string& name);

	/**
	 * Get the string name of this SpecifierType type.
	 */
	std::string getName() const {
		return _name;
	}

	/**
	 * Get the user-friendly display name of this SpecifierType type.
	 */
	std::string getDisplayName() const {
		return _displayName;
	}

	/**
	 * Get the numeric ID of this SpecifierType type.
	 */
	int getId() const {
		return _id;
	}

	/**
	 * @name Static SpecifierType instances.
	 */

	//@{

	/** No specifier (used in-game to represent "any entity"). */
	static const SpecifierType& SPEC_NONE();

	/** Specify name of an entity. */
	static const SpecifierType& SPEC_NAME();

	/** Overall specifier for AI, inventory etc. */
	static const SpecifierType& SPEC_OVERALL();

	/** Type-specific group specifier. */
	static const SpecifierType& SPEC_GROUP();

	/** Specify a DEF-based class name. */
	static const SpecifierType& SPEC_CLASSNAME();

	/** Specify an SDK-level spawnclass. */
	static const SpecifierType& SPEC_SPAWNCLASS();

	/** Specify an AI type, such as "human". */
	static const SpecifierType& SPEC_AI_TYPE();

	/** Specify a numeric AI team ID. */
	static const SpecifierType& SPEC_AI_TEAM();

	/** Specify the combat/non-combat status of an AI. */
	static const SpecifierType& SPEC_AI_INNOCENCE();

	//@}

	/**
	 * @name Static specifier sets
	 */

	//@{

	/** Set of all specifiers. */
	static const SpecifierTypeSet& SET_ALL();

	// The specifier set for the COMP_ITEM components
	static const SpecifierTypeSet& SET_ITEM();

	// The specifier set for the COMP_READABLE_* components
	static const SpecifierTypeSet& SET_READABLE();

	// The specifier set for COMP_LOCATION-style components
	static const SpecifierTypeSet& SET_LOCATION();

	/** Set for standard AI specifiers (all except SPEC_GROUP). */
	static const SpecifierTypeSet& SET_STANDARD_AI();

	//@}

};

/**
 * Operator less function for SpecifierTypes.
 *
 * This function returns true if the first SpecifierType's ID is less than the
 * second. It is required so that SpecifierType objects can be used as keys in a
 * std::map.
 */
inline bool operator< (const SpecifierType& first, const SpecifierType& second)
{
	return first.getId() < second.getId();
}

/**
 * Operator equals function for SpecifierTypes.
 *
 * Returns true if the two SpecifierTypes have the same internal name, false
 * otherwise.
 */
inline bool operator== (const SpecifierType& first, const SpecifierType& second)
{
    return first.getName() == second.getName();
}
inline bool operator!= (const SpecifierType& first, const SpecifierType& second)
{
    return !(first == second);
}

}

#endif /*SPECIFIERTYPE_H_*/
