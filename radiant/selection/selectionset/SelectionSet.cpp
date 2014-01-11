#include "SelectionSet.h"

#include "iselectable.h"

namespace selection
{

SelectionSet::SelectionSet(const std::string& name) :
	_name(name)
{}

const std::string& SelectionSet::getName()
{
	return _name;
}

bool SelectionSet::empty()
{
	return _nodes.empty();
}

void SelectionSet::clear()
{
	_nodes.clear();
}

void SelectionSet::select()
{
	for (NodeSet::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
	{
		scene::INodePtr node = i->lock();

		if (node == NULL) continue; // skip deleted nodes

		if (!node->visible()) continue; // skip invisible, non-instantiated nodes

		Node_setSelected(node, true);
	}
}

void SelectionSet::deselect()
{
	for (NodeSet::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
	{
		scene::INodePtr node = i->lock();

		if (node == NULL) continue; // skip deleted nodes

		if (!node->visible()) continue; // skip invisible, non-instantiated nodes

		Node_setSelected(node, false);
	}
}

void SelectionSet::addNode(const scene::INodePtr& node)
{
	scene::INodeWeakPtr weakPtr(node);
	_nodes.insert(weakPtr);
}

void SelectionSet::assignFromCurrentScene()
{
	clear();

	GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
	{
		addNode(node);
	});
}

std::set<scene::INodePtr> SelectionSet::getNodes()
{
	std::set<scene::INodePtr> nodeSet;

	for (NodeSet::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
	{
		scene::INodePtr node = i->lock();

		if (node == NULL) continue; // skip deleted nodes

		nodeSet.insert(node);
	}

	return nodeSet;
}

} // namespace
