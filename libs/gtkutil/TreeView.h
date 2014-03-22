#pragma once

#include <wx/dataview.h>

namespace wxutil
{

/**
 * greebo: Extension of the regular wxDataViewCtrl to add
 * a few regularly need improvements, like automatic column sizing
 * for treeviews (a thing that seems to be problematic in the 
 * pure wxDataViewCtrl).
 */
class TreeView :
	public wxDataViewCtrl
{
public:
	TreeView(wxWindow* parent, long style = wxDV_SINGLE) :
		wxDataViewCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style)
	{
		EnableAutoColumnWidthFix();
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
