#pragma once

#include "inode.h"
#include "iselectiontest.h"
#include "selection/OccludeSelector.h"

namespace camera
{

class ObjectFinder :
    public scene::NodeVisitor
{
private:
    scene::INodePtr _node;
    SelectionTest& _selectionTest;

    // To store the best intersection candidate
    SelectionIntersection _bestIntersection;

public:
    // Constructor
    ObjectFinder(SelectionTest& test) :
        _selectionTest(test)
    {}

    const scene::INodePtr& getNode() const
    {
        return _node;
    }

    // The visitor function
    bool pre(const scene::INodePtr& node)
    {
        // Check if the node is filtered
        if (node->visible())
        {
            auto selectionTestable = Node_getSelectionTestable(node);

            if (selectionTestable)
            {
                bool occluded;
                selection::OccludeSelector selector(_bestIntersection, occluded);
                selectionTestable->testSelect(selector, _selectionTest);

                if (occluded)
                {
                    _node = node;
                }
            }
        }
        else
        {
            return false; // don't traverse filtered nodes
        }

        return true;
    }
};

}
