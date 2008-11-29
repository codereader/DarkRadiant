#ifndef CONVERSATION_COMMAND_LIBRARY_H_
#define CONVERSATION_COMMAND_LIBRARY_H_

#include "ConversationCommandInfo.h"

namespace conversation {

	namespace {
		const std::string RKEY_CONVERSATION_COMMAND_INFO_PREFIX = 
			"game/conversationSystem/conversationCommandPrefix";
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

private:
	// Loads all entityDefs matching the given prefix
	void loadConversationCommands();
};

} // namespace conversation

#endif /* CONVERSATION_COMMAND_LIBRARY_H_ */