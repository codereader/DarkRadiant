#include "ObjectiveEntityFinder.h"

#include "ientity.h"

namespace objectives
{

bool ObjectiveEntityFinder::pre(const scene::INodePtr& node)
{
    // Get the entity and check the classname
    Entity* ePtr = Node_getEntity(node);
    if (!ePtr)
        return true;

    // We have an entity at this point

    if (ePtr->getKeyValue("classname") == "worldspawn")
    {
        _worldSpawn = ePtr;
        return false; // Don't traverse worldspawn children
    }

    // Check for objective entity or worldspawn
    for (std::vector<std::string>::iterator i = _classNames.begin();
         i != _classNames.end();
         ++i)
    {
        if (ePtr->getKeyValue("classname") == *i)
        {
            // Construct the display string
            std::string name = ePtr->getKeyValue("name");

            // Add the entity to the list
            Gtk::TreeModel::Row row = *_store->append();

            row[_columns.displayName] = (boost::format(_("<b>%s</b> at [ %s ]")) % name % ePtr->getKeyValue("origin")).str();
            row[_columns.entityName] = name;
            row[_columns.startActive] = false;

            // Construct an ObjectiveEntity with the node, and add to the map
            ObjectiveEntityPtr oe(new ObjectiveEntity(node));
            _map.insert(ObjectiveEntityMap::value_type(name, oe));

            break;
        }
    }

    return false; // don't traverse entity children
}

}



