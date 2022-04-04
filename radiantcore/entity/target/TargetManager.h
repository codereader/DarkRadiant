#pragma once

#include <map>
#include <string>
#include "ientity.h"
#include "Target.h"

namespace entity
{

/**
 * greebo: TargetManager keeps track of all Target objects in the current scene.
 * An instance is owned by each map's root node.
 */
class TargetManager :
    public ITargetManager
{
private:
	// The list of all named Target objects
    std::map<std::string, TargetPtr> _targets;

	// An empty Target (this is returned if an empty name is requested)
	TargetPtr _emptyTarget;

public:
    TargetManager();

	/**
	 * greebo: Returns the Target with the given name.
	 *
	 * This never returns NULL, a Target is created if it doesn't exist yet.
	 */
	ITargetableObjectPtr getTarget(const std::string& name) override;

	/**
	 * greebo: Associates the Target with the given name
	 *         to the given scene::INode.
	 *
	 * The Target will be created if it doesn't exist yet.
	 */
    void associateTarget(const std::string& name, const scene::INode& node) override;

    // Is called by the TargetableNode to notify about visibility changes
    void onTargetVisibilityChanged(const std::string& name, const scene::INode& node) override;

    // Is called by the TargetableNode to notify about a position change
    void onTargetPositionChanged(const std::string& name, const scene::INode& node) override;

	/**
	 * greebo: Disassociates the Target from the given name. The node
	 *         must also be passed to allow the manager to check the request.
	 *         Otherwise it would be possible for cloned nodes to dissociate
	 *         the target from their source node.
	 */
    void clearTarget(const std::string& name, const scene::INode& node) override;
};

} // namespace entity
