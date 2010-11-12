#ifndef CONVERSATION_COMMAND_H_
#define CONVERSATION_COMMAND_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

namespace conversation {

/**
 * Data object representing a single ConversationCommand.
 */
class ConversationCommand
{
public:
	// What kind of command this is (index of a certain ConversationCommandInfo structure)
	int type;

	// Which actor should perform this command.
	int actor;

	// whether the command must be finished before the next command can start
	bool waitUntilFinished;

	// The numbered arguments
	typedef std::map<int, std::string> ArgumentMap;
	ArgumentMap arguments;

	// Constructor
	ConversationCommand();

	/**
	 * greebo: Returns a human-readable version of this command.
	 */
	std::string getSentence() const;

	// Returns the argument with the given index or "" if not found
	std::string getArgument(int index) const;
};
typedef boost::shared_ptr<ConversationCommand> ConversationCommandPtr;

} // namespace conversation

#endif /* CONVERSATION_COMMAND_H_ */
