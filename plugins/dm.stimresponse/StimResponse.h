#ifndef STIMRESPONSE_H_
#define STIMRESPONSE_H_

#include <map>
#include <list>
#include <string>
#include "ResponseEffect.h"

	namespace {
		const std::string RKEY_STIM_RESPONSE_PREFIX = 
				"game/stimResponseSystem/stimResponsePrefix";
	}

struct SRKey {
	// The key name
	std::string key;
	
	// A string defining the classes this applies for
	// e.g. "R" for responses only, "SR" for both
	std::string classes;
};

/** greebo: A simple stim/response representation structure
 * (a SREntity can hold a bunch of these)
 */
class StimResponse
{
public:
	enum SRClass {
		typeStim,
		typeResponse,
	};

private:
	// The key/value mapping
	typedef std::map<std::string, std::string> PropertyMap;

	// The S/R class
	SRClass _class;
	
	// TRUE, if this stems from an inherited eclass. Makes this object read-only	
	bool _inherited;
	
	// The list of named properties
	PropertyMap _properties;
	
	/** greebo: The index of this object. The StimResponse objects themselves
	 * 			are already mapped by the SREntity class, although it's
	 * 			useful to set an index of this object by hand to allow
	 * 			it to override an inherited stim.
	 */
	int _index;
	
	typedef std::map<unsigned int, ResponseEffect> EffectMap;
	
	// The list of ResponseEffects for this response (does not apply for stims)
	EffectMap _effects;
	
public:
	StimResponse();
	
	// Copy constructor
	StimResponse(const StimResponse& other);
	
	/** greebo: Returns / sets the "inherited" flag
	 */
	bool inherited() const;
	void setInherited(bool inherited);
	
	/** greebo: Gets/Sets the index of this object (only for non-inherited)
	 */
	int getIndex() const;
	void setIndex(int index);
	
	/** greebo: Gets the property value string or "" if not defined/empty
	 */
	std::string get(const std::string& key);
	
	/** greebo: Sets the given <key> to <value>
	 */
	void set(const std::string& key, const std::string& value);
	
	/** greebo: Retrieves the response effect with the given id.
	 * 			This creates the response effect if it doesn't exist yet.
	 */
	ResponseEffect& getResponseEffect(const unsigned int index);
};

#endif /*STIMRESPONSE_H_*/
