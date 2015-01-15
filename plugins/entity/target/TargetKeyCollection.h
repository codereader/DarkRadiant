#pragma once

#include <map>
#include "TargetKey.h"

namespace entity
{

class TargetKeyCollection :
	public Entity::Observer
{
private:
	// greebo: A container mapping "targetN" keys to TargetKey objects
	typedef std::map<std::string, TargetKey> TargetKeyMap;
	TargetKeyMap _targetKeys;

public:
	// Entity::Observer implementation, gets called on key insert/erase
	void onKeyInsert(const std::string& key, EntityKeyValue& value);
	void onKeyErase(const std::string& key, EntityKeyValue& value);

	/**
	 * greebo: Walker function, calls visit() for each target
	 *         contained in this structure.
	 */
    void forEachTarget(const std::function<void(const TargetPtr&)>& func) const;

	// Returns TRUE if there are no "target" keys observed
	bool empty() const;

private:
	// Returns TRUE if the given key matches the pattern for target keys
	bool isTargetKey(const std::string& key);
};

} // namespace entity
