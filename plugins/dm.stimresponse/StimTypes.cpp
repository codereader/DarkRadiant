#include "StimTypes.h"

#include "iregistry.h"
#include "string/string.h"

#include <iostream>

	namespace {
		const std::string RKEY_STIM_DEFINITIONS = "game/stimResponseSystem//stim";
	}

StimTypes::StimTypes() {
	xml::NodeList stimNodes = GlobalRegistry().findXPath(RKEY_STIM_DEFINITIONS);
	
	for (unsigned int i = 0; i < stimNodes.size(); i++) {
		int id = strToInt(stimNodes[i].getAttributeValue("id"));
		
		Stim newStim;
		newStim.name = stimNodes[i].getAttributeValue("name");
		newStim.caption = stimNodes[i].getAttributeValue("caption");
		newStim.description = stimNodes[i].getAttributeValue("description");
		
		// Add the stim to the map
		_stims[id] = newStim;
	}
}

Stim StimTypes::get(int id) {
	StimTypeMap::iterator i = _stims.find(id);
	
	if (i != _stims.end()) {
		return i->second;
	}
	else {
		return _emptyStim;
	}
}
