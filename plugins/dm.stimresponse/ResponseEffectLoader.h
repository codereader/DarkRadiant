#ifndef RESPONSEEFFECTLOADER_H_
#define RESPONSEEFFECTLOADER_H_

#include "iregistry.h"
#include "ResponseEffectTypes.h"
#include <boost/algorithm/string/predicate.hpp>

	namespace {
		const std::string RKEY_RESPONSE_EFFECT_PREFIX = 
				"game/stimResponseSystem/responseEffectPrefix";
	}

class ResponseEffectLoader :
	public EntityClassVisitor
{
	ResponseEffectTypeMap& _map;
	
	std::string _prefix;
public:
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

#endif /*RESPONSEEFFECTLOADER_H_*/
