#pragma once

#include <regex>
#include "Conversation.h"

#include "ientity.h"

namespace conversation 
{

/**
 * Entity Visitor which extracts conversation keyvalues (of the form "conv_<n>_yyyy")
 * and populates the given ConversationMap with the parsed conversation objects.
 */
class ConversationKeyExtractor
{
private:
	// Map of number->Conversation objects
	ConversationMap& _convMap;

	const std::regex _regexConvNum;
	const std::regex _regexConvCmd;

public:

	/**
	 * Constructor. Sets the map to populate.
	 */
	ConversationKeyExtractor(ConversationMap& map);

	/**
	 * Required visit function.
	 */
	void operator()(const std::string& key, const std::string& value);
};

} // namespace
