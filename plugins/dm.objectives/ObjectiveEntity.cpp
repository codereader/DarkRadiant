#include "ObjectiveEntity.h"
#include "ObjectiveKeyExtractor.h"
#include "TargetList.h"

namespace objectives
{

// Constructor
ObjectiveEntity::ObjectiveEntity(scene::Node& n)
: _node(n),
  _entity(Node_getEntity(n))
{ 
	assert(_entity);
	
	// Use an ObjectiveKeyExtractor to populate the ObjectiveMap from the keys
	// on the entity
	ObjectiveKeyExtractor extractor(_objectives);
	_entity->forEachKeyValue(extractor);
}

// Test for targeting
bool ObjectiveEntity::isOnTargetList(const TargetList& list) const {
	assert(_entity);
	return list.isTargeted(_entity);
}

// Populate a list store with objectives
void ObjectiveEntity::populateListStore(GtkListStore* store) const {
	for (ObjectiveMap::const_iterator i = _objectives.begin();
		 i != _objectives.end();
		 ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, i->first, 
						   1, i->second.description.c_str(),
						   -1);	
	}	
}

}
