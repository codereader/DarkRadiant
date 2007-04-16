#include "SREntity.h"

#include "iregistry.h"
#include "iundo.h"
#include "itextstream.h"
#include "ieclass.h"
#include "entitylib.h"
#include "gtkutil/image.h"
#include "gtkutil/TreeModel.h"
#include <gtk/gtk.h>

#include "SRPropertyLoader.h"
#include "SRPropertyRemover.h"
#include "SRPropertySaver.h"

#include <iostream>

	namespace {
		const std::string RKEY_STIM_PROPERTIES = 
			"game/stimResponseSystem/properties//property";
	}

SREntity::SREntity(Entity* source, StimTypes& stimTypes) :
	_stimStore(gtk_list_store_new(NUM_COLS, 
								  G_TYPE_INT,		// S/R index
								  GDK_TYPE_PIXBUF, 	// Type String
								  G_TYPE_STRING, 	// Caption String
								  GDK_TYPE_PIXBUF,	// Icon
								  G_TYPE_BOOLEAN,	// Inheritance flag
								  G_TYPE_INT,		// ID (unique)
								  G_TYPE_STRING)),	// Text colour
	_responseStore(gtk_list_store_new(NUM_COLS, 
								  G_TYPE_INT,		// S/R index
								  GDK_TYPE_PIXBUF, 	// Type String
								  G_TYPE_STRING, 	// Caption String
								  GDK_TYPE_PIXBUF,	// Icon
								  G_TYPE_BOOLEAN,	// Inheritance flag
								  G_TYPE_INT,		// ID (unique)
								  G_TYPE_STRING)),	// Text colour
	_stimTypes(stimTypes)
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

int SREntity::getHighestIndex() {
	int index = 0;
	
	for (StimResponseMap::iterator i = _list.begin(); i != _list.end(); i++) {
		if (i->second.getIndex() > index) {
			index = i->second.getIndex();
		}
	}
	return index;
}

void SREntity::load(Entity* source) {
	// Clear all the items from the liststore
	gtk_list_store_clear(_stimStore);
	gtk_list_store_clear(_responseStore);
	
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
	
	// Scan the entity itself after the class has been searched
	source->forEachKeyValue(visitor);
	
	// Populate the liststore
	updateListStores();
}

void SREntity::remove(int id) {
	StimResponseMap::iterator found = _list.find(id);
	
	if (found != _list.end() && !found->second.inherited()) {
		_list.erase(found);
		updateListStores();
	}
}

int SREntity::duplicate(int fromId) {
	StimResponseMap::iterator found = _list.find(fromId);
	
	if (found != _list.end()) {
		int id = getHighestId() + 1;
		int index = getHighestIndex() + 1;
		
		// Copy the object to the new id
		_list[id] = found->second;
		// Set the index and the inheritance status
		_list[id].setInherited(false);
		_list[id].setIndex(index);
		
		// Rebuild the liststores
		updateListStores();
		
		return id;
	}
	
	return -1;
}

void SREntity::updateListStores() {
	// Clear all the items from the liststore
	gtk_list_store_clear(_stimStore);
	gtk_list_store_clear(_responseStore);
	
	// Now populate the liststore
	GtkTreeIter iter;
	
	for (StimResponseMap::iterator i = _list.begin(); i!= _list.end(); i++) {
		int id = i->first;
		
		GtkListStore* targetListStore;
		
		// And write the rest of the data to the row
		StimResponse& sr = i->second;
		targetListStore = (sr.get("class") == "S") ? _stimStore : _responseStore;
		
		gtk_list_store_append(targetListStore, &iter);
		// Store the ID into the liststore
		gtk_list_store_set(targetListStore, &iter, 
						   ID_COL, id,
						   -1);
		
		writeToListStore(targetListStore, &iter, sr);
	}
}

int SREntity::add() {
	int id = getHighestId() + 1;
	int index = getHighestIndex() + 1;
	
	// Create a new StimResponse object 
	_list[id] = StimResponse();
	// Set the index and the inheritance status
	_list[id].setInherited(false);
	_list[id].setIndex(index);
	_list[id].set("class", "S");
	
	return id;
}

void SREntity::cleanEntity(Entity* target) {
	// Clean the entity from all the S/R spawnargs
	SRPropertyRemover remover(target, _keys);
	target->forEachKeyValue(remover);
	
	// scope ends here, SRPropertyRemover's destructor
	// will now delete the keys
}

void SREntity::save(Entity* target) {
	if (target == NULL) {
		return;
	}
	
	// Scoped undo object
	UndoableCommand command("setStimResponse");
	
	// Remove the S/R spawnargs from the entity
	cleanEntity(target);
	
	// Setup the saver object
	SRPropertySaver saver(target, _keys);
	for (StimResponseMap::iterator i = _list.begin(); i != _list.end(); i++) {
		saver.visit(i->second);
	}
}

GtkTreeIter SREntity::getIterForId(GtkListStore* targetStore, int id) {
	// Setup the selectionfinder to search for the id string
	gtkutil::TreeModel::SelectionFinder finder(id, ID_COL);
	
	gtk_tree_model_foreach(
		GTK_TREE_MODEL(targetStore), 
		gtkutil::TreeModel::SelectionFinder::forEach, 
		&finder
	);
	
	return finder.getIter();
}

void SREntity::writeToListStore(GtkListStore* targetListStore, GtkTreeIter* iter, StimResponse& sr) {
	StimType stimType = _stimTypes.get(sr.get("type"));
		
	std::string stimTypeStr = stimType.caption;
	stimTypeStr += (sr.inherited()) ? " (inherited) " : "";
	
	std::string classIcon = (sr.get("class") == "R") ? ICON_RESPONSE : ICON_STIM;
	classIcon += (sr.inherited()) ? SUFFIX_INHERITED : "";
	classIcon += (sr.get("state") != "1") ? SUFFIX_INACTIVE : "";
	classIcon += SUFFIX_EXTENSION;
	
	gtk_list_store_set(targetListStore, iter, 
						INDEX_COL, sr.getIndex(),
						CLASS_COL, gtkutil::getLocalPixbufWithMask(classIcon),
						CAPTION_COL, stimTypeStr.c_str(),
						ICON_COL, gtkutil::getLocalPixbufWithMask(stimType.icon),
						INHERIT_COL, sr.inherited(),
						COLOUR_COLUMN, (sr.inherited() ? "#707070" : "#000000"),
						-1);
}

void SREntity::setProperty(int id, const std::string& key, const std::string& value) {
	// First, propagate the SR set() call
	StimResponse& sr = get(id);
	sr.set(key, value);
	
	GtkListStore* targetStore = (sr.get("class") == "S") ? _stimStore : _responseStore;
	
	GtkTreeIter iter = getIterForId(targetStore, id);
	writeToListStore(targetStore, &iter, sr);
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

GtkListStore* SREntity::getStimStore() {
	return _stimStore;
}

GtkListStore* SREntity::getResponseStore() {
	return _responseStore;
}

// static key loader
void SREntity::loadKeys() {
	xml::NodeList propList = GlobalRegistry().findXPath(RKEY_STIM_PROPERTIES);
	
	for (unsigned int i = 0; i < propList.size(); i++) {
		// Create a new key and set the key name / class string
		SRKey newKey;
		newKey.key = propList[i].getAttributeValue("name");
		newKey.classes = propList[i].getAttributeValue("classes");
		
		// Add the key to the list
		_keys.push_back(newKey);
	}
}
