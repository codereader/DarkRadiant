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
								  G_TYPE_INT,		// ID 
								  GDK_TYPE_PIXBUF, 	// Type String
								  G_TYPE_STRING, 	// Caption String
								  GDK_TYPE_PIXBUF,	// Icon
								  G_TYPE_BOOLEAN,	// Inheritance flag
								  G_TYPE_STRING)) 	// ID in string format
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
	
	// Scan the entity itself after the class has been searched
	source->forEachKeyValue(visitor);
	
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
		StimType stimType = _stimTypes.get(i->second.get("type"));
		
		std::string stimTypeStr = stimType.caption;
		stimTypeStr += (i->second.inherited()) ? " (inherited) " : "";
		
		std::string classIcon = 
			(i->second.get("class") == "R") ? ICON_RESPONSE : ICON_STIM; 
		
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 
							ID_COL, id,
							CLASS_COL, gtkutil::getLocalPixbufWithMask(classIcon),
							CAPTION_COL, stimTypeStr.c_str(),
							ICON_COL, gtkutil::getLocalPixbufWithMask(stimType.icon),
							INHERIT_COL, i->second.inherited(),
							IDSTR_COL, intToStr(id).c_str(),
							-1);
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
	
	GtkTreePath* found = finder.getPath();
	
	GtkTreeIter iter;
	
	if (found != NULL) {
		gtk_tree_model_get_iter_from_string(
			GTK_TREE_MODEL(_listStore),
			&iter,
			gtk_tree_path_to_string(found)
		);
	}
	
	return iter;
}

void SREntity::setProperty(int id, const std::string& key, const std::string& value) {
	// First, propagate the SR set() call
	StimResponse& sr = get(id);
	sr.set(key, value);
	
	GtkTreeIter iter = getIterForId(id);
	
	StimType stimType = _stimTypes.get(sr.get("type"));
		
	std::string stimTypeStr = stimType.caption;
	stimTypeStr += (sr.inherited()) ? " (inherited) " : "";
		
	std::string classIcon = (sr.get("class") == "R") ? ICON_RESPONSE : ICON_STIM; 
	
	gtk_list_store_set(_listStore, &iter, 
						ID_COL, id,
						CLASS_COL, gtkutil::getLocalPixbufWithMask(classIcon),
						CAPTION_COL, stimTypeStr.c_str(),
						ICON_COL, gtkutil::getLocalPixbufWithMask(stimType.icon),
						INHERIT_COL, sr.inherited(),
						IDSTR_COL, intToStr(id).c_str(),
						-1);
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
