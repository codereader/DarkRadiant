#ifndef CONVERSATION_COMMAND_INFO_H_
#define CONVERSATION_COMMAND_INFO_H_

#include "ieclass.h"
#include <vector>

namespace conversation {

// Information about a single conversation command argument
struct ArgumentInfo
{
	enum ArgumentType {
		ARGTYPE_INT,
		ARGTYPE_FLOAT,
		ARGTYPE_STRING,
		ARGTYPE_VECTOR,
		ARGTYPE_SOUNDSHADER,
		ARGTYPE_ACTOR,
		ARGTYPE_ENTITY,
		ARGTYPE_BOOL,
		NUM_ARGTYPES,
	};

	ArgumentType type;

	std::string description;
	std::string title;

	bool required;
};

/**
 * greebo: This structure holds information about a certain
 * Conversation Command Type (e.g. "WaitSeconds") and its arguments.
 *
 * As the number and type of arguments varies, the CommandEditor GUI
 * needs this structure to build the variable interface.
 */
class ConversationCommandInfo
{
public:
	// The unique id of this conversation command.
	int id;

	// The internal name, like "WaitSeconds" or "InteractWithEntity"
	std::string name;

	// Whether the "wait until finished" checkbox is displayed
	bool waitUntilFinishedAllowed;

	// The "human readable" sentence format (with placeholders)
	std::string sentence;

	typedef std::vector<ArgumentInfo> ArgumentInfoList;
	ArgumentInfoList arguments;

	ConversationCommandInfo() :
		id(++_highestId),
		waitUntilFinishedAllowed(true)
	{}

	// Fills the member variables from the given entityDef
	void parseFromEntityClass(const IEntityClassPtr& eclass);

private:
	// Highest ID so far
	static int _highestId;
};
typedef boost::shared_ptr<ConversationCommandInfo> ConversationCommandInfoPtr;

// A mapping between command typenames ("WalkToActor") to actual information structures
typedef std::map<std::string, ConversationCommandInfoPtr> ConversationCommandInfoMap;

} // namespace conversation

#endif /* CONVERSATION_COMMAND_INFO_H_ */
