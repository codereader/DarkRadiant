#include "ResponseEffectTypes.h"

#include "ieclass.h"
#include "iregistry.h"
#include <boost/algorithm/string/predicate.hpp>

	namespace {
		const std::string RKEY_RESPONSE_EFFECT_PREFIX = 
				"game/stimResponseSystem/responseEffectPrefix";
	}

/** greebo: The visitor class that stores all the relevant eclassptrs
 * 			into the given target map if the prefix matches.
 */
class ResponseEffectLoader :
	public EntityClassVisitor
{
	// The target map to populate
	ResponseEffectTypeMap& _map;
	
	// The entityDef prefix (e.g. "effect_")
	std::string _prefix;
public:
	/** greebo: Pass the target map where all the eclassptrs
	 * 			should be stored into. 
	 */
	ResponseEffectLoader(ResponseEffectTypeMap& map) :
		_map(map)
	{
		_prefix = GlobalRegistry().get(RKEY_RESPONSE_EFFECT_PREFIX);
	}
	
	void visit(IEntityClassPtr eclass) {
		if (boost::algorithm::starts_with(eclass->getName(), _prefix)) {
			// We have a match, store the eclassptr
			_map[eclass->getName()] = eclass;
		}
	}
};

ResponseEffectTypes::ResponseEffectTypes() {
	// Load the possible effect types
	ResponseEffectLoader loader(_effectTypes);
	GlobalEntityClassManager().forEach(loader);
}

// Static accessor
ResponseEffectTypes& ResponseEffectTypes::Instance() {
	static ResponseEffectTypes _instance;
	return _instance;
}

IEntityClassPtr ResponseEffectTypes::getEClassForName(const std::string& name) {
	// Try to lookup the given name in the map
	ResponseEffectTypeMap::iterator found = _effectTypes.find(name);
	// Return an empty pointer if no effect type is matching
	return (found != _effectTypes.end()) ? found->second : IEntityClassPtr();
}

ResponseEffectTypeMap& ResponseEffectTypes::getMap() {
	return _effectTypes;
}
