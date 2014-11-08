#pragma once

#include <wx/dataview.h>
#include <wx/windowptr.h>

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
	class SearchPopupWindow;
	SearchPopupWindow* _searchPopup;

	std::vector<TreeModel::Column> _colsToSearch;
	wxDataViewItem _curSearchMatch;

	TreeView(wxWindow* parent, TreeModel::Ptr model, long style);

public:
    typedef wxWindowPtr<TreeView> Ptr;

	// Create a TreeView without model (single-selection mode)
	static TreeView* Create(wxWindow* parent, long style = wxDV_SINGLE);

	// Construct a TreeView using the given TreeModel, which will be associated
	// with this view (refcount is automatically decreased by one).
	static TreeView* CreateWithModel(wxWindow* parent, TreeModel::Ptr model, long style = wxDV_SINGLE);

	virtual ~TreeView();

    // ovverride wxDataViewCtrl to make it more robust
    virtual bool AssociateModel(wxDataViewModel* model);

	// Enable the automatic recalculation of column widths
	void EnableAutoColumnWidthFix(bool enable = true);
	
	// Adds a column to search when the user starts typing
	void AddSearchColumn(const TreeModel::Column& column);

public:
	// Event handled by the TreeView when the user triggers a search
	// or tries to navigate between search results
	class SearchEvent :
		public wxEvent
	{
	private:
		wxString _searchString;
	public:
		enum EventType
		{
			SEARCH,				// user has entered something, changed the search terms
			SEARCH_NEXT_MATCH,	// user wants to display the next match
			SEARCH_PREV_MATCH,	// user wants to display the prev match
			POPUP_DISMISSED,	// popup has been dismissed, search has ended
		};

		SearchEvent(int id = SEARCH);
		SearchEvent(const wxString& searchString, int id = SEARCH);
		SearchEvent(const SearchEvent& ev);

		wxEvent* Clone() const;

		const wxString& GetSearchString() const;
	};

	typedef void (wxEvtHandler::*SearchHandlerFunction)(SearchEvent&);

private:
	void _onItemExpanded(wxDataViewEvent& ev);
	void _onChar(wxKeyEvent& ev);
	void _onSearch(SearchEvent& ev);
};

// wx event macros
wxDECLARE_EVENT(EV_TREEVIEW_SEARCH_EVENT, TreeView::SearchEvent);
#define SearchEventHandler(func) wxEVENT_HANDLER_CAST(wxutil::TreeView::SearchHandlerFunction, func)

} // namespace
