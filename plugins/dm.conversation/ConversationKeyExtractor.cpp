#include "ConversationKeyExtractor.h"

#include "itextstream.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "string/string.h"

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
		// This is an actor definition
		// TODO
	}
	else if (convSubString.substr(0, 4) == "cmd_") {
		// This is a conversation command
		// TODO
	}
}
	
} // namespace conversation
