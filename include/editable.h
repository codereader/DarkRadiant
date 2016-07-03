#pragma once

class AABB;

#include "inode.h"

class Snappable
{
public:
    virtual ~Snappable() {}
	virtual void snapto(float snap) = 0;
};
typedef std::shared_ptr<Snappable> SnappablePtr;

inline SnappablePtr Node_getSnappable(const scene::INodePtr& node)
{
	return std::dynamic_pointer_cast<Snappable>(node);
}

class ComponentEditable {
public:
    virtual ~ComponentEditable() {}
	virtual const AABB& getSelectedComponentsBounds() const = 0;
};
typedef std::shared_ptr<ComponentEditable> ComponentEditablePtr;

inline ComponentEditablePtr Node_getComponentEditable(const scene::INodePtr& node)
{
	return std::dynamic_pointer_cast<ComponentEditable>(node);
}

class ComponentSnappable {
public:
    virtual ~ComponentSnappable() {}
	virtual void snapComponents(float snap) = 0;
};
typedef std::shared_ptr<ComponentSnappable> ComponentSnappablePtr;

inline ComponentSnappablePtr Node_getComponentSnappable(const scene::INodePtr& node)
{
	return std::dynamic_pointer_cast<ComponentSnappable>(node);
}
