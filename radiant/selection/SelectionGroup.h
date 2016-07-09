#pragma once

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

public:
	SelectionGroup(std::size_t id) :
		_id(id)
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
