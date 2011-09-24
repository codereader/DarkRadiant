#pragma once

#include "scenelib.h"
#include <gtkmm/treeselection.h>
#include "GraphTreeModel.h"

namespace ui {

class GraphTreeModelSelectionUpdater :
	public scene::NodeVisitor
{
private:
	GraphTreeModel& _model;
	Glib::RefPtr<Gtk::TreeSelection> _selection;
	bool _visibleOnly;

public:
	GraphTreeModelSelectionUpdater(GraphTreeModel& model, const Glib::RefPtr<Gtk::TreeSelection>& selection, bool visibleOnly) :
		_model(model),
		_selection(selection),
		_visibleOnly(visibleOnly)
	{}

	bool pre(const scene::INodePtr& node)
	{
		const GraphTreeNodePtr& gtNode = _model.find(node);
		Gtk::TreeModel::iterator iter;

		if (gtNode)
		{
			iter = gtNode->getIter();
		}
		else if (!_visibleOnly)
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
