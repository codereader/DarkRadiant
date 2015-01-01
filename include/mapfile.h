#pragma once

#include <limits>

#include "iscenegraph.h"
#include <memory>
#include <functional>

const std::size_t MAPFILE_MAX_CHANGES = std::numeric_limits<std::size_t>::max();

class IMapFileChangeTracker
{
public:
    virtual ~IMapFileChangeTracker() {}

    virtual void save() = 0;
    virtual bool saved() const = 0;
    virtual void changed() = 0;
    virtual void setChangedCallback(const std::function<void()>& changed) = 0;
    virtual std::size_t changes() const = 0;
};
typedef std::shared_ptr<IMapFileChangeTracker> IMapFileChangeTrackerPtr;

inline IMapFileChangeTrackerPtr Node_getMapFile(const scene::INodePtr& node)
{
    return std::dynamic_pointer_cast<IMapFileChangeTracker>(node);
}

namespace scene
{

inline IMapFileChangeTracker* findMapFile(INodePtr node)
{
	while (node)
	{
        IMapFileChangeTrackerPtr map = Node_getMapFile(node);

		 if (map)
		 {
			 return map.get();
		 }

		 node = node->getParent();
	}

	return nullptr;
}

} // namespace scene
