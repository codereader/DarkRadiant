#pragma once

#include "TreeModel.h"

namespace wxutil
{

/**
 * Filter class that can be created on top of an existing
 * TreeModel instance to filter its data and return only
 * those items that are matching the given criteria.
 *
 * Once a TreeModelFilter is constructede it will increase
 * the refcount of the underlying TreeModel. When the 
 * TreeModelFilter is destroyed, the reference count of the 
 * child TreeModel is decreased as well, so it's safe to 
 * transfer the ownership of the child model to this filter.
 *
 * Upon constructor, the root node of the child model will be shared
 * by this instance, all settable properties like "hasDefaultCompare" 
 * will be copied over, but can be changed independently for this instance.
 *
 * Note that the TreeModelFilter cannot provide a different ordering scheme
 * than the child model, any Sort*() calls are issued directly to the child.
 */
class TreeModelFilter : 
	public TreeModel
{
public:
    typedef std::function<bool(Row&)> VisibleFunc;

protected:
    TreeModel::Ptr _childModel;

	// When the child model issues any events, we need to pass them to the
	// wxDataViewCtrl owning this TreeModelFilter.
	class ChildModelNotifier;

	ChildModelNotifier* _notifier;

	// The filter column
	const Column* _filterColumn;

    // Custom filter logic
    VisibleFunc _customVisibleFunc;

public:
    typedef wxObjectDataPtr<TreeModelFilter> Ptr;

	TreeModelFilter(TreeModel::Ptr childModel, const Column* filterColumn = NULL);

	virtual ~TreeModelFilter();
	
	// Methods only provided by TreeModelFilter to implement filtering

    TreeModel::Ptr GetChildModel();

	// Set the boolean-valued column filtering the child model
	void SetFilterColumn(const Column& column);

    // Alternative to SetFilterColumn: use a custom function to evaluate
    // whether a given row is visible or not.
    // A non-empty VisibleFunc always takes precedence over a filter column.
    void SetVisibleFunc(const VisibleFunc& visibleFunc);

	bool ItemIsVisible(const wxDataViewItem& item) const;
    bool ItemIsVisible(Row& row) const;

	// We need to provide some TreeModel methods on our own, 
	// to implement filtering

	// Visit each node in the model, excluding the internal root node
	virtual void ForeachNode(const VisitFunction& visitFunction);

	virtual wxDataViewItem FindString(const std::string& needle, int column);
	virtual wxDataViewItem FindInteger(long needle, int column);
	
    virtual bool IsContainer(const wxDataViewItem& item) const;

	virtual unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const;
};

} // namespace
