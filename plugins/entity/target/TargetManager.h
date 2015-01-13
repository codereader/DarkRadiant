#pragma once

#include <map>
#include <string>
#include "ientity.h"
#include "Target.h"

namespace entity
{

/**
 * greebo: This singleton TargetManager keeps track of all
 *         Target objects in the current scene.
 *
 * TargetKey classes acquire a named TargetPtr by calling getTarget(). This
 * always succeeds - if the named Target is not found, a new, empty one
 * is created.
 *
 * The TargetableInstances report to this manager as soon as
 * their name is changed or set. This will associate an empty Target object
 * with an actual scene::Node.
 *
 *
 *          Target object (can be empty)
 *                   ________
 *                  /        \
 *                  |        |
 * TargetKey ----->>|    -------->> holds scene::INodePtr (==NULL, if empty)
 *                  |        |
 *                  \________/
 */
class TargetManager :
    public ITargetManager

{
	// The list of all named Target objects
	typedef std::map<std::string, TargetPtr> TargetList;
	TargetList _targets;

	// An empty Target (this is returned if an empty name is requested)
	TargetPtr _emptyTarget;

public:
    // constructor
    TargetManager();

	// Accessor to the singleton instance (DEPRECATED)
	static TargetManager& Instance();

	/**
	 * greebo: Returns the Target with the given name.
	 *
	 * This never returns NULL, a Target is created if it doesn't exist yet.
	 */
	ITargetableObjectPtr getTarget(const std::string name) override;

	/**
	 * greebo: Associates the Target with the given name
	 *         to the given scene::INode.
	 *
	 * The Target will be created if it doesn't exist yet.
	 */
    void associateTarget(const std::string& name, const scene::INode& node) override;

	/**
	 * greebo: Disassociates the Target from the given name. The node
	 *         must also be passed to allow the manager to check the request.
	 *         Otherwise it would be possible for cloned nodes to dissociate
	 *         the target from their source node.
	 */
    void clearTarget(const std::string& name, const scene::INode& node) override;
};

} // namespace entity
