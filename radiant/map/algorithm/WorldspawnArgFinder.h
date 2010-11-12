#ifndef _WORLDSPAWN_ARG_FINDER_H_
#define _WORLDSPAWN_ARG_FINDER_H_

#include "ientity.h"
#include "inode.h"
#include <string>

namespace map {

/**
 * greebo: This class can be used to traverse a subgraph to search
 * for a specific spawnarg on the worldspawn entity. The method
 * getValue() can be used to retrieve the value of the key as specified
 * in the constructor.
 */
class WorldspawnArgFinder :
	public scene::NodeVisitor
{
	std::string _key;
	std::string _value;

public:
	WorldspawnArgFinder(const std::string& keyName) :
		_key(keyName)
	{}

	bool pre(const scene::INodePtr& node) {
		// Try to cast this node onto an entity
		Entity* ent = Node_getEntity(node);

		if (ent != NULL) {

			if (ent->getKeyValue("classname") == "worldspawn") {
				// Load the description spawnarg
				_value = ent->getKeyValue(_key);
			}

			return false; // don't traverse entities
		}

		return true;
	}

	/**
	 * Returns the found value for the desired spawnarg. If not found,
	 * this function will return an empty string "".
	 */
	const std::string& getFoundValue() const {
		return _value;
	}
};

} // namespace map

#endif /* _WORLDSPAWN_ARG_FINDER_H_ */
