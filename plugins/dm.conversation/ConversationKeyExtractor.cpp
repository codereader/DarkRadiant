#include "ConversationKeyExtractor.h"

#include "itextstream.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "string/string.h"
#include "ConversationCommand.h"

namespace conversation {

// Shortcut for boost::algorithm::split
typedef std::vector<std::string> StringParts;
	
// Required entity visit function
void ConversationKeyExtractor::visit(const std::string& key, const std::string& value) {
	// Quick discard of any non-conversation keys
	if (key.substr(0, 4) != "conv") return;
		
	// Extract the objective number
	static const boost::regex reConvNum("conv_(\\d+)_(.*)");
	boost::smatch results;
	int iNum;
	
	if (!boost::regex_match(key, results, reConvNum)) {
		// No match, abort
		return;
	}

	// Get the conversation number
	iNum = strToInt(results[1]);

	// We now have the conversation number and the substring (everything after
	// "conv_<n>_" which applies to this conversation.
	std::string convSubString = results[2];
	
	// Switch on the substring
	if (convSubString == "name") {
		_convMap[iNum].name = value;			
	}
	else if (convSubString == "talk_distance") {
		_convMap[iNum].talkDistance = strToFloat(value, 60);
	}
	else if (convSubString == "actors_must_be_within_talkdistance") {
		_convMap[iNum].actorsMustBeWithinTalkdistance = (value == "1");
	}
	else if (convSubString == "actors_always_face_each_other_while_talking") {
		_convMap[iNum].actorsAlwaysFaceEachOther = (value == "1");
	}
	else if (convSubString == "max_play_count") {
		_convMap[iNum].maxPlayCount = strToInt(value, -1);
	}
	else if (convSubString.substr(0, 6) == "actor_") {
		// This is an actor definition, extract the number
		int actorNum = strToInt(convSubString.substr(6), -1);

		if (actorNum == -1) {
			return;
		}

		// Store the actor in the map
		_convMap[iNum].actors.insert(Conversation::ActorMap::value_type(actorNum, value));
	}
	else if (convSubString.substr(0, 4) == "cmd_") {
		// This is a conversation command, form a new regex
		static const boost::regex reCommand("(\\d+)_(.*)");
		boost::smatch results;
		
		if (!boost::regex_match(convSubString, results, reCommand)) {
			return; // not matching
		}

		int cmdIndex = strToInt(results[1]);
		std::string cmdSubStr = results[2];

		// Create a new command
		ConversationCommandPtr command(new ConversationCommand);

		if (cmdSubStr == "type") {
			command->type = value;
		}
		else if (cmdSubStr == "actor") {
			command->actor = strToInt(value);
		}
		else if (cmdSubStr == "wait_until_finished") {
			command->waitUntilFinished = (value == "1");
		}
		else if (cmdSubStr.substr(0,4) == "arg_") {
			int cmdArgIndex = strToInt(cmdSubStr.substr(4));

			command->arguments[cmdArgIndex] = value;
		}
	}
}
	
} // namespace conversation
