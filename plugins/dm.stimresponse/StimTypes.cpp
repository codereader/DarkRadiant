#include "StimTypes.h"

#include "iregistry.h"
#include "string/string.h"
#include <gtk/gtkliststore.h>

#include <iostream>

	namespace {
		const std::string RKEY_STIM_DEFINITIONS = "game/stimResponseSystem//stim";
		
		enum {
		  ID_COL,
		  CAPTION_COL,
		  NUM_COLS
		};
	}

StimTypes::StimTypes() {
	// Create a new liststore
	_listStore = gtk_list_store_new(NUM_COLS, G_TYPE_INT, G_TYPE_STRING);
	
	// Find all the relevant nodes
	xml::NodeList stimNodes = GlobalRegistry().findXPath(RKEY_STIM_DEFINITIONS);
	
	for (unsigned int i = 0; i < stimNodes.size(); i++) {
		int id = strToInt(stimNodes[i].getAttributeValue("id"));
		
		Stim newStim;
		newStim.name = stimNodes[i].getAttributeValue("name");
		newStim.caption = stimNodes[i].getAttributeValue("caption");
		newStim.description = stimNodes[i].getAttributeValue("description");
		
		// Add the stim to the map
		_stims[id] = newStim;
		
		GtkTreeIter iter;
		
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 
							ID_COL, id,
							CAPTION_COL, _stims[id].caption.c_str(),
							-1);
	}
}

StimTypes::operator GtkListStore* () {
	return _listStore;
}

Stim StimTypes::get(int id) {
	StimTypeMap::iterator i = _stims.find(id);
	
	if (i != _stims.end()) {
		return i->second;
	}
	else {
		return _emptyStim;
	}
}
