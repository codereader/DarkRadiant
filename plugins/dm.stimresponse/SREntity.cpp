#include "SREntity.h"

#include "iregistry.h"
#include "entitylib.h"
#include <gtk/gtkliststore.h>

	namespace {
		const unsigned int NUM_MAX_STIMS = 99999;
		const std::string RKEY_STIM_PROPERTIES = 
			"game/stimResponseSystem/properties//property";
			
		
		class SRPropertyLoader :
			public Entity::Visitor
		{
		public:
			SRPropertyLoader();
			
			void visit(const std::string& key, const std::string& value) {
				
			}
		};
	}

SREntity::SREntity(Entity* source) {
	loadKeys();
	load(source);
}

void SREntity::load(Entity* source) {
	if (source == NULL) {
		return;
	}
	
	for (unsigned int i = 0; i < NUM_MAX_STIMS; i++) {
		
	}
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
