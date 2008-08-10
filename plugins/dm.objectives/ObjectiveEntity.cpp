#include "ObjectiveEntity.h"
#include "ObjectiveKeyExtractor.h"
#include "TargetList.h"

#include "scenelib.h"

#include "string/string.h"
#include <boost/lexical_cast.hpp>

namespace objectives {

// Constructor
ObjectiveEntity::ObjectiveEntity(scene::INodePtr node) :
	_entityNode(node)
{ 
	Entity* entity = Node_getEntity(node);
	assert(entity != NULL);
	
	// Use an ObjectiveKeyExtractor to populate the ObjectiveMap from the keys
	// on the entity
	ObjectiveKeyExtractor extractor(_objectives);
	entity->forEachKeyValue(extractor);
}

// Delete the entity's world node
void ObjectiveEntity::deleteWorldNode() {
	// Try to convert the weak_ptr reference to a shared_ptr
	scene::INodePtr node = _entityNode.lock();

	if (node != NULL) {
		GlobalSceneGraph().root()->removeChildNode(node);
	}
}

// Add a new objective
void ObjectiveEntity::addObjective() {
	// Locate the first unused id
	int index = 1;
	while (_objectives.find(index) != _objectives.end())
		++index;
		
	// Insert a new Objective at this ID.
	Objective o;
	o.description = "New objective " + boost::lexical_cast<std::string>(index);
	_objectives.insert(ObjectiveMap::value_type(index, o));
}

// Test for targeting
bool ObjectiveEntity::isOnTargetList(const TargetList& list) const {
	// Try to convert the weak_ptr reference to a shared_ptr
	Entity* entity = Node_getEntity(_entityNode.lock());
	assert(entity != NULL);

	return list.isTargeted(entity);
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

// Write the Components from a single Objective to the underlying entity
void ObjectiveEntity::writeComponents(Entity* entity,
    const std::string& keyPrefix, const Objective& obj
)
{
    assert(entity != NULL);

    for (Objective::ComponentMap::const_iterator i = obj.components.begin();
         i != obj.components.end();
         ++i)
    {
        const Component& c = i->second;

        // Component prefix is like obj1_2_blah
		std::string prefix = keyPrefix + intToStr(i->first) + "_";

        // Write out Component keyvals
        entity->setKeyValue(prefix + "state", c.isSatisfied() ? "1" : "0");
        entity->setKeyValue(prefix + "not", c.isInverted() ? "1" : "0");
        entity->setKeyValue(
            prefix + "irreversible", c.isIrreversible() ? "1": "0"
        );
        entity->setKeyValue(
            prefix + "player_responsible", c.isPlayerResponsible() ? "1" : "0"
        );
        entity->setKeyValue(prefix + "type", c.getType().getName());

		entity->setKeyValue(prefix + "clock_interval", 
			c.getClockInterval() > 0 ? floatToStr(c.getClockInterval()) : "");

        // Write out Specifier keyvals
		for (int i = Specifier::FIRST_SPECIFIER; i < Specifier::MAX_SPECIFIERS; i++)
		{
			// The specifier index of the spawnargs is starting from 1, not 0
			std::string indexStr = intToStr(i + 1);

			SpecifierPtr spec = c.getSpecifier(static_cast<Specifier::SpecifierNumber>(i));

			if (spec != NULL) {
				entity->setKeyValue(prefix + "spec" + indexStr, spec->getType().getName());
				entity->setKeyValue(prefix + "spec_val" + indexStr, spec->getValue());
			}
		}

		// Export the component arguments
		entity->setKeyValue(prefix + "args", c.getArgumentString());
    }
}

// Write out Objectives to entity keyvals
void ObjectiveEntity::writeToEntity() {
	// Try to convert the weak_ptr reference to a shared_ptr
	Entity* entity = Node_getEntity(_entityNode.lock());
	assert(entity != NULL);
	
	for (ObjectiveMap::const_iterator i = _objectives.begin();
		 i != _objectives.end();
		 ++i) 
	{
		// Obtain the Objective and construct the key prefix from the index
		const Objective& o = i->second;
		std::string prefix = "obj" + intToStr(i->first) + "_";
		
		// Set the entity keyvalues
		entity->setKeyValue(prefix + "desc", o.description);
		entity->setKeyValue(prefix + "ongoing", o.ongoing ? "1" : "0");
		entity->setKeyValue(prefix + "visible", o.visible ? "1" : "0");
		entity->setKeyValue(prefix + "mandatory", o.mandatory ? "1" : "0");
		entity->setKeyValue(prefix + "irreversible", 
							 o.irreversible ? "1" : "0");
		entity->setKeyValue(prefix + "state", intToStr(o.state));

		// Write an empty "objN_difficulty" value when this objective applies to all levels
		entity->setKeyValue(prefix + "difficulty", 
			o.difficultyLevel != -1 ? intToStr(o.difficultyLevel) : "");

		entity->setKeyValue(prefix + "enabling_objs", o.enablingObjs);

        // Write the Components for this Objective
        writeComponents(entity, prefix, o);
	}	
}

} // namespace objectives
