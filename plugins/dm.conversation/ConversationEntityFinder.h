#pragma once

#include "i18n.h"
#include <string>

#include "ConversationEntity.h"

namespace conversation
{

/**
 * Visitor class to locate and list any <b>atdm:conversation_info</b> entities in
 * the current map.
 *
 * The ConversationEntityFinder will visit each scenegraph node in turn, as per the
 * behaviour of a scenegraph walker. The classname of each entity visited is
 * tested against a given value (passed in during construction) which identifies
 * it as a Conversation entity, and if the test is successful, the entity's
 * details are added to the target ConversationEntityMap and TreeModel objects
 * to be populated.
 */
class ConversationEntityFinder :
	public scene::NodeVisitor
{
	// Name of entity class we are looking for
	std::string _className;

	// ListStore to populate with results
	wxutil::TreeModel* _store;
	const ConvEntityColumns& _columns;

	// ConversationEntityMap which we also populate
	ConversationEntityMap& _map;

public:

	/**
	 * Construct a visitor to populate the given store and ConversationEntityMap.
	 *
	 * The TreeModel provided must contain two columns. The first column
	 * is a string containing the display name of the conversation entity,
	 * which is constructed from the real entity name plus the origin in
	 * brackets for convenience purposes. The second column is a string
	 * containing the raw entity name in the map.
	 *
	 * @param st
	 * The TreeModel to populate.
	 *
	 * @param map
	 * The ConversationEntityMap to populate.
	 *
	 * @param classname
	 * The text classname used to identify a Conversation entity.
	 */
	ConversationEntityFinder(wxutil::TreeModel* st,
							 const ConvEntityColumns& columns,
						    ConversationEntityMap& map,
						    const std::string& classname)
	: _className(classname),
	  _store(st),
	  _columns(columns),
	  _map(map)
	{}

	/**
	 * NodeVisitor implementation
	 */
	bool pre(const scene::INodePtr& node)
	{
		// Get the entity and check the classname
		Entity* entity = Node_getEntity(node);

		// Check for conversation entity or worldspawn
		if (entity != NULL && entity->getKeyValue("classname") == _className)
		{
			// Construct the display string
			std::string name = entity->getKeyValue("name");
			std::string sDisplay =
				(boost::format(_("%s at [ %s ]")) % name % entity->getKeyValue("origin")).str();

			// Add the entity to the list
			wxutil::TreeModel::Row row = _store->AddItem();

			row[_columns.displayName] = sDisplay;
			row[_columns.entityName] = name;

			row.SendItemAdded();

			// Construct an ObjectiveEntity with the node, and add to the map
			ConversationEntityPtr ce(new ConversationEntity(node));
			_map.insert(ConversationEntityMap::value_type(name, ce));
		}

		return true;
	}

};

} // namespace conversation
