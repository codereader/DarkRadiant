#ifndef CONVERSATIONENTITYFINDER_H_
#define CONVERSATIONENTITYFINDER_H_

#include "i18n.h"
#include <gtkmm/liststore.h>
#include <string>

#include "ConversationEntity.h"

namespace conversation {

/**
 * Visitor class to locate and list any <b>atdm:conversation_info</b> entities in
 * the current map.
 *
 * The ConversationEntityFinder will visit each scenegraph node in turn, as per the
 * behaviour of a scenegraph walker. The classname of each entity visited is
 * tested against a given value (passed in during construction) which identifies
 * it as a Conversation entity, and if the test is successful, the entity's
 * details are added to the target ConversationEntityMap and GtkListStore objects
 * to be populated.
 */
class ConversationEntityFinder :
	public scene::NodeVisitor
{
	// Name of entity class we are looking for
	std::string _className;

	// ListStore to populate with results
	Glib::RefPtr<Gtk::ListStore> _store;
	const ConvEntityColumns& _columns;

	// ConversationEntityMap which we also populate
	ConversationEntityMap& _map;

public:

	/**
	 * Construct a visitor to populate the given store and ConversationEntityMap.
	 *
	 * The GtkListStore provided must contain two columns. The first column
	 * is a G_TYPE_STRING containing the display name of the conversation entity,
	 * which is constructed from the real entity name plus the origin in
	 * brackets for convenience purposes. The second column is a G_TYPE_STRING
	 * containing the raw entity name in the map.
	 *
	 * @param st
	 * The GtkListStore to populate.
	 *
	 * @param map
	 * The ConversationEntityMap to populate.
	 *
	 * @param classname
	 * The text classname used to identify a Conversation entity.
	 */
	ConversationEntityFinder(const Glib::RefPtr<Gtk::ListStore>& st,
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
	bool pre(const scene::INodePtr& node) {

		// Get the entity and check the classname
		Entity* entity = Node_getEntity(node);

		// Check for conversation entity or worldspawn
		if (entity != NULL && entity->getKeyValue("classname") == _className)
		{
			// Construct the display string
			std::string name = entity->getKeyValue("name");
			std::string sDisplay =
				(boost::format(_("<b>%s</b> at [ %s ]")) % name % entity->getKeyValue("origin")).str();

			// Add the entity to the list
			Gtk::TreeModel::Row row = *_store->append();

			row[_columns.displayName] = sDisplay;
			row[_columns.entityName] = name;

			// Construct an ObjectiveEntity with the node, and add to the map
			ConversationEntityPtr ce(new ConversationEntity(node));
			_map.insert(ConversationEntityMap::value_type(name, ce));
		}

		return true;
	}

};

} // namespace conversation

#endif /* CONVERSATIONENTITYFINDER_H_ */
