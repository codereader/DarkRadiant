#include "SREntity.h"

#include "iregistry.h"
#include "ieclass.h"
#include "entitylib.h"
#include <gtk/gtkliststore.h>

#include "SRPropertyLoader.h"

#include <iostream>

	namespace {
		const unsigned int NUM_MAX_STIMS = 99999;
		
		const std::string RKEY_STIM_PROPERTIES = 
			"game/stimResponseSystem/properties//property";
	}

SREntity::SREntity(Entity* source) {
	loadKeys();
	load(source);
}

void SREntity::load(Entity* source) {
	if (source == NULL) {
		return;
	}
	
	// Get the entity class to scan the inherited values
	IEntityClassPtr eclass = GlobalEntityClassManager().findOrInsert(
		source->getKeyValue("classname"), true
	);
	
	// Instantiate a visitor class with the list of possible keys 
	// and the target list where all the S/Rs are stored
	SRPropertyLoader visitor(_keys, _list);
	eclass->forEachClassAttribute(visitor);
}

void SREntity::save(Entity* target) {
	if (target == NULL) {
		return;
	}
}

SREntity::operator GtkListStore* () {
	return gtk_list_store_new(2);
}

// static key loader
void SREntity::loadKeys() {
	xml::NodeList propList = GlobalRegistry().findXPath(RKEY_STIM_PROPERTIES);
	
	for (unsigned int i = 0; i < propList.size(); i++) {
		_keys.push_back(propList[i].getAttributeValue("name"));
	}
}
