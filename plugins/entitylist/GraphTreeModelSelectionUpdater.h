#pragma once

#include <gtkmm/treeselection.h>
#include "GraphTreeModel.h"
#include "ientity.h"
#include "iselectable.h"
#include "gtkutil/TreeView.h"

namespace ui 
{

class GraphTreeModelSelectionUpdater :
	public scene::NodeVisitor
{
private:
	GraphTreeModel& _model;
	wxutil::TreeView* _view;

public:
	GraphTreeModelSelectionUpdater(GraphTreeModel& model, wxutil::TreeView* view) :
		_model(model),
		_view(view)
	{}

	bool pre(const scene::INodePtr& node)
	{
		const GraphTreeNodePtr& gtNode = _model.find(node);
		wxDataViewItem item;

		if (gtNode)
		{
			item = gtNode->getIter();
		}
		else if (node->visible())
		{
			// The node might have been previously hidden, insert a new one
			GraphTreeNodePtr newlyInserted = _model.insert(node);

			if (newlyInserted)
			{
				item = newlyInserted->getIter();
			}
		}
		else
		{
			return true; 
		}

		if (!item.IsOk()) return true;

		if (Node_isSelected(node))
		{
			_view->Select(item);
		}
		else
		{
			_view->Unselect(item);
		}

		return true;
	}
};

} // namespace ui
