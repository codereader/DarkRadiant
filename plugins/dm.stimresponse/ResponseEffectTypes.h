#ifndef RESPONSEEFFECTTYPES_H_
#define RESPONSEEFFECTTYPES_H_

#include <map>
#include <string>
#include "ieclass.h"

	namespace {
		const std::string RKEY_RESPONSE_EFFECT_PREFIX = 
				"game/stimResponseSystem/responseEffectPrefix";
	}

typedef std::map<std::string, IEntityClassPtr> ResponseEffectTypeMap;

class ResponseEffectTypes
{
	// The actual map where all the name => EClassPtr associations are stored.
	ResponseEffectTypeMap _effectTypes;
public:
	ResponseEffectTypes();
	
	// Contains the static instance of this class
	static ResponseEffectTypes& Instance();
	
	/** greebo: Returns the entityclass pointer for the given name.
	 */
	IEntityClassPtr getEClassForName(const std::string& name);
	
	/** greebo: Returns a reference to the internally stored EffectTypeMap
	 */
	ResponseEffectTypeMap& getMap();
};

#endif /*RESPONSEEFFECTTYPES_H_*/
