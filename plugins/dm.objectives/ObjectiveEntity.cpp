#include "ObjectiveEntity.h"
#include "ObjectiveKeyExtractor.h"
#include "TargetList.h"

#include "ientity.h"
#include "iscenegraph.h"

#include <boost/lexical_cast.hpp>

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

// Delete the entity's world node
void ObjectiveEntity::deleteWorldNode() {
	Node_getTraversable(GlobalSceneGraph().root())->erase(_node);
	_entity = NULL;		
}

// Add a new objective
void ObjectiveEntity::addObjective() {
	
	// Locate the first unused id
	int index = 0;
	while (_objectives.find(index) != _objectives.end())
		++index;
		
	// Insert a new Objective at this ID.
	Objective o;
	o.description = "New objective " + boost::lexical_cast<std::string>(index);
	_objectives.insert(ObjectiveMap::value_type(index, o));
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

// Write out Objectives to entity keyvals
void ObjectiveEntity::writeToEntity() const {
	assert(_entity);
	
	for (ObjectiveMap::const_iterator i = _objectives.begin();
		 i != _objectives.end();
		 ++i) 
	{
		using std::string;
		
		// Obtain the Objective and construct the key prefix from the index
		const Objective& o = i->second;
		string prefix = "obj" + boost::lexical_cast<string>(i->first) + "_";
		
		// Set the entity keyvalues
		_entity->setKeyValue(prefix + "desc", o.description);
		_entity->setKeyValue(prefix + "state", o.startActive ? "1" : "0");
		_entity->setKeyValue(prefix + "ongoing", o.ongoing ? "1" : "0");
		_entity->setKeyValue(prefix + "visible", o.visible ? "1" : "0");
		_entity->setKeyValue(prefix + "mandatory", o.mandatory ? "1" : "0");
		_entity->setKeyValue(prefix + "irreversible", 
							 o.irreversible ? "1" : "0");
		
	}	
}

}
