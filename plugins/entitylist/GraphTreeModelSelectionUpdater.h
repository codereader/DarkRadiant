#pragma once

#include <gtkmm/treeselection.h>
#include "GraphTreeModel.h"
#include "ientity.h"
#include "iselectable.h"

namespace ui {

class GraphTreeModelSelectionUpdater :
	public scene::NodeVisitor
{
private:
	GraphTreeModel& _model;
	Glib::RefPtr<Gtk::TreeSelection> _selection;

public:
	GraphTreeModelSelectionUpdater(GraphTreeModel& model, const Glib::RefPtr<Gtk::TreeSelection>& selection) :
		_model(model),
		_selection(selection)
	{}

	bool pre(const scene::INodePtr& node)
	{
		const GraphTreeNodePtr& gtNode = _model.find(node);
		Gtk::TreeModel::iterator iter;

		if (gtNode)
		{
			iter = gtNode->getIter();
		}
		else if (node->visible())
		{
			// The node might have been previously hidden, insert a new one
			GraphTreeNodePtr newlyInserted = _model.insert(node);

			if (newlyInserted)
			{
				iter = newlyInserted->getIter();
			}
		}
		else
		{
			return true; 
		}

		if (!iter) return true;

		if (Node_isSelected(node))
		{
			_selection->select(iter);
		}
		else
		{
			_selection->unselect(iter);
		}

		return true;
	}
};

} // namespace ui
