#include "StimTypes.h"

#include "iregistry.h"
#include "string/string.h"
#include "gtkutil/image.h"
#include "gtkutil/TreeModel.h"
#include "entitylib.h"
#include <gtk/gtk.h>

	namespace {
		const std::string RKEY_STIM_DEFINITIONS = 
			"game/stimResponseSystem/stims//stim";
		const std::string RKEY_STORAGE_ECLASS = 
			"game/stimResponseSystem/customStimStorageEClass";
		
		enum {
		  ID_COL,
		  CAPTION_COL,
		  ICON_COL,
		  NAME_COL,
		  NUM_COLS
		};
		
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
	_listStore = gtk_list_store_new(NUM_COLS, 
									G_TYPE_INT, 
									G_TYPE_STRING, 
									GDK_TYPE_PIXBUF,
									G_TYPE_STRING);
	
	// Find all the relevant nodes
	xml::NodeList stimNodes = GlobalRegistry().findXPath(RKEY_STIM_DEFINITIONS);
	
	for (unsigned int i = 0; i < stimNodes.size(); i++) {
		int id = strToInt(stimNodes[i].getAttributeValue("id"));
		
		StimType newStimType;
		newStimType.name = stimNodes[i].getAttributeValue("name");
		newStimType.caption = stimNodes[i].getAttributeValue("caption");
		newStimType.description = stimNodes[i].getAttributeValue("description");
		newStimType.icon = stimNodes[i].getAttributeValue("icon");
		
		// Add the stim to the map
		_stims[id] = newStimType;
		
		GtkTreeIter iter;
		
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 
							ID_COL, id,
							CAPTION_COL, _stims[id].caption.c_str(),
							ICON_COL, gtkutil::getLocalPixbufWithMask(newStimType.icon),
							NAME_COL, _stims[id].name.c_str(),
							-1);
	}
	
	// Load the custom stims from the storage entity
	std::string storageEClass = GlobalRegistry().get(RKEY_STORAGE_ECLASS);
	Entity* storageEntity = findEntityByClass(storageEClass);
	
	if (storageEntity != NULL) {
		
	}
}

StimTypes::operator GtkListStore* () {
	return _listStore;
}

StimTypeMap& StimTypes::getStimMap() {
	return _stims;
}

GtkTreeIter StimTypes::getIterForName(const std::string& name) {
	// Setup the selectionfinder to search for the name string
	gtkutil::TreeModel::SelectionFinder finder(name, NAME_COL);
	
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
