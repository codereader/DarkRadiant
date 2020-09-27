#pragma once

#include "imodule.h"
#include "math/AABB.h"

namespace map
{

class IRegionManager :
	public RegisterableModule
{
public:
	virtual ~IRegionManager() {}

	/** 
	 ' greebo: Stores the corners coordinates of the currently active
	 * region into the given <regionMin>, <regionMax> vectors.
	 * If regioning is inactive, the maximum possible bounds are returned.
	 */
	virtual AABB getRegionBounds() = 0;
};

} // namespace

const char* const MODULE_REGION_MANAGER = "RegionManager";

inline map::IRegionManager& GlobalRegionManager()
{
	static module::InstanceReference<map::IRegionManager> _reference(MODULE_REGION_MANAGER);
	return _reference;
}
