#include "TreeView.h"

namespace wxutil
{

TreeView::TreeView(wxWindow* parent, TreeModel* model, long style) :
	wxDataViewCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style)
{
	EnableAutoColumnWidthFix();

	if (model != NULL)
	{
		AssociateModel(model);
		model->DecRef();
	}
}

TreeView* TreeView::Create(wxWindow* parent, long style)
{
	return new TreeView(parent, NULL, style);
}

// Construct a TreeView using the given TreeModel, which will be associated
// with this view (refcount is automatically decreased by one).
TreeView* TreeView::CreateWithModel(wxWindow* parent, TreeModel* model, long style)
{
	return new TreeView(parent, model, style);
}

TreeView::~TreeView()
{}

// Enable the automatic recalculation of column widths
void TreeView::EnableAutoColumnWidthFix(bool enable)
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

void TreeView::_onItemExpanded(wxDataViewEvent& ev)
{
	// This should force a recalculation of the column width
	if (GetModel() != NULL)
	{
		GetModel()->ItemChanged(ev.GetItem());
	}

	ev.Skip();
}

} // namespace
