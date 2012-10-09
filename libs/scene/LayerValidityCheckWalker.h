#pragma once

#include "ilayer.h"

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
		LayerList list = node->getLayers();

		bool fixed = false;

		for (LayerList::iterator i = list.begin(); i != list.end(); ++i)
		{
			if (!GlobalLayerSystem().layerExists(*i))
			{
				node->removeFromLayer(*i);
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
