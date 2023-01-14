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

	std::vector<TreeModel::Column> _colsToSearch;

	bool _collapseRecursively;
    bool _searchPopupEnabled;

	TreeView(wxWindow* parent, wxDataViewModel* model, long style);

public:
    typedef wxWindowPtr<TreeView> Ptr;

	// Create a TreeView without model (single-selection mode)
	static TreeView* Create(wxWindow* parent, long style = wxDV_SINGLE);

    /**
     * \brief
     * Construct a TreeView with an associated model
     *
     * \param parent
     * The parent window of the tree view widget
     *
     * \param model
     * Data model to supply data for the tree view. May be null.
     */
    static TreeView* CreateWithModel(wxWindow* parent, wxDataViewModel* model,
                                     long style = wxDV_SINGLE);

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

	// When enabled, collapsing a single item will automatically collapse all child items too
	void EnableRecursiveCollapse(bool enabled);

	// Removes any sort keys
	void ResetSortingOnAllColumns();

	// Adds a column to search when the user starts typing
	void AddSearchColumn(const TreeModel::Column& column);

    // Returns true if the treeview search popup is currently visible
    bool HasActiveSearchPopup();

    // Enable/Disable the search popup (which is enabled by default)
    void EnableSearchPopup(bool enabled);

#if !defined(__linux__)
    // Triggers a rebuild of the tree (done by calling ItemDeleted+ItemAdded for each
    // of the root's immediate children.
    void Rebuild();
#endif

    // Stops any ongoing inline-editing in any column
    void CancelEditing();

protected:
    // Synthesises a selection change event to notify any handlers
    void SendSelectionChangeEvent(const wxDataViewItem& item);

	void JumpToSearchMatch(const wxDataViewItem& item);

private:
	void CollapseChildren(const wxDataViewItem& item);

	void CloseSearch();

	void _onItemCollapsing(wxDataViewEvent& ev);
	void _onItemExpanded(wxDataViewEvent& ev);
	void _onChar(wxKeyEvent& ev);
	void _onItemActivated(wxDataViewEvent& ev);
};
} // namespace
