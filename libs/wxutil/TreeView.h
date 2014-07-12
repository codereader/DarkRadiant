#pragma once

#include <wx/dataview.h>
#include "TreeModel.h"

namespace wxutil
{

/**
 * greebo: Extension of the regular wxDataViewCtrl to add
 * a few regularly need improvements, like automatic column sizing
 * for treeviews (a thing that seems to be problematic in the 
 * pure wxDataViewCtrl).
 *
 * Use the named constructors Create*() to instantiate a new TreeView.
 */
class TreeView :
	public wxDataViewCtrl
{
protected:
	TreeView(wxWindow* parent, TreeModel* model, long style) :
		wxDataViewCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style)
	{
		EnableAutoColumnWidthFix();

		if (model != NULL)
		{
			AssociateModel(model);
			model->DecRef();
		}
	}

public:
	// Create a TreeView without model (single-selection mode)
	static TreeView* Create(wxWindow* parent, long style = wxDV_SINGLE)
	{
		return new TreeView(parent, NULL, style);
	}

	// Construct a TreeView using the given TreeModel, which will be associated
	// with this view (refcount is automatically decreased by one).
	static TreeView* CreateWithModel(wxWindow* parent, TreeModel* model, long style = wxDV_SINGLE)
	{
		return new TreeView(parent, model, style);
	}

	virtual ~TreeView() {}

	// Enable the automatic recalculation of column widths
	void EnableAutoColumnWidthFix(bool enable = true)
	{
		if (enable)
		{
			Connect(wxEVT_DATAVIEW_ITEM_EXPANDED, wxDataViewEventHandler(TreeView::_onItemExpanded), NULL, this);
		}
		else
		{
			Disconnect(wxEVT_DATAVIEW_ITEM_EXPANDED, wxDataViewEventHandler(TreeView::_onItemExpanded), NULL, this);
		}
	}

private:
	void TreeView::_onItemExpanded(wxDataViewEvent& ev)
	{
		// This should force a recalculation of the column width
		if (GetModel() != NULL)
		{
			GetModel()->ItemChanged(ev.GetItem());
		}

		ev.Skip();
	} 
};

} // namespace
