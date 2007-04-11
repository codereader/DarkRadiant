#include "SRPropertySaver.h"

#include "iregistry.h"
#include "entitylib.h"
#include "string/string.h"
#include <iostream>

SRPropertySaver::SRPropertySaver(Entity* target, SREntity::KeyList& keys) :
	_target(target),
	_keys(keys)
{}

void SRPropertySaver::visit(StimResponse& sr) {
	std::string prefix = GlobalRegistry().get(RKEY_STIM_RESPONSE_PREFIX);
	std::string suffix = "_" + intToStr(sr.getIndex());
	
	// Now cycle through the possible key names and see if we have a match
	for (unsigned int i = 0; i < _keys.size(); i++) {
		// Check if the property carries a non-empty value
		std::string value = sr.get(_keys[i].key);
		
		// The class of this S/R
		std::string srClass = sr.get("class");
		
		// Check if the property matches for the class type
		if (!value.empty() && _keys[i].classes.find(srClass, 0) != std::string::npos) {
			std::string fullKey = prefix + _keys[i].key + suffix;
			
			if (sr.inherited() && sr.isOverridden(_keys[i].key)) {
				// Save the key
				_target->setKeyValue(fullKey, value);
			}
			else if (!sr.inherited()) {
				// Save the key, if it hasn't been inherited and not overridden
				_target->setKeyValue(fullKey, value);
			}
		}
	}
	
	// If we have a Response, save the response effects to the spawnargs
	if (sr.get("class") == "R") {
		std::string responseEffectPrefix = 
			GlobalRegistry().get(RKEY_RESPONSE_EFFECT_PREFIX);
		
		// Re-index the effect map before saving
		sr.sortEffects();
		
		// Retrieve the effect map and save them one after the other
		StimResponse::EffectMap& effects = sr.getEffects();
		
		for (StimResponse::EffectMap::iterator i = effects.begin(); 
			 i != effects.end(); 
			 i++) 
		{
			ResponseEffect& effect = i->second;
			
			// Save the effect declaration ("sr_effect_1_1")
			std::string key = prefix + responseEffectPrefix + 
							  intToStr(sr.getIndex()) + "_" + intToStr(i->first);
						
			if (effect.isInherited() && effect.nameIsOverridden()) {
				// Overridden name, save it
				_target->setKeyValue(key, effect.getName());
			}
			else if (!effect.isInherited()) {
				// Non-inherited name, save it
				_target->setKeyValue(key, effect.getName());
			}
			
			// Now save the arguments
			ResponseEffect::ArgumentList& args = effect.getArguments();
			
			for (ResponseEffect::ArgumentList::iterator a = args.begin(); 
				 a != args.end(); a++)
			{
				int argIndex = a->first;
				std::string argValue = a->second.value;
				
				// Construct the argument key ("sr_effect_3_2_arg4")
				std::string argKey = key + "_arg" + intToStr(argIndex);
				
				if (effect.isInherited() && effect.argIsOverridden(argIndex)) {
					// This is an overridden argument, save it
					_target->setKeyValue(argKey, argValue);
				}
				else if (!effect.isInherited()) {
					// A non-inherited argument, save it
					_target->setKeyValue(argKey, argValue);
				}
			}
			
			if (effect.isInherited() && effect.activeIsOverridden()) {
				std::cout << "Overridden state found: " << (key + "_state") << "\n";
				// An overridden state, save the spawnarg in every case
				_target->setKeyValue(key + "_state", effect.isActive() ? "1" : "0");
			}
			else if (!effect.isInherited()) {
				// A non-inherited state, save it only if necessary (inactive)
				if (!effect.isActive()) {
					_target->setKeyValue(key + "_state", "0");
				}
			}
		}
	}
}
