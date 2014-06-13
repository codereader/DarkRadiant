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
 */
class TreeModelFilter : 
	public TreeModel
{
protected:
	TreeModel* _childModel;

public:
	TreeModelFilter(TreeModel* childModel) :
		TreeModel(childModel->GetColumns(), childModel->IsListModel()),
		_childModel(childModel)
	{
		_childModel->IncRef();
	}

	virtual ~TreeModelFilter()
	{
		_childModel->DecRef();
	}

	// Methods only provided by TreeModelFilter to implement filtering

	// wxTODO

	// We need to provide all necessary TreeModel methods, 
	// so below is a list of wrappers or adapters.

	// Add a new item below the root element
	virtual Row AddItem()
	{
		return _childModel->AddItem();
	}

	// Add a new item below the given element
	virtual Row AddItem(const wxDataViewItem& parent)
	{
		return _childModel->AddItem(parent);
	}

	// Removes the item, returns TRUE on success
	virtual bool RemoveItem(const wxDataViewItem& item)
	{
		return _childModel->RemoveItem(item);
	}

	// Remove all items matching the predicate, returns the number of deleted items
	virtual int RemoveItems(const std::function<bool (const Row&)>& predicate)
	{
		return _childModel->RemoveItems(predicate);
	}

	// Returns a Row reference to the topmost element
	virtual Row GetRootItem()
	{
		return _childModel->GetRootItem();
	}

	// Removes all items - internally the root node will be kept, but cleared too
	// This also fires the "Cleared" event to any listeners
	virtual void Clear()
	{
		_childModel->Clear();
	}

	virtual void SetDefaultStringSortColumn(int index)
	{
		_childModel->SetDefaultStringSortColumn(index);
	}

	virtual void SetHasDefaultCompare(bool hasDefaultCompare)
	{
		_childModel->SetHasDefaultCompare(hasDefaultCompare);
	}

	// Visit each node in the model, excluding the internal root node
	virtual void ForeachNode(const VisitFunction& visitFunction)
	{
		_childModel->ForeachNode(visitFunction);
	}

	// Sorts the entire tree using the given sort function
	virtual void SortModel(const SortFunction& sortFunction)
	{
		_childModel->SortModel(sortFunction);
	}

	// Sort the model by a string-valued column, sorting folders on top.
	// Pass a boolean-valued "is-a-folder" column to indicate which items are actual folders.
	virtual void SortModelFoldersFirst(const Column& stringColumn, const Column& isFolderColumn)
	{
		_childModel->SortModelFoldersFirst(stringColumn, isFolderColumn);
	}

	virtual wxDataViewItem FindString(const std::string& needle, int column)
	{
		return _childModel->FindString(needle, column);
	}

	virtual wxDataViewItem FindInteger(long needle, int column)
	{
		return _childModel->FindInteger(needle, column);
	}

	virtual void SetAttr(const wxDataViewItem& item, unsigned int col, const wxDataViewItemAttr& attr) const
	{
		_childModel->SetAttr(item, col, attr);
	}

	virtual void SetIsListModel(bool isListModel)
	{
		_childModel->SetIsListModel(isListModel);
	}

	// Base class implementation / overrides

	virtual bool HasContainerColumns(const wxDataViewItem& item) const
	{ 
		return _childModel->HasContainerColumns(item);
	}

	virtual bool HasDefaultCompare() const
	{ 
		return _childModel->HasDefaultCompare();
	}

	virtual unsigned int GetColumnCount() const
	{ 
		return _childModel->GetColumnCount();
	}

    // return type as reported by wxVariant
    virtual wxString GetColumnType(unsigned int col) const
	{
		return _childModel->GetColumnType(col);
	}

    // get value into a wxVariant
    virtual void GetValue(wxVariant &variant,
                          const wxDataViewItem &item, unsigned int col) const
	{
		_childModel->GetValue(variant, item, col);
	}

	virtual bool SetValue(const wxVariant &variant,
                          const wxDataViewItem &item,
                          unsigned int col)
	{
		return _childModel->SetValue(variant, item, col);
	}

	virtual bool GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const
	{
		return _childModel->GetAttr(item, col, attr);
	}

	virtual wxDataViewItem GetParent(const wxDataViewItem &item) const
	{
		return _childModel->GetParent(item);
	}

    virtual bool IsContainer(const wxDataViewItem& item) const
	{
		return _childModel->IsContainer(item);
	}

	virtual unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
	{
		return _childModel->GetChildren(item, children);
	}

	virtual wxDataViewItem GetRoot()
	{
		return _childModel->GetRoot();
	}

	virtual bool IsListModel() const
	{
		return _childModel->IsListModel();
	}

	virtual int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending) const
	{
		return _childModel->Compare(item1, item2, column, ascending);
	}
};

} // namespace
