#include "ConversationCommand.h"

namespace conversation {

ConversationCommand::ConversationCommand() :
	type(-1), // invalid id
	actor(-1),
	waitUntilFinished(true)
{}

std::string ConversationCommand::getSentence() const {
	return "test";
}

} // namespace conversation
