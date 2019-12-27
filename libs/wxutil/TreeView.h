#pragma once

#include <memory>
#include <wx/dataview.h>
#include <wx/windowptr.h>

#include "TreeModel.h"

namespace wxutil
{

/**
 * greebo: Extension of the regular wxDataViewCtrl to add
 * a few needed improvements, like automatic column sizing
 * for treeviews (a thing that seems to be problematic in the 
 * pure wxDataViewCtrl) and type-ahead searching.
 *
 * Use the named constructors Create*() to instantiate a new TreeView.
 */
class TreeView :
	public wxDataViewCtrl
{
protected:
	class Search;
	std::unique_ptr<Search> _search;

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

    // override wxDataViewCtrl to make it more robust
    virtual bool AssociateModel(wxDataViewModel* model) override;

	// Enable the automatic recalculation of column widths
	void EnableAutoColumnWidthFix(bool enable = true);
	
    // Trigger an ItemChanged event on all the children of the given node
    // Call this if your text columns get squashed and are not sized correctly
    void TriggerColumnSizeEvent(const wxDataViewItem& item = wxDataViewItem());

	// Expands the first level of all the top-level items
	void ExpandTopLevelItems();

	// Removes any sort keys
	void ResetSortingOnAllColumns();

	// Adds a column to search when the user starts typing
	void AddSearchColumn(const TreeModel::Column& column);

    // Returns true if the treeview search popup is currently visible
    bool HasActiveSearchPopup();

#if !defined(__linux__)
    // Triggers a rebuild of the tree (done by calling ItemDeleted+ItemAdded for each
    // of the root's immediate children.
    void Rebuild();
#endif

private:
	void CloseSearch();
	void JumpToSearchMatch(const wxDataViewItem& item);

	void _onItemExpanded(wxDataViewEvent& ev);
	void _onChar(wxKeyEvent& ev);
	void _onItemActivated(wxDataViewEvent& ev);
};

} // namespace
