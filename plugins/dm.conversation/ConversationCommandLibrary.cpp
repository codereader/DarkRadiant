#include "ConversationCommandLibrary.h"

#include "ieclass.h"
#include "iregistry.h"
#include "gamelib.h"
#include "string/convert.h"
#include "string/predicate.h"
#include <wx/choice.h>

namespace conversation {

/** greebo: The visitor class that stores all the relevant eclassptrs
 * 			into the given target map if the prefix matches.
 */
class ConversationCommandInfoLoader :
	public EntityClassVisitor
{
	// The target map to populate
	ConversationCommandInfoMap& _map;

	// The entityDef prefix (e.g. "atdm:conversation_command_")
	std::string _prefix;

public:
	/**
	 * greebo: Pass the target map where all the eclassptrs should be stored into.
	 */
	ConversationCommandInfoLoader(ConversationCommandInfoMap& map) :
		_map(map),
		_prefix(game::current::getValue<std::string>(GKEY_CONVERSATION_COMMAND_INFO_PREFIX))
	{}

	void visit(const IEntityClassPtr& eclass)
	{
		if (string::starts_with(eclass->getDeclName(), _prefix))
		{
			// We have a match, create a new structure
			ConversationCommandInfoPtr commandInfo(new ConversationCommandInfo);

			// Fill the values from the found entityDef
			commandInfo->parseFromEntityClass(eclass);

			// Store the structure to the target map
			_map[commandInfo->name] = commandInfo;
		}
	}
};

ConversationCommandLibrary::ConversationCommandLibrary() {
	loadConversationCommands();
}

const ConversationCommandInfo& ConversationCommandLibrary::findCommandInfo(const std::string& name) {
	ConversationCommandInfoMap::const_iterator i = _commandInfo.find(name);

	if (i == _commandInfo.end()) {
		throw std::runtime_error(std::string("Could not find command info with the given name: ") + name);
	}

	return *(i->second);
}

const ConversationCommandInfo& ConversationCommandLibrary::findCommandInfo(int id) {

	for (ConversationCommandInfoMap::const_iterator i = _commandInfo.begin();
		 i != _commandInfo.end();
		 ++i)
	{
		if (i->second->id == id) {
			return *(i->second);
		}
	}

	throw std::runtime_error(std::string("Could not find command info with the given ID: ") + string::to_string(id));
}

void ConversationCommandLibrary::loadConversationCommands()
{
	// Load the possible command types
	ConversationCommandInfoLoader loader(_commandInfo);
	GlobalEntityClassManager().forEachEntityClass(loader);
}

void ConversationCommandLibrary::populateChoice(wxChoice* choice)
{
	// Iterate over everything and push the data into the liststore
	for (ConversationCommandInfoMap::const_iterator i = _commandInfo.begin();
		 i != _commandInfo.end();
		 ++i)
	{
		// Store the actor ID into a client data object and pass it along
		choice->Append(i->second->name, new wxStringClientData(string::to_string(i->second->id)));
	}
}

// Static accessor
ConversationCommandLibrary& ConversationCommandLibrary::Instance()
{
	static ConversationCommandLibrary _instance;
	return _instance;
}

} // namespace conversation
