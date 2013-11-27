#pragma once

#include "ConversationCommandInfo.h"
#include <gtkmm/liststore.h>

namespace conversation
{

	// Treemodel definitions
	struct CommandColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		CommandColumns() { add(cmdNumber); add(caption); }

		Gtk::TreeModelColumn<int> cmdNumber;
		Gtk::TreeModelColumn<Glib::ustring> caption;
	};

	namespace {
		const std::string GKEY_CONVERSATION_COMMAND_INFO_PREFIX =
			"/conversationSystem/conversationCommandPrefix";
	}

/**
 * greebo: This class holds all the possible conversation command types,
 * indexed by name. Each conversation commmand is parsed from an entityDef
 * matching a given prefix and holding a bunch of information about that command.
 *
 * The ConversationCommand editor is using this information to construct
 * the UI elements.
 */
class ConversationCommandLibrary
{
	// The map containing the named information
	ConversationCommandInfoMap _commandInfo;

	// Private constructor, loads all matching entityDefs
	ConversationCommandLibrary();

public:
	// Accessor to the singleton instance
	static ConversationCommandLibrary& Instance();

	/**
	 * Returns the command info with the given typename or ID. Throws an exception if not found.
	 *
	 * @throws: std::runtime error if the named info structure could not be found.
	 */
	const ConversationCommandInfo& findCommandInfo(const std::string& name);
	const ConversationCommandInfo& findCommandInfo(int id);

	/**
	 * greebo: This populates the given liststore with all available commands.
	 */
	void populateListStore(const Glib::RefPtr<Gtk::ListStore>& store, const CommandColumns& columns);

private:
	// Loads all entityDefs matching the given prefix
	void loadConversationCommands();
};

} // namespace conversation
