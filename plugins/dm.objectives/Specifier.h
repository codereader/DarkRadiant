#ifndef SPECIFIER_H_
#define SPECIFIER_H_

#include <map>
#include <string>

namespace objectives
{

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
 * instances are registered in a map so that Specifier objects can be obtained
 * for a given string name (as provided by a spawnarg).
 */
class Specifier
{
	// Static enum count
	static int enumCount;
	
	// Static map of name Specifier instances
	typedef std::map<std::string, Specifier> SpecifierMap;
	static SpecifierMap& getMap();
	
	// The integer number of this Specifier
	int _id;
	
	// The string name of this Specifier (used as a spawnarg)
	std::string _name;
	
private:
	
	// Construct a named Specifier type
	Specifier(const std::string& name);
	
public:
	
	/**
	 * Retrieve the Specifier instance corresponding to the given name.
	 * 
	 * @param name
	 * The name of the Specifier instance to look up (e.g. "KILL").
	 * 
	 * @return
	 * The named Specifier if found.
	 * 
	 * @exception std::runtime_error
	 * Thrown if the named Specifier type does not exist.
	 */
	static const Specifier& getSpecifier(const std::string& name); 
	
	/**
	 * Get the string name of this Specifier type.
	 */
	std::string getName() const {
		return _name;
	}
	
	/**
	 * Get the numeric ID of this Specifier type.
	 */
	int getId() const {
		return _id;
	}
	
	/**
	 * @name Static Specifier instances.
	 */
	
	//@{

	/** No specifier. */
	static const Specifier& SPEC_NONE();
	
	/** Specify name of an entity. */
	static const Specifier& SPEC_NAME();
	
	/** Overall specifier for AI, inventory etc. */
	static const Specifier& SPEC_OVERALL();
	
	/** Type-specific group specifier. */
	static const Specifier& SPEC_GROUP();
	
	/** Specify a DEF-based class name. */
	static const Specifier& SPEC_CLASSNAME();
	
	/** Specify an SDK-level spawnclass. */
	static const Specifier& SPEC_SPAWNCLASS();
	
	/** Specify an AI type, such as "human". */
	static const Specifier& SPEC_AI_TYPE();
	
	/** Specify a numeric AI team ID. */
	static const Specifier& SPEC_AI_TEAM();
	
	/** Specify the combat/non-combat status of an AI. */
	static const Specifier& SPEC_AI_INNOCENCE();
	
	//@}
	
};

/**
 * Operator less function for Specifiers.
 * 
 * This function returns true if the first Specifier's ID is less than the
 * second. It is required so that Specifier objects can be used as keys in a
 * std::map.
 */
inline bool operator< (const Specifier& first, const Specifier& second)
{
	return first.getId() < second.getId();
}

}

#endif /*SPECIFIER_H_*/
