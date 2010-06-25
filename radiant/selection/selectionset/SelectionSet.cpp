#include "SelectionSet.h"

#include "scenelib.h"

namespace selection
{

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
	class Walker :
		public SelectionSystem::Visitor
	{
	private:
		SelectionSet& _set;

	public:
		Walker(SelectionSet& set) :
			_set(set)
		{}

		void visit(const scene::INodePtr& node) const
		{
			_set.addNode(node);
		}

	} _walker(*this);

	GlobalSelectionSystem().foreachSelected(_walker);
}

} // namespace
