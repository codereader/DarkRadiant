#ifndef GRAPHTREEMODELSELECTIONUPDATER_H_
#define GRAPHTREEMODELSELECTIONUPDATER_H_

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

public:
	GraphTreeModelSelectionUpdater(GraphTreeModel& model, const Glib::RefPtr<Gtk::TreeSelection>& selection) :
		_model(model),
		_selection(selection)
	{}
	
	bool pre(const scene::INodePtr& node) 
	{
		const GraphTreeNodePtr& gtNode = _model.find(node);
		
		if (gtNode == NULL) {
			return true;
		}
		
		if (Node_isSelected(node))
		{
			_selection->select(gtNode->getIter());
		}
		else
		{
			_selection->unselect(gtNode->getIter());
		}
		
		return true;
	}
};

} // namespace ui

#endif /*GRAPHTREEMODELSELECTIONUPDATER_H_*/
