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
protected:
	TreeModel* _childModel;

	// When the child model issues any events, we need to pass them to the
	// wxDataViewCtrl owning this TreeModelFilter.
	class ChildModelNotifier : 
		public wxDataViewModelNotifier
	{
	private:
		TreeModelFilter* _owner;
	public:
		ChildModelNotifier(TreeModelFilter* owner) :
			_owner(owner)
		{}

		virtual bool ItemAdded(const wxDataViewItem& parent, const wxDataViewItem& item)
		{ 
			// Only pass the call if the item is relevant according to the filter criteria
			if (_owner->ItemIsVisible(parent) && _owner->ItemIsVisible(item))
			{
				return _owner->ItemAdded(parent, item);
			}

			return true;
		}

		virtual bool ItemDeleted(const wxDataViewItem& parent, const wxDataViewItem& item)
		{ 
			if (_owner->ItemIsVisible(parent) && _owner->ItemIsVisible(item))
			{
				return _owner->ItemDeleted(parent, item);
			}

			return true;
		}

		virtual bool ItemChanged(const wxDataViewItem& item)
		{
			if (_owner->ItemIsVisible(item))
			{
				return _owner->ItemChanged(item);
			}

			return true;
		
		}
		virtual bool ValueChanged(const wxDataViewItem& item, unsigned int col)
		{ 
			if (_owner->ItemIsVisible(item))
			{
				return _owner->ValueChanged(item, col);
			}

			return _owner->ValueChanged(item, col); 
		}

		virtual bool Cleared()
		{ 
			return _owner->Cleared();
		}

		virtual void Resort()
		{ 
			_owner->Resort(); 
		}
	};

	ChildModelNotifier* _notifier;

	// The filter column
	const Column* _filterColumn;

public:
	TreeModelFilter(TreeModel* childModel) :
		TreeModel(*childModel), // reference the existing model
		_childModel(childModel)
	{
		_childModel->IncRef();

		_notifier = new ChildModelNotifier(this);
		_childModel->AddNotifier(_notifier);
	}

	virtual ~TreeModelFilter()
	{
		_childModel->RemoveNotifier(_notifier);
		_childModel->DecRef();
	}
	
	// Methods only provided by TreeModelFilter to implement filtering

	TreeModel* GetChildModel()
	{
		return _childModel;
	}

	// Set the boolean-valued column filtering the child model
	void SetFilterColumn(const Column& column)
	{
		assert(column.type == Column::Bool);
		_filterColumn = &column;
	}

	bool ItemIsVisible(const wxDataViewItem& item)
	{
		if (!item.IsOk() || _filterColumn == NULL) return true;

		Row row(item, *const_cast<TreeModelFilter*>(this));

		return row[*_filterColumn].getBool();
	}

	// We need to provide some TreeModel methods on our own, 
	// to implement filtering

	// Visit each node in the model, excluding the internal root node
	virtual void ForeachNode(const VisitFunction& visitFunction)
	{
		_childModel->ForeachNode([&] (Row& row)
		{
			// Only visit unfiltered items
			if (_filterColumn == NULL || row[*_filterColumn].getBool())
			{
				visitFunction(row);
			}
		});
	}

	virtual wxDataViewItem FindString(const std::string& needle, int column)
	{
		return FindRecursiveUsingRows(getRootNode(), [&] (Row& row)->bool
		{
			if (_filterColumn != NULL && row[*_filterColumn].getBool() == false)
			{
				return false; // skip filtered items
			}

			return static_cast<std::string>(row[GetColumns()[column]]) == needle;
		});
	}

	virtual wxDataViewItem FindInteger(long needle, int column)
	{
		return FindRecursiveUsingRows(getRootNode(), [&] (Row& row)->bool
		{
			if (_filterColumn != NULL && row[*_filterColumn].getBool() == false)
			{
				return false; // skip filtered items
			}

			return row[GetColumns()[column]].getInteger() == needle;
		});
	}
	
    virtual bool IsContainer(const wxDataViewItem& item) const
	{
		bool isContainer = _childModel->IsContainer(item);

		if (!isContainer) 
		{
			return false;
		}

		// Check if the node actually has visible children
		wxDataViewItemArray children;
		return GetChildren(item, children) > 0;
	}

	virtual unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
	{
		if (_filterColumn == NULL)
		{
			return _childModel->GetChildren(item, children);
		}

		// Get the raw child list
		wxDataViewItemArray unfilteredChildren;
		_childModel->GetChildren(item, unfilteredChildren);

		// Only add the visible ones to the result set
		std::for_each(unfilteredChildren.begin(), unfilteredChildren.end(), [&] (const wxDataViewItem& item)
		{
			Row row(item, *const_cast<TreeModelFilter*>(this));

			if (row[*_filterColumn].getBool())
			{
				children.Add(item);
			}
		});

		return children.size();
	}
};

} // namespace
