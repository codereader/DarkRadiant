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

SREntity::SREntity(Entity* source) :
	_listStore(gtk_list_store_new(NUM_COLS, 
								  G_TYPE_INT,		// S/R index
								  GDK_TYPE_PIXBUF, 	// Type String
								  G_TYPE_STRING, 	// Caption String
								  GDK_TYPE_PIXBUF,	// Icon
								  G_TYPE_BOOLEAN,	// Inheritance flag
								  G_TYPE_STRING)), 	// ID in string format (unique)
	_scriptStore(gtk_list_store_new(SCR_NUM_COLS,
									G_TYPE_INT,			// id
									G_TYPE_STRING,		// caption
									G_TYPE_STRING,		// name (STIM_FIRE)
									GDK_TYPE_PIXBUF,	// Icon
									G_TYPE_STRING,		// Script String
									G_TYPE_BOOLEAN,		// Inheritance Flag
									G_TYPE_STRING))		// ID in string format
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
	gtk_list_store_clear(_listStore);
	gtk_list_store_clear(_scriptStore);
	
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
	SRPropertyLoader visitor(_keys, _list, _scripts, _warnings);
	eclass->forEachClassAttribute(visitor);
	
	// Create a new map with all the stims/scripts defined directly on the entity
	StimResponseMap entityStims;
		
	// Scan the entity itself after the class has been searched
	SRPropertyLoader entityVisitor(_keys, entityStims, _scripts, _warnings);
	source->forEachKeyValue(entityVisitor);
	
	// Now combine the two S/R maps, the id gets incremented, but the internal
	// index of the StimResponse objects remains unchanged.
	for (StimResponseMap::iterator i = entityStims.begin(); 
		 i != entityStims.end(); 
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

void SREntity::remove(int id) {
	StimResponseMap::iterator found = _list.find(id);
	
	if (found != _list.end() && !found->second.inherited()) {
		_list.erase(found);
		updateListStore();
	}
}

void SREntity::removeScript(int id) {
	if (id > 0 && !_scripts[id].inherited) {
		// Remove the item from the vector
		_scripts.erase(_scripts.begin() + id);
		updateListStore();
	}
}

void SREntity::setScript(int scriptId, const std::string& newScript) {
	_scripts[scriptId].script = newScript;
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
	
	// Clear the scripts
	gtk_list_store_clear(_scriptStore);
	
	for (unsigned int i = 0; i < _scripts.size(); i++) {
		ResponseScript& s = _scripts[i];
		
		gtk_list_store_append(_scriptStore, &iter);
		// Store the ID into the liststore
		gtk_list_store_set(_scriptStore, &iter, 
						   SCR_ID_COL, i,
						   SCR_IDSTR_COL, intToStr(i).c_str(),
						   -1);
		
		// And write the rest of the data to the row
		writeToScriptStore(&iter, s);
	}
}

int SREntity::add() {
	int id = getHighestId() + 1;
	int index = getHighestIndex() + 1;
	
	// Create a new StimResponse object 
	_list[id] = StimResponse();
	// Set the index and the inheritance status
	_list[id].setIndex(index);
	_list[id].set("class", "S");
	_list[id].setInherited(false);
	
	return id;
}

void SREntity::cleanEntity(Entity* target) {
	// Clean the entity from all the S/R spawnargs
	SRPropertyRemover remover(target, _keys);
	target->forEachKeyValue(remover);
	
	// scope ends here, SRPropertyRemover's destructor
	// will delete the keys here
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

void SREntity::writeToScriptStore(GtkTreeIter* iter, ResponseScript& script) {
	StimType stim = _stimTypes.get(script.stimType);
	
	// Store the ID into the liststore
	gtk_list_store_set(_scriptStore, iter, 
					    SCR_CAPTION_COL, stim.caption.c_str(),
						SCR_NAME_COL, stim.name.c_str(),
						SCR_ICON_COL, gtkutil::getLocalPixbufWithMask(stim.icon),
						SCR_SCRIPT_COL, script.script.c_str(),
						SCR_INHERIT_COL, script.inherited,
					   -1);
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

GtkListStore* SREntity::getStimResponseStore() {
	return _listStore;
}

GtkListStore* SREntity::getScriptStore() {
	return _scriptStore;
}

// static key loader
void SREntity::loadKeys() {
	xml::NodeList propList = GlobalRegistry().findXPath(RKEY_STIM_PROPERTIES);
	
	for (unsigned int i = 0; i < propList.size(); i++) {
		_keys.push_back(propList[i].getAttributeValue("name"));
	}
}
