#include "ConversationEntity.h"

#include "scenelib.h"
#include "ientity.h"

#include "string/string.h"
#include <boost/algorithm/string/classification.hpp>

#include "ConversationKeyExtractor.h"

namespace conversation {

	namespace {
		const int INVALID_LEVEL_INDEX = -9999;
	}

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
	o.name = "New Conversation";
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
void ConversationEntity::populateListStore(GtkListStore* store) const {
	for (ConversationMap::const_iterator i = _conversations.begin();
		 i != _conversations.end();
		 ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, i->first, 
						   1, i->second.name.c_str(),
						   -1);	
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
		/*const Conversation& o = i->second;
		std::string prefix = "obj" + intToStr(i->first) + "_";
		
		// Set the entity keyvalues
		entity->setKeyValue(prefix + "desc", o.description);
		entity->setKeyValue(prefix + "ongoing", o.ongoing ? "1" : "0");
		entity->setKeyValue(prefix + "visible", o.visible ? "1" : "0");
		entity->setKeyValue(prefix + "mandatory", o.mandatory ? "1" : "0");
		entity->setKeyValue(prefix + "irreversible", 
							 o.irreversible ? "1" : "0");
		entity->setKeyValue(prefix + "state", intToStr(o.state));

		// Write an empty "objN_difficulty" value when this conversation applies to all levels
		entity->setKeyValue(prefix + "difficulty", o.difficultyLevels);

		entity->setKeyValue(prefix + "enabling_objs", o.enablingObjs);

		entity->setKeyValue(prefix + "script_complete", o.completionScript);
		entity->setKeyValue(prefix + "script_failed", o.failureScript);

		entity->setKeyValue(prefix + "logic_success", o.logic.successLogic);
		entity->setKeyValue(prefix + "logic_failure", o.logic.failureLogic);

        // Write the Components for this conversation
        writeComponents(entity, prefix, o);*/
	}
}

} // namespace conversations
