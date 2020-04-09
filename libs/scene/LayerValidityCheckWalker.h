#pragma once

#include "ilayer.h"
#include "imap.h"

namespace scene
{

class LayerValidityCheckWalker :
	public NodeVisitor
{
private:
	std::size_t _numFixed;
public:
	LayerValidityCheckWalker() :
		_numFixed(0)
	{}

	// scene::NodeVisitor
	bool pre(const INodePtr& node)
	{
		if (ProcessNode(node))
		{
			_numFixed++;
		}
		
		return true;
	}

	// Returns true if the node has been "fixed"
	static bool ProcessNode(const INodePtr& node)
	{
		auto rootNode = node->getRootNode();

		if (!rootNode) return false;

		LayerList list = node->getLayers(); // create a copy of the list

		bool fixed = false;

		for (auto id : list)
		{
			if (!rootNode->getLayerManager().layerExists(id))
			{
				node->removeFromLayer(id);
				fixed = true;
			}
		}

		return fixed;
	}

	std::size_t getNumFixed() const
	{
		return _numFixed;
	}
};

}
