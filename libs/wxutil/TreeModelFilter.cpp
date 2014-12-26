#include "TreeModelFilter.h"

#include <algorithm>

namespace wxutil
{

class TreeModelFilter::ChildModelNotifier : 
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

TreeModelFilter::TreeModelFilter(TreeModel::Ptr childModel, const Column* filterColumn) :
	TreeModel(*childModel), // reference the existing model
	_childModel(childModel),
	_notifier(NULL),
	_filterColumn(NULL)
{
	_notifier = new ChildModelNotifier(this);
	_childModel->AddNotifier(_notifier);

	if (filterColumn != NULL)
	{
		SetFilterColumn(*filterColumn);
	}
}

TreeModelFilter::~TreeModelFilter()
{
	_childModel->RemoveNotifier(_notifier);
}
	
TreeModel::Ptr TreeModelFilter::GetChildModel()
{
	return _childModel;
}

void TreeModelFilter::SetFilterColumn(const Column& column)
{
	assert(column.type == Column::Boolean);
	_filterColumn = &column;
}

bool TreeModelFilter::ItemIsVisible(const wxDataViewItem& item)
{
	if (!item.IsOk() || _filterColumn == NULL) return true;

	Row row(item, *const_cast<TreeModelFilter*>(this));

	return row[*_filterColumn].getBool();
}

void TreeModelFilter::ForeachNode(const VisitFunction& visitFunction)
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

wxDataViewItem TreeModelFilter::FindString(const std::string& needle, int column)
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

wxDataViewItem TreeModelFilter::FindInteger(long needle, int column)
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
	
bool TreeModelFilter::IsContainer(const wxDataViewItem& item) const
{
    if (!item.IsOk())
    {
        return true;
    }

#ifdef __WXGTK__
    // greebo: The GTK DataViewCtrl implementation treats nodes differently
    // based on whether they have children or not. If a tree model node has no children
    // now it's not possible to add any children later on, causing assertions.
    // wxGTK wants to know *in advance* whether a node has children, so let's assume true
    // unless this is a listmodel (in which case non-root nodes never have children)
    return !_isListModel ? true : false;
#else
    bool isContainer = _childModel->IsContainer(item);

    if (!isContainer)
    {
        return false;
    }

	// Check if the node actually has visible children
	wxDataViewItemArray children;
	return GetChildren(item, children) > 0;
#endif
}

unsigned int TreeModelFilter::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
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

} // namespace
