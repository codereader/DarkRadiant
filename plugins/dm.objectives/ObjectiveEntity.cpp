#include "ObjectiveEntity.h"
#include "ObjectiveKeyExtractor.h"
#include "TargetList.h"

#include "scenelib.h"

#include <boost/lexical_cast.hpp>

namespace objectives
{

// Constructor
ObjectiveEntity::ObjectiveEntity(scene::INodePtr n)
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
	GlobalSceneGraph().root()->removeChildNode(_node);
	_entity = NULL;		
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

// Write the Components from a single Objective to the underlying entity
void ObjectiveEntity::writeComponents(
    const std::string& keyPrefix, const Objective& obj
)
{
    assert(_entity);

    using std::string;
    using boost::lexical_cast;

    for (Objective::ComponentMap::const_iterator i = obj.components.begin();
         i != obj.components.end();
         ++i)
    {
        const Component& c = i->second;

        // Component prefix is like obj1_2_blah
        string prefix = keyPrefix + lexical_cast<string>(i->first) + "_";

        // Write out Component keyvals
        _entity->setKeyValue(prefix + "state", c.isSatisfied() ? "1" : "0");
        _entity->setKeyValue(prefix + "not", c.isInverted() ? "1" : "0");
        _entity->setKeyValue(
            prefix + "irreversible", c.isIrreversible() ? "1": "0"
        );
        _entity->setKeyValue(
            prefix + "player_responsible", c.isPlayerResponsible() ? "1" : "0"
        );
        _entity->setKeyValue(prefix + "type", c.getType().getName());

        // Write out Specifier keyvals for FIRST and SECOND specifiers
        SpecifierPtr spec1 = c.getSpecifier(Specifier::FIRST_SPECIFIER);
        if (spec1)
        {
            _entity->setKeyValue(prefix + "spec1", spec1->getType().getName());
            _entity->setKeyValue(prefix + "spec1_val", spec1->getValue());
        }
        SpecifierPtr spec2 = c.getSpecifier(Specifier::SECOND_SPECIFIER);
        if (spec2)
        {
            _entity->setKeyValue(prefix + "spec2", spec2->getType().getName());
            _entity->setKeyValue(prefix + "spec2_val", spec2->getValue());
        }
    }
}

// Write out Objectives to entity keyvals
void ObjectiveEntity::writeToEntity() 
{
	assert(_entity);
	
	for (ObjectiveMap::const_iterator i = _objectives.begin();
		 i != _objectives.end();
		 ++i) 
	{
		using std::string;
		using boost::lexical_cast;
		
		// Obtain the Objective and construct the key prefix from the index
		const Objective& o = i->second;
		string prefix = "obj" + lexical_cast<string>(i->first) + "_";
		
		// Set the entity keyvalues
		_entity->setKeyValue(prefix + "desc", o.description);
		_entity->setKeyValue(prefix + "ongoing", o.ongoing ? "1" : "0");
		_entity->setKeyValue(prefix + "visible", o.visible ? "1" : "0");
		_entity->setKeyValue(prefix + "mandatory", o.mandatory ? "1" : "0");
		_entity->setKeyValue(prefix + "irreversible", 
							 o.irreversible ? "1" : "0");
		_entity->setKeyValue(prefix + "state", lexical_cast<string>(o.state));

        // Write the Components for this Objective
        writeComponents(prefix, o);
	}	
}

}
