#pragma once

#include <map>
#include <sigc++/signal.h>
#include "TargetKey.h"

namespace entity
{

class TargetableNode;

class TargetKeyCollection :
	public Entity::Observer
{
private:
    TargetableNode& _owner;

	// greebo: A container mapping "targetN" keys to TargetKey objects
	typedef std::map<std::string, TargetKey> TargetKeyMap;
	TargetKeyMap _targetKeys;

    sigc::signal<void> _sigTargetPositionChanged;

public:
    TargetKeyCollection(TargetableNode& owner);

    // Might return nullptr if not inserted in the scene
    ITargetManager* getTargetManager();

    // Called by the owning TargetableNode to notify this class of a new target manager
    // Since not all nodes are inserted in a scene by the time the target spawnargs are
    // set we might need to call this at a later point.
    void onTargetManagerChanged();

	// Entity::Observer implementation, gets called on key insert/erase
	void onKeyInsert(const std::string& key, EntityKeyValue& value);
	void onKeyErase(const std::string& key, EntityKeyValue& value);

	/**
	 * greebo: Walker function, calls visit() for each target
	 *         contained in this structure.
	 */
    void forEachTarget(const std::function<void(const TargetPtr&)>& func) const;

    // Returns the number of target keys
    std::size_t getNumTargets() const;

	// Returns TRUE if there are no "target" keys observed
	bool empty() const;

    // The TargetLineNode listens to this to reconstructs its renderables
    sigc::signal<void>& signal_TargetPositionChanged();

    // Invoked by the TargetKey instance if a single Target changes its position
    void onTargetPositionChanged();

private:
	// Returns TRUE if the given key matches the pattern for target keys
	bool isTargetKey(const std::string& key);
};

} // namespace entity
