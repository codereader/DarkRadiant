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
	GraphTreeModel::NotifySelectionUpdateFunc _notifySelectionChanged;

public:
	GraphTreeModelSelectionUpdater(GraphTreeModel& model, 
			const GraphTreeModel::NotifySelectionUpdateFunc& notifySelectionChanged) :
		_model(model),
		_notifySelectionChanged(notifySelectionChanged)
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

		_notifySelectionChanged(item, Node_isSelected(node));

		return true;
	}
};

} // namespace ui
