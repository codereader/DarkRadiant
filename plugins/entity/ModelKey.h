#ifndef MODELKEY_H_
#define MODELKEY_H_

#include <string>
#include "inode.h"
#include "generic/callback.h"

/**
 * greebo: A ModelKey object watches the "model" spawnarg of 
 *         an entity. As soon as the keyvalue changes, the according
 *         modelnode is loaded and inserted into the entity's Traversable.
 */
class ModelKey
{
	scene::INodePtr _modelNode;
	
	// The parent node, where the model node can be added to (as child)
	scene::INode& _parentNode;

	std::string _modelPath;

public:
	ModelKey(scene::INode& parentNode);
	
	// Update the model to the provided keyvalue, this removes the old scene::Node
	// and inserts the new one after acquiring the model from the cache.
	void modelChanged(const std::string& value);
	typedef MemberCaller1<ModelKey, const std::string&, &ModelKey::modelChanged> ModelChangedCaller;

	// Returns the reference to the "singleton" model node
	const scene::INodePtr& getNode() const;
};

#endif /* MODELKEY_H_ */
