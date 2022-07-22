#include "ResponseEffectTypes.h"

#include "ieclass.h"
#include "iregistry.h"
#include "gamelib.h"
#include "string/predicate.h"

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
		_prefix = game::current::getValue<std::string>(GKEY_RESPONSE_EFFECT_PREFIX);
	}

	void visit(const IEntityClassPtr& eclass)
	{
		if (string::starts_with(eclass->getDeclName(), _prefix))
		{
			// We have a match, store the eclassptr
			_map[eclass->getDeclName()] = eclass;
		}
	}
};

ResponseEffectTypes::ResponseEffectTypes()
{
	// Load the possible effect types
	ResponseEffectLoader loader(_effectTypes);
	GlobalEntityClassManager().forEachEntityClass(loader);
}

std::shared_ptr<ResponseEffectTypes>& ResponseEffectTypes::InstancePtr()
{
	static std::shared_ptr<ResponseEffectTypes> _instance;
	return _instance;
}

// Static accessor
ResponseEffectTypes& ResponseEffectTypes::Instance()
{
	if (!InstancePtr())
	{
		InstancePtr().reset(new ResponseEffectTypes);
	}

	return *InstancePtr();
}

void ResponseEffectTypes::Clear()
{
	InstancePtr().reset();
}

IEntityClassPtr ResponseEffectTypes::getEClassForName(const std::string& name)
{
	// Try to lookup the given name in the map
	ResponseEffectTypeMap::iterator found = _effectTypes.find(name);

	// Return an empty pointer if no effect type is matching
	return (found != _effectTypes.end()) ? found->second : IEntityClassPtr();
}

ResponseEffectTypeMap& ResponseEffectTypes::getMap()
{
	return _effectTypes;
}

std::string ResponseEffectTypes::getFirstEffectName() 
{
	ResponseEffectTypeMap::iterator found = _effectTypes.begin();

	// Return the first name or nothing, if we didn't find anything
	return (found != _effectTypes.end()) ? found->first : "";
}
