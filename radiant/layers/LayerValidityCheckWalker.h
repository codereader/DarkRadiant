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
		LayerList list = node->getLayers();

		for (LayerList::iterator i = list.begin(); i != list.end(); ++i)
		{
			if (!GlobalLayerSystem().layerExists(*i))
			{
				node->removeFromLayer(*i);
				_numFixed++;
			}
		}
		
		return true;
	}

	std::size_t getNumFixed() const
	{
		return _numFixed;
	}
};

}
