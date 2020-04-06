#include "ConversationEntity.h"

#include <limits>

#include "i18n.h"
#include "itextstream.h"
#include "ientity.h"

#include "string/convert.h"

#include "ConversationKeyExtractor.h"
#include "ConversationCommandLibrary.h"

namespace conversation
{

// Constructor
ConversationEntity::ConversationEntity(const scene::INodePtr& node) :
	_entityNode(node)
{
	Entity* entity = Node_getEntity(node);
	assert(entity != nullptr);

	// Use an conversationKeyExtractor to populate the ConversationMap from the keys
	// on the entity
	ConversationKeyExtractor extractor(_conversations);
	entity->forEachKeyValue(extractor);
}

// Delete the entity's world node
void ConversationEntity::deleteWorldNode() 
{
	// Try to convert the weak_ptr reference to a shared_ptr
	scene::INodePtr node = _entityNode.lock();

	if (node && node->getParent())
	{
		node->getParent()->removeChildNode(node);
	}
}

int ConversationEntity::getHighestIndex()
{
	if (_conversations.empty())
	{
		return -1;
	}

	return _conversations.rbegin()->first;
}

// Add a new conversation
void ConversationEntity::addConversation() 
{
	// Locate the first unused id
	int index = 1;
	while (_conversations.find(index) != _conversations.end()) 
	{
		if (index == std::numeric_limits<int>::max())
		{
			rError() << "Ran out of conversation indices." << std::endl;
			throw new std::runtime_error("Ran out of conversation indices.");
		}

		++index;
	}

	// Insert a new conversation at this ID.
	Conversation o;
	o.name = _("New Conversation");
	_conversations.insert(ConversationMap::value_type(index, o));
}

void ConversationEntity::deleteConversation(int index) 
{
	// Look up the conversation with the given index
	auto i = _conversations.find(index);

	if (i == _conversations.end()) 
	{
		// not found, nothing to do
		return;
	}

	// Delete the found element and move the iterator past it
	_conversations.erase(i++);

	// Then iterate all the way to the highest index
	while (i != _conversations.end()) 
	{
		// Decrease the index of this conversation
		int newIndex = i->first - 1;
		// Copy the conversation into a temporary object
		Conversation temp = i->second;

		// Remove the old one
		_conversations.erase(i++);

		// Re-insert with new index
		_conversations.insert(std::make_pair(newIndex, temp));
	}
}

int ConversationEntity::moveConversation(int index, bool moveUp)
{
	if (moveUp && index <= 1 || !moveUp && index >= getHighestIndex())
	{
		return index; // no change
	}

	int targetIndex = index + (moveUp ? -1 : +1);

	if (_conversations.find(targetIndex) != _conversations.end())
	{
		// Swap with existing element
		std::swap(_conversations[index], _conversations[targetIndex]);
	}
	else
	{
		// Nothing present at the target index, just move
		_conversations[targetIndex] = _conversations[index];
		_conversations.erase(index);
	}

	return targetIndex;
}

void ConversationEntity::populateListStore(wxutil::TreeModel& store, const ConversationColumns& columns) const
{
	for (const auto& pair : _conversations)
	{
		wxutil::TreeModel::Row row = store.AddItem();

		row[columns.index] = pair.first;
		row[columns.name] = pair.second.name;

		row.SendItemAdded();
	}
}

void ConversationEntity::clearEntity(Entity* entity)
{
	// Get all keyvalues matching the "obj" prefix.
	auto keyValues = entity->getKeyValuePairs("conv_");

	for (const auto& keyValuePair : keyValues)
	{
		// Set the spawnarg to empty, which is equivalent to a removal
		entity->setKeyValue(keyValuePair.first, "");
	}
}

// Write out conversations to entity keyvals
void ConversationEntity::writeToEntity()
{
	// Try to convert the weak_ptr reference to a shared_ptr
	Entity* entity = Node_getEntity(_entityNode.lock());
	assert(entity != nullptr);

	// greebo: Remove all conversation-related spawnargs first
	clearEntity(entity);

	for (const auto& pair : _conversations)
	{
		// Obtain the conversation and construct the key prefix from the index
		const Conversation& conv = pair.second;
		std::string prefix = "conv_" + string::to_string(pair.first) + "_";

		// Set the entity keyvalues
		entity->setKeyValue(prefix + "name", conv.name);
		entity->setKeyValue(prefix + "actors_must_be_within_talkdistance",
			conv.actorsMustBeWithinTalkdistance ? "1" : "0");
		entity->setKeyValue(prefix + "talk_distance", string::to_string(conv.talkDistance));
		entity->setKeyValue(prefix + "actors_always_face_each_other_while_talking",
			conv.actorsAlwaysFaceEachOther ? "1" : "0");
		entity->setKeyValue(prefix + "max_play_count", string::to_string(conv.maxPlayCount));

		// Write the actor list
		for (const auto& actor : conv.actors)
		{
			std::string actorKey = prefix + "actor_" + string::to_string(actor.first);
			entity->setKeyValue(actorKey, actor.second);
		}

		// Write all the commands
		for (const auto& cmd : conv.commands)
		{
			std::string cmdPrefix = prefix + "cmd_" + string::to_string(cmd.first) + "_";

			try 
			{
				const auto& cmdInfo = ConversationCommandLibrary::Instance().findCommandInfo(cmd.second->type);

				entity->setKeyValue(cmdPrefix + "type", cmdInfo.name);
				entity->setKeyValue(cmdPrefix + "actor", string::to_string(cmd.second->actor));
				entity->setKeyValue(cmdPrefix + "wait_until_finished", cmd.second->waitUntilFinished ? "1" : "0");

				for (const auto& arg : cmd.second->arguments)
				{
					entity->setKeyValue(cmdPrefix + "arg_" + string::to_string(arg.first), arg.second);
				}
			}
			catch (const std::runtime_error&)
			{
				rError() << "Unrecognised conversation command ID: " << cmd.second->type << std::endl;
			}
		}
	}
}

} // namespace conversations
