#pragma once

#include "util/ScopedBoolLock.h"

namespace selection
{

// Represents a named group of selectable items
class SelectionGroup
{
private:
	std::size_t _id;

	std::string _name;

	// The contained nodes of this group
	std::set<scene::INodeWeakPtr, std::owner_less<scene::INodeWeakPtr> > _nodes;

	// To avoid entering feedback loops during group updates
	bool _selectionLock;

public:
	SelectionGroup(std::size_t id) :
		_id(id),
		_selectionLock(false)
	{}

	std::size_t getId() const
	{
		return _id;
	}

	const std::string& getName() const
	{
		return _name;
	}

	void setName(const std::string& name)
	{
		_name = name;
	}

	void addNode(const scene::INodePtr& node)
	{
		_nodes.insert(scene::INodeWeakPtr(node));
	}

	void removeNode(const scene::INodePtr& node)
	{
		_nodes.erase(scene::INodeWeakPtr(node));
	}

	std::size_t size() const
	{
		return _nodes.size();
	}

	void setSelected(bool selected)
	{
		// In debug build's, I'd like to see the feedback loops, 
		// so fire the debugger if we're re-entering the setSelected loop
		assert(!_selectionLock);

		if (_selectionLock) return; // already updating this group

		util::ScopedBoolLock lock(_selectionLock);

		foreachNode([&](const scene::INodePtr& node)
		{
			std::shared_ptr<IGroupSelectable> selectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

			assert(selectable);

			// Set the node status, but don't do a group update (we're already here)
			selectable->setSelected(selected, false);
		});
	}

	void foreachNode(const std::function<void(const scene::INodePtr&)>& functor)
	{
		for (const scene::INodeWeakPtr& node : _nodes)
		{
			scene::INodePtr locked = node.lock();

			if (locked)
			{
				functor(locked);
			}
		}
	}
};
typedef std::shared_ptr<SelectionGroup> SelectionGroupPtr;

} // namespace
