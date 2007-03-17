#include "SREntity.h"

#include "iregistry.h"
#include "ieclass.h"
#include "entitylib.h"
#include "gtkutil/image.h"
#include "gtkutil/TreeModel.h"
#include <gtk/gtk.h>

#include "SRPropertyLoader.h"

#include <iostream>

	namespace {
		const std::string RKEY_STIM_PROPERTIES = 
			"game/stimResponseSystem/properties//property";
	}

SREntity::SREntity(Entity* source) :
	_listStore(gtk_list_store_new(NUM_COLS, 
								  G_TYPE_INT,		// S/R index
								  GDK_TYPE_PIXBUF, 	// Type String
								  G_TYPE_STRING, 	// Caption String
								  GDK_TYPE_PIXBUF,	// Icon
								  G_TYPE_BOOLEAN,	// Inheritance flag
								  G_TYPE_STRING)) 	// ID in string format (unique)
{
	loadKeys();
	load(source);
}

int SREntity::getHighestId() {
	int id = 0;
	
	for (StimResponseMap::iterator i = _list.begin(); i != _list.end(); i++) {
		if (i->first > id) {
			id = i->first;
		}
	}
	return id;
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
	
	// Create a new map with all the stims defined directly on the entity
	StimResponseMap _entityStims;
	
	// Scan the entity itself after the class has been searched
	SRPropertyLoader entityVisitor(_keys, _entityStims, _warnings);
	source->forEachKeyValue(entityVisitor);
	
	// Now combine the two maps, the id gets incremented, but the internal
	// index of the StimResponse objects remains unchanged.
	for (StimResponseMap::iterator i = _entityStims.begin(); 
		 i != _entityStims.end(); 
		 i++) 
	{
		// Get the Id of the entity stim
		int id = i->first;
		
		// This will be the ID of the new object
		int newId = id;
		
		// Is there already such a stim number defined in the inherited ones?
		StimResponseMap::iterator found = _list.find(id);
		if (found != _list.end()) {
			// Get a new ID based on the highest available
			newId = getHighestId() + 1;
		}
		
		// Copy the StimResponse over to the member _list
		_list[newId] = i->second;
	}
	
	// Populate the liststore
	updateListStore();
}

void SREntity::updateListStore() {
	// Clear all the items from the liststore
	gtk_list_store_clear(_listStore);
	
	// Now populate the liststore
	GtkTreeIter iter;
	
	for (StimResponseMap::iterator i = _list.begin(); i!= _list.end(); i++) {
		int id = i->first;
		
		gtk_list_store_append(_listStore, &iter);
		// Store the ID into the liststore
		gtk_list_store_set(_listStore, &iter, 
						   IDSTR_COL, intToStr(id).c_str(),
						   -1);
		
		// And write the rest of the data to the row
		StimResponse& sr = i->second;
		writeToListStore(&iter, sr);
	}
}

void SREntity::save(Entity* target) {
	if (target == NULL) {
		return;
	}
}

GtkTreeIter SREntity::getIterForId(int id) {
	// Setup the selectionfinder to search for the id string
	gtkutil::TreeModel::SelectionFinder finder(intToStr(id), IDSTR_COL);
	
	gtk_tree_model_foreach(
		GTK_TREE_MODEL(_listStore), 
		gtkutil::TreeModel::SelectionFinder::forEach, 
		&finder
	);
	
	return finder.getIter();
}

void SREntity::writeToListStore(GtkTreeIter* iter, StimResponse& sr) {
	StimType stimType = _stimTypes.get(sr.get("type"));
		
	std::string stimTypeStr = stimType.caption;
	stimTypeStr += (sr.inherited()) ? " (inherited) " : "";
		
	std::string classIcon = (sr.get("class") == "R") ? ICON_RESPONSE : ICON_STIM; 
	
	// The S/R index (the N in sr_class_N)
	int index = sr.getIndex();
	
	gtk_list_store_set(_listStore, iter, 
						INDEX_COL, index,
						CLASS_COL, gtkutil::getLocalPixbufWithMask(classIcon),
						CAPTION_COL, stimTypeStr.c_str(),
						ICON_COL, gtkutil::getLocalPixbufWithMask(stimType.icon),
						INHERIT_COL, sr.inherited(),
						-1);
}

void SREntity::setProperty(int id, const std::string& key, const std::string& value) {
	// First, propagate the SR set() call
	StimResponse& sr = get(id);
	sr.set(key, value);
	
	GtkTreeIter iter = getIterForId(id);
	writeToListStore(&iter, sr);
}

StimResponse& SREntity::get(int id) {
	StimResponseMap::iterator i = _list.find(id);
	
	if (i != _list.end()) {
		return i->second;
	}
	else {
		return _emptyStimResponse;
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
