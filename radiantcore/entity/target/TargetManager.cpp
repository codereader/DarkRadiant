#include "TargetManager.h"

#include "itextstream.h"

namespace entity
{

TargetManager::TargetManager() :
	_emptyTarget(new Target)
{
	// Just to be sure, clear the empty target
	_emptyTarget->clear();
}

ITargetableObjectPtr TargetManager::getTarget(const std::string& name) 
{
	if (name.empty())
    {
		return _emptyTarget;
	}

	auto found = _targets.find(name);

	if (found != _targets.end())
    {
		return found->second;
	}

	// Doesn't exist yet, create this one
	auto target = std::make_shared<Target>();
	target->clear();

	// Insert into the local map and return
	_targets.emplace(name, target);

	return target;
}

void TargetManager::onTargetVisibilityChanged(const std::string& name, const scene::INode& node)
{
    if (name.empty()) return;

    auto existing = _targets.find(name);

    if (existing != _targets.end())
    {
        existing->second->onVisibilityChanged();
    }
}

void TargetManager::onTargetPositionChanged(const std::string& name, const scene::INode& node)
{
    if (name.empty()) return;

    auto existing = _targets.find(name);

    if (existing != _targets.end())
    {
        existing->second->onPositionChanged();
    }
}

void TargetManager::associateTarget(const std::string& name, const scene::INode& node)
{
	if (name.empty())
    {
		return; // don't associate empty names
	}

	auto found = _targets.find(name);

	if (found != _targets.end())
    {
		if (found->second->isEmpty())
        {
			// Already registered, but empty => associate it
			found->second->setNode(node);
            // Trigger a visibility changed signal
            found->second->onVisibilityChanged();
		}
		else {
			// Non-empty target!
			// greebo: Skip that warning, these things happen regularly when cloning entities
			// and are hard to avoid.
			//rError() << "TargetManager: Target " << name << " already associated!\n";
		}

		return;
	}

	// Doesn't exist yet, create a new target and associate it
	// Insert into the local map and return
	_targets.emplace(name, std::make_shared<Target>(node));
}

void TargetManager::clearTarget(const std::string& name, const scene::INode& node)
{
	// Locate and clear the named target
	auto found = _targets.find(name);

	// If found, check the pointer if the request is ok
	if (found != _targets.end() && found->second->getNode() == &node)
    {
		// Yes, the node is matching too, clear the target
		found->second->clear();
	}
}

} // namespace entity
