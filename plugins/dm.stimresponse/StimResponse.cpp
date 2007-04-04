#include "StimResponse.h"

#include <gtk/gtk.h>
#include "string/string.h"

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
											 -1);
	
	for (EffectMap::iterator i = _effects.begin(); i != _effects.end(); i++) {
		GtkTreeIter iter;
		
		int index = i->first;
		
		gtk_list_store_append(store, &iter);
		// Store the ID into the liststore
		gtk_list_store_set(store, &iter, 
						   EFFECT_INDEX_COL, index,
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
