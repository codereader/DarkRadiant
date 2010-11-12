#ifndef CONVERSATION_ENTITY_H_
#define CONVERSATION_ENTITY_H_

#include "inode.h"
#include "ientity.h"
#include <boost/shared_ptr.hpp>
#include <gtkmm/liststore.h>

#include "Conversation.h"

namespace conversation
{

struct ConvEntityColumns :
	public Gtk::TreeModel::ColumnRecord
{
	ConvEntityColumns() { add(displayName); add(entityName); }

	Gtk::TreeModelColumn<Glib::ustring> displayName;
	Gtk::TreeModelColumn<Glib::ustring> entityName;
};

struct ConversationColumns :
	public Gtk::TreeModel::ColumnRecord
{
	ConversationColumns() { add(index); add(name); }

	Gtk::TreeModelColumn<int> index;			// conversation index
	Gtk::TreeModelColumn<Glib::ustring> name;	// conversation name
};

/**
 * Representation of a single conversation entity (atdm:conversation_info).
 *
 * In The Dark Mod, conversations are stored as numbered spawnargs on a conversation
 * entity, e.g. <b>conv_1_name</b>. Each conversation entity can contain any number
 * of conversations described in this way.
 *
 * The ConversationEntity class provides an object-oriented view of the conversation
 * information, by wrapping a pointer to an Entity and providing methods to
 * retrieve and manipulate conversation information without seeing the spawnargs
 * directly. When changes are completed, the ConversationEntity::writeToEntity()
 * method is invoked to save all changes in the form of spawnargs.
 *
 * @see Entity
 */
class ConversationEntity
{
	// The actual entity's world node and entity pointer
	scene::INodeWeakPtr _entityNode;

	// Indexed conversations
	ConversationMap _conversations;

public:
	/**
	 * Construct an ConversationEntity wrapper around the given Node.
	 */
	ConversationEntity(scene::INodePtr node);

	/**
	 * Return an Conversation reference by numeric index.
	 *
	 * @param iIndex
	 * The numberic index of the conversation to retrieve.
	 *
	 * @return
	 * A non-const reference to an Conversation object, corresponding to the
	 * given index. If the provided index did not previously exist, a new
	 * Conversation object will be created and returned.
	 */
	Conversation& getConversation(int iIndex) {
		return _conversations[iIndex];
	}

	/**
	 * Add a new conversation, starting from the first unused conversation ID.
	 */
	void addConversation();

	/**
	 * Delete a numbered conversation. This re-orders all conversations so that the
	 * numbering is consistent again (deleting obj 2 will re-number 3 => 2, 4 => 3, etc.)
	 */
	void deleteConversation(int index);

	/**
	 * Clear all conversations.
	 */
	void clearConversations() {
		_conversations.clear();
	}

	/**
	 * Test whether this entity contains conversations or not.
	 */
	bool isEmpty() const {
		return _conversations.empty();
	}

	/**
	 * Delete the actual entity node from the map. This will render any further
	 * operations on this ConversationEntity undefined, and it should immediately
	 * be deleted.
	 */
	void deleteWorldNode();

	/**
	 * Populate the given list store with the conversations from this entity.
	 *
	 * @param store
	 * The list store to populate. This must have 2 columns -- an integer
	 * column for the conversation number, and a text column for the name.
	 */
	void populateListStore(const Glib::RefPtr<Gtk::ListStore>& store, const ConversationColumns& columns) const;

	/**
	 * Write all conversation data to keyvals on the underlying entity.
	 */
	void writeToEntity();

private:

	// Removes all conversation-related spawnargs from the given entity
	void clearEntity(Entity* entity);
};

/**
 * Conversation entity pointer type.
 */
typedef boost::shared_ptr<ConversationEntity> ConversationEntityPtr;

/**
 * Conversation entity named map type.
 */
typedef std::map<std::string, ConversationEntityPtr> ConversationEntityMap;

} // namespace conversation

#endif /* CONVERSATION_ENTITY_H_ */
