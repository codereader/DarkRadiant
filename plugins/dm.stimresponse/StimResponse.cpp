#include "StimResponse.h"

#include <gtk/gtk.h>
#include "string/string.h"
#include <iostream>

StimResponse::StimResponse() :
	_inherited(false),
	_index(0)
{}

// Copy constructor
StimResponse::StimResponse(const StimResponse& other) :
	_inherited(other._inherited),
	_properties(other._properties),
	_index(other._index)
{}

/** greebo: Gets the property value string or "" if not defined/empty
 */
std::string StimResponse::get(const std::string& key) {
	
	PropertyMap::iterator i = _properties.find(key);
	
	if (i != _properties.end()) {
		return _properties[key];
	}
	else {
		// Not found, return an empty string
		return "";
	}
}

int StimResponse::getIndex() const {
	return _index;
}

void StimResponse::setIndex(int index) {
	if (!_inherited) {
		_index = index;
	}
}

void StimResponse::set(const std::string& key, const std::string& value) {
	_properties[key] = value;
}

void StimResponse::setInherited(bool inherited) {
	_inherited = inherited;
}

bool StimResponse::inherited() const {
	return _inherited;
}

ResponseEffect& StimResponse::getResponseEffect(const unsigned int index) {
	EffectMap::iterator found = _effects.find(index);
	
	if (found == _effects.end()) {
		// ResponseEffect doesn't exist yet, create a new, empty one
		 _effects[index] = ResponseEffect();
	}
	
	return _effects[index];
}

void StimResponse::sortEffects() {
	EffectMap newMap;
	
	// Re-index the effects to avoid gaps in the indexing
	int newIndex = 1;
	for (EffectMap::iterator i = _effects.begin(); 
		 i != _effects.end(); 
		 i++, newIndex++) 
	{
		// Copy the visited ResponseEffect to the new index
		newMap[newIndex] = i->second;
	}
	
	// Replace the old map with the sorted one
	_effects = newMap;
}

void StimResponse::moveEffect(const unsigned int fromIndex, 
							  const unsigned int toIndex)
{
	EffectMap::iterator from = _effects.find(fromIndex);
	EffectMap::iterator to = _effects.find(toIndex);
	
	if (from != _effects.end() && to != _effects.end()) {
		// Copy the ResponseEffects from the map
		ResponseEffect fromEffect = from->second;
		ResponseEffect toEffect = to->second;
		
		// Write them back at the swapped locations
		_effects[fromIndex] = toEffect;
		_effects[toIndex] = fromEffect;
	}
}

unsigned int StimResponse::highestEffectIndex() {
	unsigned int returnValue = 0;
	
	// Search for the highest index
	for (EffectMap::iterator i = _effects.begin(); i != _effects.end(); i++) {
		if (i->first > returnValue) {
			returnValue = i->first;
		}
	}
	
	return returnValue;
}

void StimResponse::addEffect(const unsigned int index) {
	// Resort the effects, it may be unsorted when loaded fresh from the entity
	sortEffects();
	
	EffectMap::iterator found = _effects.find(index);
	
	if (found == _effects.end()) {
		unsigned int newIndex = highestEffectIndex() + 1; 
		// No item found (index could be -1), append to the end of the list
		_effects[newIndex] = ResponseEffect();
		_effects[newIndex].setName(
			ResponseEffectTypes::Instance().getFirstEffectName()
		);
	}
	else {
		EffectMap newMap;
	
		// Traverse the current effect list from back to front and 
		// increase all indices >= index and insert a new effect   
		for (EffectMap::reverse_iterator i = _effects.rbegin(); 
			 i != _effects.rend(); 
			 i++) 
		{
			// Increase all indices >= index
			if (i->first >= index) {
				// Store the item to index + 1
				newMap[i->first + 1] = i->second;
			}
			else {
				// All smaller indices get copied
				newMap[i->first] = i->second;
			}
			
			// If we are exactly at the insert point, insert the new effect
			if (i->first == index) {
				newMap[i->first] = ResponseEffect();
				newMap[i->first].setName(
					ResponseEffectTypes::Instance().getFirstEffectName()
				);
			}
		}
		
		// Replace the old map with the new one
		_effects = newMap;
	}
}

void StimResponse::deleteEffect(const unsigned int index) {
	EffectMap::iterator found = _effects.find(index);
	
	if (found != _effects.end()) {
		// Remove the item from the map
		_effects.erase(found);
	}
	
	// Re-index the effects in the map
	sortEffects();
}

StimResponse::EffectMap& StimResponse::getEffects() {
	return _effects;
}

GtkListStore* StimResponse::getEffectStore() {
	GtkListStore* store = gtk_list_store_new(EFFECT_NUM_COLS,
											 G_TYPE_INT,	// Index
											 G_TYPE_STRING, // Caption
											 G_TYPE_STRING, // Arguments
											 G_TYPE_STRING, // Index String
											 -1);
	
	for (EffectMap::iterator i = _effects.begin(); i != _effects.end(); i++) {
		GtkTreeIter iter;
		
		int index = i->first;
		
		gtk_list_store_append(store, &iter);
		// Store the ID into the liststore
		gtk_list_store_set(store, &iter, 
						   EFFECT_INDEX_COL, index,
						   EFFECT_INDEX_STR_COL, intToStr(index).c_str(),
						   -1);
		
		// And write the rest of the data to the row
		ResponseEffect& effect = i->second;
		writeToListStore(store, &iter, effect);
	}
	
	return store;
}

void StimResponse::writeToListStore(GtkListStore* store, GtkTreeIter* iter, ResponseEffect& effect) {
	gtk_list_store_set(store, iter, 
					   EFFECT_CAPTION_COL, effect.getCaption().c_str(),
					   EFFECT_ARGS_COL, effect.getArgumentStr().c_str(),
					   -1);
}
