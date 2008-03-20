#include "TargetManager.h"

#include "stream/textstream.h"

namespace entity {

// Constructor
TargetManager::TargetManager() :
	_emptyTarget(new Target)
{
	// Just to be sure, clear the empty target
	_emptyTarget->clear();
}

// Static accessor method
TargetManager& TargetManager::Instance() {
	static TargetManager _instance;
	return _instance;
}

TargetPtr TargetManager::getTarget(const std::string name) {
	if (name.empty()) {
		return _emptyTarget;
	}

	TargetList::iterator found = _targets.find(name);

	if (found != _targets.end()) {
		return found->second;
	}

	// Doesn't exist yet, create this one
	TargetPtr target(new Target);
	target->clear();

	// Insert into the local map and return
	_targets.insert(TargetList::value_type(name, target));

	return target;
}

void TargetManager::associateTarget(const std::string& name, const scene::INodePtr& node) {
	if (name.empty()) {
		return; // don't associate empty names
	}

	TargetList::iterator found = _targets.find(name);

	if (found != _targets.end()) {
		if (found->second->isEmpty()) {
			// Already registered, but empty => associate it
			found->second->setNode(node);
		}
		else {
			// Non-empty target!
			globalErrorStream() << "TargetManager: Target " << name.c_str() << " already associated!\n";
		}

		return;
	}

	// Doesn't exist yet, create a new target
	TargetPtr target(new Target);

	// Associate the target
	target->setNode(node);

	// Insert into the local map and return
	_targets.insert(TargetList::value_type(name, target));
}

void TargetManager::clearTarget(const std::string& name, const scene::INodePtr& node) {
	// Locate and clear the named target
	TargetList::iterator found = _targets.find(name);

	if (found != _targets.end()) {
		// Found, check the pointer if the request is ok
		if (found->second->getNode() == node) {
			// Yes, the node is matching too, clear the target
			found->second->clear();
		}
	}
}

} // namespace entity
