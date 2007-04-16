#include "StimTypes.h"

#include "iregistry.h"
#include "string/string.h"
#include "gtkutil/image.h"
#include "gtkutil/TreeModel.h"
#include "entitylib.h"
#include "SREntity.h"
#include <gtk/gtk.h>
#include <boost/algorithm/string/predicate.hpp>

	namespace {
		const std::string RKEY_STIM_DEFINITIONS = 
			"game/stimResponseSystem/stims//stim";
		const std::string RKEY_STORAGE_ECLASS = 
			"game/stimResponseSystem/customStimStorageEClass";
		const std::string RKEY_STORAGE_PREFIX = 
			"game/stimResponseSystem/customStimKeyPrefix";
		const std::string RKEY_LOWEST_CUSTOM_STIM_ID = 
			"game/stimResponseSystem/lowestCustomStimId";
		
		/* greebo: Finds an entity with the given classname
		 */
		Entity* findEntityByClass(const std::string& className) {
			// Instantiate a walker to find the entity
			EntityFindByClassnameWalker walker(className);
			
			// Walk the scenegraph
			GlobalSceneGraph().traverse(walker);
			
			return walker.getEntity();
		}
	}

StimTypes::StimTypes() {
	// Create a new liststore
	_listStore = gtk_list_store_new(ST_NUM_COLS, 
									G_TYPE_INT, 
									G_TYPE_STRING, 
									GDK_TYPE_PIXBUF,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_BOOLEAN);
	
	// Find all the relevant nodes
	xml::NodeList stimNodes = GlobalRegistry().findXPath(RKEY_STIM_DEFINITIONS);
	
	for (unsigned int i = 0; i < stimNodes.size(); i++) {
		// Add the new stim type
		add(strToInt(stimNodes[i].getAttributeValue("id")), 
			stimNodes[i].getAttributeValue("name"),
			stimNodes[i].getAttributeValue("caption"),
			stimNodes[i].getAttributeValue("description"),
			stimNodes[i].getAttributeValue("icon"),
			false	// non-custom stim
		);
	}
	
	// Load the custom stims from the storage entity
	std::string storageEClass = GlobalRegistry().get(RKEY_STORAGE_ECLASS);
	Entity* storageEntity = findEntityByClass(storageEClass);
	
	if (storageEntity != NULL) {
		// Visit each keyvalue with the <self> class as visitor 
		storageEntity->forEachKeyValue(*this);
	}
}

void StimTypes::add(int id, 
					const std::string& name,
					const std::string& caption,
					const std::string& description,
					const std::string& icon,
					bool custom)
{
	StimType newStimType;
	newStimType.name = name;
	newStimType.caption = caption;
	newStimType.description = description;
	newStimType.icon = icon;
	newStimType.custom = custom;
	
	// Add the stim to the map
	_stims[id] = newStimType;
	
	GtkTreeIter iter;
	
	// Combine the ID and the caption
	std::string captionPlusId = _stims[id].caption + " (" + intToStr(id) + ")";
	
	gtk_list_store_append(_listStore, &iter);
	gtk_list_store_set(_listStore, &iter, 
						ST_ID_COL, id,
						ST_CAPTION_COL, _stims[id].caption.c_str(),
						ST_CAPTION_PLUS_ID_COL, captionPlusId.c_str(),
						ST_ICON_COL, gtkutil::getLocalPixbufWithMask(newStimType.icon),
						ST_NAME_COL, _stims[id].name.c_str(),
						ST_CUSTOM_COL, custom,
						-1);
}

void StimTypes::visit(const std::string& key, const std::string& value) {
	std::string prefix = GlobalRegistry().get(RKEY_STORAGE_PREFIX);
	int lowestCustomId = GlobalRegistry().getInt(RKEY_LOWEST_CUSTOM_STIM_ID);
	
	if (boost::algorithm::starts_with(key, prefix)) {
		// Extract the stim name from the key (the part after the prefix) 
		std::string idStr = key.substr(prefix.size());
		int id = strToInt(idStr);
		std::string stimName= value;
		
		if (id < lowestCustomId) {
			globalErrorStream() << "Warning: custom stim Id " << id << " is lower than " 
								<< lowestCustomId << "\n";
		}
		
		// Add this as new stim type
		add(id,
			idStr,	// The name is the id in string format: e.g. "1002"
			stimName,
			"Custom Stim",
			ICON_CUSTOM_STIM,
			true	// custom stim
		);
	}
}

StimTypes::operator GtkListStore* () {
	return _listStore;
}

StimTypeMap& StimTypes::getStimMap() {
	return _stims;
}

int StimTypes::getFreeCustomStimId() {
	int freeId = GlobalRegistry().getInt(RKEY_LOWEST_CUSTOM_STIM_ID);
	
	StimTypeMap::iterator found = _stims.find(freeId);
	while (found != _stims.end()) {
		freeId++;
		found = _stims.find(freeId);
	}
	
	return freeId;
}

GtkTreeIter StimTypes::getIterForName(const std::string& name) {
	// Setup the selectionfinder to search for the name string
	gtkutil::TreeModel::SelectionFinder finder(name, ST_NAME_COL);
	
	gtk_tree_model_foreach(
		GTK_TREE_MODEL(_listStore), 
		gtkutil::TreeModel::SelectionFinder::forEach, 
		&finder
	);
	
	return finder.getIter();
}

StimType StimTypes::get(int id) {
	StimTypeMap::iterator i = _stims.find(id);
	
	if (i != _stims.end()) {
		return i->second;
	}
	else {
		return _emptyStimType;
	}
}

std::string StimTypes::getFirstName() {
	StimTypeMap::iterator i = _stims.begin();
	
	return (i != _stims.end()) ? i->second.name : "noname";
}

StimType StimTypes::get(const std::string& name) {
	for (StimTypeMap::iterator i = _stims.begin(); i!= _stims.end(); i++) {
		if (i->second.name == name) {
			return i->second;
		}
	}
	// Nothing found
	return _emptyStimType;
}
