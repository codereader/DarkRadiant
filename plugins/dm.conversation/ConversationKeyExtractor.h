#ifndef CONVERSATIONKEYEXTRACTOR_H_
#define CONVERSATIONKEYEXTRACTOR_H_

#include "Conversation.h"

#include "ientity.h"

namespace conversation {

/**
 * Entity Visitor which extracts conversation keyvalues (of the form "conv_<n>_yyyy")
 * and populates the given ConversationMap with the parsed conversation objects.
 */
class ConversationKeyExtractor :
	public Entity::Visitor
{
	// Map of number->Conversation objects
	ConversationMap& _convMap;

public:

	/**
	 * Constructor. Sets the map to populate.
	 */
	ConversationKeyExtractor(ConversationMap& map) :
		_convMap(map)
	{
		assert(_convMap.empty());
	}

	/**
	 * Required visit function.
	 */
	void visit(const std::string& key, const std::string& value);
};

} // namespace conversation

#endif /* CONVERSATIONKEYEXTRACTOR_H_ */
