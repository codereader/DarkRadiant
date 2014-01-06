#include "ConversationEntity.h"

#include "i18n.h"
#include "itextstream.h"
#include "ientity.h"

#include "string/convert.h"
#include <boost/algorithm/string/classification.hpp>

#include "ConversationKeyExtractor.h"
#include "ConversationCommandLibrary.h"

namespace conversation {

// Constructor
ConversationEntity::ConversationEntity(scene::INodePtr node) :
	_entityNode(node)
{
	Entity* entity = Node_getEntity(node);
	assert(entity != NULL);

	// Use an conversationKeyExtractor to populate the ConversationMap from the keys
	// on the entity
	ConversationKeyExtractor extractor(_conversations);
	entity->forEachKeyValue(extractor);
}

// Delete the entity's world node
void ConversationEntity::deleteWorldNode() {
	// Try to convert the weak_ptr reference to a shared_ptr
	scene::INodePtr node = _entityNode.lock();

	if (node != NULL && node->getParent() != NULL) {
		node->getParent()->removeChildNode(node);
	}
}

// Add a new conversation
void ConversationEntity::addConversation() {
	// Locate the first unused id
	int index = 1;
	while (_conversations.find(index) != _conversations.end()) {
		++index;
	}

	// Insert a new conversation at this ID.
	Conversation o;
	o.name = _("New Conversation");
	_conversations.insert(ConversationMap::value_type(index, o));
}

void ConversationEntity::deleteConversation(int index) {
	// Look up the conversation with the given index
	ConversationMap::iterator i = _conversations.find(index);

	if (i == _conversations.end()) {
		// not found, nothing to do
		return;
	}

	// Delete the found element
	_conversations.erase(i++);

	// Then iterate all the way to the highest index
	while (i != _conversations.end()) {
		// Decrease the index of this conversation
		int newIndex = i->first - 1;
		// Copy the conversation into a temporary object
		Conversation temp = i->second;

		// Remove the old one
		_conversations.erase(i++);

		// Re-insert with new index
		_conversations.insert(
			ConversationMap::value_type(newIndex, temp)
		);
	}
}

// Populate a list store with conversations
void ConversationEntity::populateListStore(const Glib::RefPtr<Gtk::ListStore>& store,
										   const ConversationColumns& columns) const
{
	for (ConversationMap::const_iterator i = _conversations.begin();
		 i != _conversations.end();
		 ++i)
	{
		Gtk::TreeModel::Row row = *store->append();

		row[columns.index] = i->first;
		row[columns.name] = i->second.name;
	}
}

void ConversationEntity::clearEntity(Entity* entity) {
	// Get all keyvalues matching the "obj" prefix.
	Entity::KeyValuePairs keyValues = entity->getKeyValuePairs("conv_");

	for (Entity::KeyValuePairs::const_iterator i = keyValues.begin();
		 i != keyValues.end(); ++i)
	{
		// Set the spawnarg to empty, which is equivalent to a removal
		entity->setKeyValue(i->first, "");
	}
}

// Write out conversations to entity keyvals
void ConversationEntity::writeToEntity() {
	// Try to convert the weak_ptr reference to a shared_ptr
	Entity* entity = Node_getEntity(_entityNode.lock());
	assert(entity != NULL);

	// greebo: Remove all conversation-related spawnargs first
	clearEntity(entity);

	for (ConversationMap::const_iterator i = _conversations.begin();
		 i != _conversations.end();
		 ++i)
	{
		// Obtain the conversation and construct the key prefix from the index
		const Conversation& conv = i->second;
		std::string prefix = "conv_" + string::to_string(i->first) + "_";

		// Set the entity keyvalues
		entity->setKeyValue(prefix + "name", conv.name);
		entity->setKeyValue(prefix + "actors_must_be_within_talkdistance",
			conv.actorsMustBeWithinTalkdistance ? "1" : "0");
		entity->setKeyValue(prefix + "talk_distance", string::to_string(conv.talkDistance));
		entity->setKeyValue(prefix + "actors_always_face_each_other_while_talking",
			conv.actorsAlwaysFaceEachOther ? "1" : "0");
		entity->setKeyValue(prefix + "max_play_count", string::to_string(conv.maxPlayCount));

		// Write the actor list
		for (Conversation::ActorMap::const_iterator a = conv.actors.begin();
			 a != conv.actors.end(); ++a)
		{
			std::string actorKey = prefix + "actor_" + string::to_string(a->first);
			entity->setKeyValue(actorKey, a->second);
		}

		// Write all the commands
		for (Conversation::CommandMap::const_iterator c = conv.commands.begin();
			 c != conv.commands.end(); ++c)
		{
			std::string cmdPrefix = prefix + "cmd_" + string::to_string(c->first) + "_";

			try {
				const ConversationCommandInfo& cmdInfo =
					ConversationCommandLibrary::Instance().findCommandInfo(c->second->type);

				entity->setKeyValue(cmdPrefix + "type", cmdInfo.name);
				entity->setKeyValue(cmdPrefix + "actor", string::to_string(c->second->actor));
				entity->setKeyValue(cmdPrefix + "wait_until_finished", c->second->waitUntilFinished ? "1" : "0");

				for (ConversationCommand::ArgumentMap::const_iterator a = c->second->arguments.begin();
					a != c->second->arguments.end(); ++a)
				{
					entity->setKeyValue(cmdPrefix + "arg_" + string::to_string(a->first), a->second);
				}
			}
			catch (std::runtime_error e) {
				rError() << "Unrecognised conversation command ID: " << c->second->type << std::endl;
			}
		}
	}
}

} // namespace conversations
