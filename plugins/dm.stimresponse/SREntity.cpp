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
		
		enum {
			ID_COL,
			CAPTION_COL,
			NUM_COLS
		};
	}

SREntity::SREntity(Entity* source) :
	_listStore(gtk_list_store_new(NUM_COLS, G_TYPE_INT, G_TYPE_STRING))
{
	loadKeys();
	load(source);
}

void SREntity::load(Entity* source) {
	// Clear all the items from the liststore
	gtk_list_store_clear(_listStore);
	
	if (source == NULL) {
		return;
	}
	
	// Get the entity class to scan the inherited values
	IEntityClassPtr eclass = GlobalEntityClassManager().findOrInsert(
		source->getKeyValue("classname"), true
	);
	
	// Instantiate a visitor class with the list of possible keys 
	// and the target list where all the S/Rs are stored
	// Warning messages are stored in the <_warnings> string
	SRPropertyLoader visitor(_keys, _list, _warnings);
	eclass->forEachClassAttribute(visitor);
	
	// Now populate the liststore
	GtkTreeIter iter;
	
	for (StimResponseMap::iterator i = _list.begin(); i!= _list.end(); i++) {
		int id = i->first;
		int stimTypeId = strToInt(i->second.get("type"));
		std::string caption = _stimTypes.get(stimTypeId).caption;
		
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 
							ID_COL, id,
							CAPTION_COL, caption.c_str(),
							-1);
	}
}

void SREntity::save(Entity* target) {
	if (target == NULL) {
		return;
	}
}

SREntity::operator GtkListStore* () {
	return _listStore;
}

// static key loader
void SREntity::loadKeys() {
	xml::NodeList propList = GlobalRegistry().findXPath(RKEY_STIM_PROPERTIES);
	
	for (unsigned int i = 0; i < propList.size(); i++) {
		_keys.push_back(propList[i].getAttributeValue("name"));
	}
}
