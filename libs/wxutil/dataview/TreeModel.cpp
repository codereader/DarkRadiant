#include "TreeModel.h"

#include <algorithm>
#include <functional>

namespace wxutil
{

wxString TreeModel::Column::getWxType() const
{
	static std::vector<wxString> types(NumTypes);

	if (types.empty())
	{
		types[String] = "string";
		// we store numbers as strings
		types[Integer] = "string";
		types[Double] = "string";
		types[Boolean] = "bool";
		types[Icon] = "icon";
		types[IconText] = "wxDataViewIconText";
		types[Pointer] = "void*";
	}

	return types[type];
}

// TreeModel nodes form a directed acyclic graph
class TreeModel::Node :
	public util::Noncopyable
{
public:
	Node* parent; // NULL for the root node

	// The item will use a Node* pointer as ID, which is possible
	// since the wxDataViewItem is using void* as ID type
	wxDataViewItem item;

	typedef std::vector<wxVariant> Values;
	Values values;

	typedef std::vector<NodePtr> Children;
	Children children;

	typedef std::vector<wxDataViewItemAttr> Attributes;
	Attributes attributes;

	typedef std::vector<bool> EnabledFlags; // Each value can be flagged as enabled/disabled
	EnabledFlags enabledFlags;

	// Public constructor, does not accept NULL pointers
	Node(Node* parent_) :
		parent(parent_),
		item(reinterpret_cast<wxDataViewItem::Type>(this))
	{
		// Use CreateRoot() instead of passing NULL
		assert(parent_ != nullptr);
	}

private:
	// Private constructor creates a root node, has a wxDataViewItem ID == NULL
	Node() :
		parent(nullptr),
		item(nullptr)
	{}

public:
	static NodePtr createRoot()
	{
		return NodePtr(new Node());
	}

	bool remove(TreeModel::Node* child)
	{
		for (Children::iterator i = children.begin();
			 i != children.end(); ++i)
		{
			if (i->get() == child)
			{
				children.erase(i);
				return true;
			}
		}

		return false;
	}
};

// -------------------------------------------------------------------------------

wxDEFINE_EVENT(EV_TREEMODEL_POPULATION_FINISHED, TreeModel::PopulationFinishedEvent);
wxDEFINE_EVENT(EV_TREEMODEL_POPULATION_PROGRESS, TreeModel::PopulationProgressEvent);

TreeModel::PopulationFinishedEvent::PopulationFinishedEvent(int id) : 
	wxEvent(id, EV_TREEMODEL_POPULATION_FINISHED),
	_treeModel(NULL)
{}

TreeModel::PopulationFinishedEvent::PopulationFinishedEvent(TreeModel::Ptr store, int id) :
	wxEvent(id, EV_TREEMODEL_POPULATION_FINISHED),
	_treeModel(store)
{}
 
// You *must* copy here the data to be transported
TreeModel::PopulationFinishedEvent::PopulationFinishedEvent(const TreeModel::PopulationFinishedEvent& event) :  
	wxEvent(event),
	_treeModel(event._treeModel)
{}
 
// Required for sending with wxPostEvent()
wxEvent* TreeModel::PopulationFinishedEvent::Clone() const
{ 
	return new PopulationFinishedEvent(*this);
}
 
TreeModel::Ptr TreeModel::PopulationFinishedEvent::GetTreeModel() const
{ 
	return _treeModel;
}

void TreeModel::PopulationFinishedEvent::SetTreeModel(TreeModel::Ptr store)
{ 
	_treeModel = store;
}

TreeModel::PopulationProgressEvent::PopulationProgressEvent(int id) :
    wxEvent(id, EV_TREEMODEL_POPULATION_PROGRESS)
{}

TreeModel::PopulationProgressEvent::PopulationProgressEvent(const wxString& message, int id) :
    wxEvent(id, EV_TREEMODEL_POPULATION_PROGRESS),
    _message(message)
{}


TreeModel::PopulationProgressEvent::PopulationProgressEvent(const TreeModel::PopulationProgressEvent& event) :
    wxEvent(event),
    _message(event._message)
{}

wxEvent* TreeModel::PopulationProgressEvent::Clone() const
{
    return new PopulationProgressEvent(*this);
}

const wxString& TreeModel::PopulationProgressEvent::GetMessage() const
{
    return _message;
}

void TreeModel::PopulationProgressEvent::SetMessage(const wxString& message)
{
    _message = message;
}

// -------------------------------------------------------------------------------

TreeModel::TreeModel(const ColumnRecord& columns, bool isListModel) :
	_columns(columns),
	_rootNode(Node::createRoot()),
	_defaultStringSortColumn(-1),
	_hasDefaultCompare(false),
	_isListModel(isListModel)
{}

TreeModel::TreeModel(const TreeModel& existingModel) :
	_columns(existingModel._columns),
	_rootNode(existingModel._rootNode),
	_defaultStringSortColumn(existingModel._defaultStringSortColumn),
	_hasDefaultCompare(existingModel._hasDefaultCompare),
	_isListModel(existingModel._isListModel)
{}

TreeModel::~TreeModel()
{}

const TreeModel::NodePtr& TreeModel::getRootNode() const
{
	return _rootNode;
}

const TreeModel::ColumnRecord& TreeModel::GetColumns() const
{
	return _columns;
}

TreeModel::Row TreeModel::AddItem()
{
	return AddItem(_rootNode->item);
}

TreeModel::Row TreeModel::AddItem(const wxDataViewItem& parent)
{
	// Redirect to the root node for invalid items
	Node* parentNode = !parent.IsOk() ? _rootNode.get() : static_cast<Node*>(parent.GetID());

	NodePtr node(new Node(parentNode));

	parentNode->children.push_back(node);

	return Row(node->item, *this);
}

bool TreeModel::RemoveItem(const wxDataViewItem& item)
{
	if (item.IsOk())
	{
		Node* node = static_cast<Node*>(item.GetID());
		Node* parent = node->parent;

		if (parent == NULL) return false; // cannot remove the root node

		if (parent->remove(node))
		{
			ItemDeleted(parent->item, item);
			return true;
		}
	}

	return false;
}

int TreeModel::RemoveItems(const std::function<bool (const TreeModel::Row&)>& predicate)
{
	return RemoveItemsRecursively(GetRoot(), predicate);
}

int TreeModel::RemoveItemsRecursively(const wxDataViewItem& parent, const std::function<bool (const TreeModel::Row&)>& predicate)
{
	Node* parentNode = !parent.IsOk() ? _rootNode.get() : static_cast<Node*>(parent.GetID());

	int deleteCount = 0;
	wxDataViewItemArray itemsToDelete;

	for (Node::Children::const_iterator i = parentNode->children.begin();
			i != parentNode->children.end(); ++i)
	{
		Row row((*i)->item, *this);

		if (predicate(row))
		{
			itemsToDelete.push_back((*i)->item);
		}
	}

	if (!itemsToDelete.IsEmpty())
	{
		// It seems that the wxDataViewCtrl has trouble in case a highlighted row is removed
		// and the actual nodes have already been deleted, so remove them afterwards.
		ItemsDeleted(parent, itemsToDelete);

		// Remove these items
		std::for_each(itemsToDelete.begin(), itemsToDelete.end(), [&] (const wxDataViewItem& item)
		{
			Node* nodeToDelete = static_cast<Node*>(item.GetID());
			parentNode->remove(nodeToDelete);
			deleteCount++;
		});
	}

	for (Node::Children::const_iterator i = parentNode->children.begin();
			i != parentNode->children.end(); ++i)
	{
		deleteCount += RemoveItemsRecursively((*i)->item, predicate);
	}

	return deleteCount;
}

TreeModel::Row TreeModel::GetRootItem()
{
	return Row(GetRoot(), *this);
}

void TreeModel::Clear()
{
    // This workaround seems to cause crashes in wxGTK 3.2.0, take it out (#6105)
#if wxCHECK_VERSION(3, 0, 5) && !wxCHECK_VERSION(3, 2, 0)
	// To work around a problem in wxGTK 3.0.5+, trigger 
	// an ItemRemoved call for all top-level children before 
	// actually deleting them.
	// The Cleared() call below might query GetParent() calls
	// for nodes that are still present in the internal tree
	wxDataViewItemArray children;
	GetChildren(_rootNode->item, children);

	if (!children.empty())
	{
		ItemsDeleted(_rootNode->item, children);
	}
#endif

	// Now it should be safe to free all the nodes
	_rootNode->values.clear();
	_rootNode->children.clear();
	
	Cleared();
}

void TreeModel::SetDefaultStringSortColumn(int index)
{
	_defaultStringSortColumn = index;
}

void TreeModel::SetHasDefaultCompare(bool hasDefaultCompare)
{
	_hasDefaultCompare = hasDefaultCompare;
}

void TreeModel::ForeachNode(const TreeModel::VisitFunction& visitFunction)
{
	// Skip the root node and traverse its immediate children recursively
	std::for_each(_rootNode->children.begin(), _rootNode->children.end(), [&] (const NodePtr& node)
	{
		ForeachNodeRecursive(node, visitFunction);
	});
}

void TreeModel::ForeachNodeRecursive(const TreeModel::NodePtr& node, const TreeModel::VisitFunction& visitFunction)
{
	wxutil::TreeModel::Row row(node->item, *this);
	visitFunction(row);

	// Enter the recursion
	std::for_each(node->children.begin(), node->children.end(), [&] (const NodePtr& child)
	{
		ForeachNodeRecursive(child, visitFunction);
	});
}

void TreeModel::ForeachNodeReverse(const TreeModel::VisitFunction& visitFunction)
{
	// Skip the root node and traverse its immediate children recursively
	for (Node::Children::const_reverse_iterator i = _rootNode->children.rbegin(); i != _rootNode->children.rend(); ++i)
	{
		ForeachNodeRecursiveReverse(*i, visitFunction);
	}
}

void TreeModel::ForeachNodeRecursiveReverse(const TreeModel::NodePtr& node, const TreeModel::VisitFunction& visitFunction)
{
	wxutil::TreeModel::Row row(node->item, *this);
	visitFunction(row);

	// Enter the recursion
	for (Node::Children::const_reverse_iterator i = node->children.rbegin(); i != node->children.rend(); ++i)
	{
		ForeachNodeRecursiveReverse(*i, visitFunction);
	}
}

void TreeModel::SortModel(const TreeModel::SortFunction& sortFunction)
{
	SortModelRecursively(_rootNode.get(), sortFunction);
}

void TreeModel::SortModelByColumn(const TreeModel::Column& column)
{
	SortModelRecursively(_rootNode.get(), [&](const wxDataViewItem& a, const wxDataViewItem& b)->bool
	{
		Row rowA(a, *this);
		Row rowB(b, *this);

		if (column.type == Column::IconText)
		{
			wxDataViewIconText txtA = rowA[column];
			wxDataViewIconText txtB = rowB[column];

			return txtA.GetText() < txtB.GetText();
		}
		else if (column.type == Column::String)
		{
			std::string txtA = rowA[column];
			std::string txtB = rowB[column];

			return txtA < txtB;
		}
		else if (column.type == Column::Integer)
		{
			int intA = rowA[column].getInteger();
			int intB = rowA[column].getInteger();
			
			return intA < intB;
		}
		else if (column.type == Column::Double)
		{
			double dblA = rowA[column].getDouble();
			double dblB = rowA[column].getDouble();
			
			return dblA < dblB;
		}
		
		return false;
	});
}

void TreeModel::SortModelFoldersFirst(const Column& stringColumn, const Column& isFolderColumn)
{
    // Pass an empty item to start at the root element
    SortModelFoldersFirst(wxDataViewItem(), stringColumn, isFolderColumn);
}

void TreeModel::SortModelFoldersFirst(const wxDataViewItem& startItem, const Column& stringColumn, const Column& isFolderColumn)
{
    // Pass an empty custom folder comparer
    SortModelFoldersFirst(startItem, stringColumn, isFolderColumn, FolderCompareFunction());
}

void TreeModel::SortModelFoldersFirst(const Column& stringColumn, const Column& isFolderColumn,
    const FolderCompareFunction& customFolderSortFunc)
{
    // Pass an empty item to start at the root element
    SortModelFoldersFirst(wxDataViewItem(), stringColumn, isFolderColumn, customFolderSortFunc);
}

void TreeModel::SortModelFoldersFirst(const wxDataViewItem& startItem, const Column& stringColumn,
    const Column& isFolderColumn, const FolderCompareFunction& customFolderSortFunc)
{
    auto startNode = !startItem.IsOk() ? _rootNode.get() : static_cast<Node*>(startItem.GetID());

    SortModelRecursively(startNode, std::bind(&TreeModel::CompareFoldersFirst,
        this,
        std::placeholders::_1,
        std::placeholders::_2,
        stringColumn,
        stringColumn.type == Column::String ? CompareStringVariants : CompareIconTextVariants,
        isFolderColumn,
        customFolderSortFunc));
}

void TreeModel::SortModelRecursively(Node* node, const TreeModel::SortFunction& sortFunction)
{
	// Use std::sort algorithm and small lambda to only pass wxDataViewItems to the client sort function
	std::sort(node->children.begin(), node->children.end(), [&] (const NodePtr& a, const NodePtr& b)->bool
	{
        return sortFunction(a->item, b->item);
	});

	// Enter recursion
	std::for_each(node->children.begin(), node->children.end(), [&] (const NodePtr& child)
	{
        SortModelRecursively(child.get(), sortFunction);
	});
}

wxDataViewItem TreeModel::FindString(const std::string& needle, const Column& column)
{
    return FindString(needle, column, wxDataViewItem());
}

wxDataViewItem TreeModel::FindString(const std::string& needle, const Column& column, const wxDataViewItem& startItem)
{
    auto* startNode = !startItem.IsOk() ? _rootNode.get() : static_cast<Node*>(startItem.GetID());

	return FindRecursive(*startNode, [&] (const Node& node)->bool
	{
		int colIndex = column.getColumnIndex();

		if (column.type == Column::IconText)
		{
			if (static_cast<int>(node.values.size()) > colIndex)
			{
				wxDataViewIconText iconText;
				iconText << node.values[colIndex];

				return iconText.GetText() == needle;
			}
		}
		else if (column.type == Column::String)
		{
			return static_cast<int>(node.values.size()) > colIndex && 
				static_cast<std::string>(node.values[colIndex]) == needle;
		}

		return false;
	});
}

wxDataViewItem TreeModel::FindInteger(long needle, const Column& column)
{
    return FindInteger(needle, column, wxDataViewItem());
}

wxDataViewItem TreeModel::FindInteger(long needle, const Column& column, const wxDataViewItem& startItem)
{
    auto* startNode = !startItem.IsOk() ? _rootNode.get() : static_cast<Node*>(startItem.GetID());

	return FindRecursive(*startNode, [&] (const Node& node)->bool
	{
		int colIndex = column.getColumnIndex();
		return static_cast<int>(node.values.size()) > colIndex && 
			static_cast<long>(node.values[colIndex]) == needle;
	});
}

wxDataViewItem TreeModel::FindItem(const std::function<bool(const TreeModel::Row&)>& predicate)
{
    return FindItem(predicate, wxDataViewItem());
}

wxDataViewItem TreeModel::FindItem(const std::function<bool(const TreeModel::Row&)>& predicate, const wxDataViewItem& startItem)
{
    auto* startNode = !startItem.IsOk() ? _rootNode.get() : static_cast<Node*>(startItem.GetID());

    return FindRecursive(*startNode, [&](const Node& node)->bool
    {
        Row row(node.item, *this);
        return predicate(row);
    });
}

wxDataViewItem TreeModel::FindRecursive(const TreeModel::Node& node, const std::function<bool (const TreeModel::Node&)>& predicate)
{
	// Test the node itself
	if (predicate(node))
	{
		return node.item;
	}

	// Then test all children, aborting on first success
	for (const auto& child : node.children)
	{
		wxDataViewItem item = FindRecursive(*child, predicate);

		if (item.IsOk())
		{
			return item;
		}
	}

	// Return an empty data item, which is "not ok"
	return wxDataViewItem();
}

wxDataViewItem TreeModel::FindRecursiveUsingRows(const TreeModel::Node& node, const std::function<bool (TreeModel::Row&)>& predicate)
{
	if (node.item.IsOk())
	{
		Row row(node.item, *this);

		// Test the node itself
		if (predicate(row))
		{
			return node.item;
		}
	}

	// Then test all children, aborting on first success
	for (const auto& child : node.children)
	{
		wxDataViewItem item = FindRecursiveUsingRows(*child, predicate);

		if (item.IsOk())
		{
			return item;
		}
	}

	// Return an empty data item, which is "not ok"
	return wxDataViewItem();
}

bool TreeModel::RowContainsString(const Row& row, const wxString& value, const std::vector<Column>& columnsToSearch, bool lowerStrings)
{
    for (const auto& column : columnsToSearch)
    {
        auto columnValue = row[column].getString();

        if (lowerStrings)
        {
            columnValue.MakeLower();
        }

        if (columnValue.Contains(value))
        {
            return true;
        }
    }

    return false;
}

bool TreeModel::HasDefaultCompare() const
{
	return _hasDefaultCompare;
}

unsigned int TreeModel::GetColumnCount() const
{
	return static_cast<unsigned int>(_columns.size());
};

// return type as reported by wxVariant
wxString TreeModel::GetColumnType(unsigned int col) const
{
	return _columns[col].getWxType();
}

// get value into a wxVariant
void TreeModel::GetValue(wxVariant &variant,
                         const wxDataViewItem &item, unsigned int col) const
{
	Node* owningNode = item.IsOk() ? static_cast<Node*>(item.GetID()) : _rootNode.get();

	if (col < owningNode->values.size())
	{
		variant = owningNode->values[col];
	}
}

bool TreeModel::SetValue(const wxVariant& variant,
                        const wxDataViewItem& item,
                        unsigned int col)
{
	Node* owningNode = item.IsOk() ? static_cast<Node*>(item.GetID()) : _rootNode.get();

	if (owningNode->values.size() < col + 1)
	{
		owningNode->values.resize(col + 1);
	}
	
	owningNode->values[col] = variant;

	return true;
}

bool TreeModel::GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const
{
	if (!item.IsOk())
	{
		return false;
	}

	Node* owningNode = static_cast<Node*>(item.GetID());

	if (col < owningNode->attributes.size())
	{
		attr = owningNode->attributes[col];
		return true;
	}

	return false;
}

void TreeModel::SetAttr(const wxDataViewItem& item, unsigned int col, const wxDataViewItemAttr& attr) const
{
	if (!item.IsOk())
	{
		return;
	}

	Node* owningNode = static_cast<Node*>(item.GetID());

	if (owningNode->attributes.size() < col + 1)
	{
		owningNode->attributes.resize(col + 1);
	}

	owningNode->attributes[col] = attr;
}

wxDataViewItem TreeModel::GetParent(const wxDataViewItem& item) const
{
	// It's ok to ask for invisible root node's parent
	if (!item.IsOk())
	{
		return wxDataViewItem(NULL);	
	}

	Node* owningNode = static_cast<Node*>(item.GetID());

	if (owningNode->parent != NULL)
	{
		return owningNode->parent->item;
	}
	
	return wxDataViewItem(NULL);
}

bool TreeModel::IsContainer(const wxDataViewItem& item) const
{
	// greebo: it appears that invalid items are treated as containers, they can have children. 
	// The wxDataViewCtrl has such a root node with invalid items
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
	// Regular implementation: return true if this node has child nodes
	Node* owningNode = static_cast<Node*>(item.GetID());

	return owningNode != NULL && !owningNode->children.empty();
#endif
}

bool TreeModel::HasContainerColumns(const wxDataViewItem&) const
{
    return !_isListModel; // we want all columns to show even if the item has children
}

unsigned int TreeModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
{
	// Requests for invalid items are asking for our root children, actually
	Node* owningNode = !item.IsOk() ? _rootNode.get() : static_cast<Node*>(item.GetID());

	for (Node::Children::const_iterator iter = owningNode->children.begin(); iter != owningNode->children.end(); ++iter)
	{
		children.Add((*iter)->item);
	}

	return static_cast<unsigned int>(owningNode->children.size());
}

wxDataViewItem TreeModel::GetRoot()
{
	// The root item carries the NULL pointer, the other methods need to be able to deal with that
	return _rootNode->item;
}

bool TreeModel::IsListModel() const
{
	return _isListModel;
}

void TreeModel::SetIsListModel(bool isListModel)
{
	_isListModel = isListModel;
}

int TreeModel::Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending) const
{
	Node* node1 = static_cast<Node*>(item1.GetID());
	Node* node2 = static_cast<Node*>(item2.GetID());

	if (!node1 || !node2)
		return 0;

#if 0 // Don't handle folders first sorting here
	if (!node1->children.empty() && node2->children.empty())
		return ascending ? -1 : 1;

	if (!node2->children.empty() && node1->children.empty())
		return ascending ? 1 : -1;
#endif

	if (_defaultStringSortColumn >= 0)
	{
		return ascending ? 
            node1->values[_defaultStringSortColumn].GetString().CmpNoCase(
			    node2->values[_defaultStringSortColumn].GetString()) :
                node2->values[_defaultStringSortColumn].GetString().CmpNoCase(
			    node1->values[_defaultStringSortColumn].GetString());
	}

	// When clicking on the dataviewctrl headers, we need to support some default algorithm
    
    // implement a few comparison types
    switch (_columns[column].type)
    {
        case Column::String:
        {
            return ascending ? 
                node1->values[column].GetString().CmpNoCase(node2->values[column].GetString()) :
                node2->values[column].GetString().CmpNoCase(node1->values[column].GetString());
        }

        case Column::IconText:
        {
            wxDataViewIconText val1;
            val1 << node1->values[column];

            wxDataViewIconText val2;
            val2 << node2->values[column];

            return ascending ? val1.GetText().CmpNoCase(val2.GetText()) :
                val2.GetText().CmpNoCase(val1.GetText());
        }

        case Column::Double:
        {
            double val1 = node1->values[column].GetDouble();
            double val2 = node2->values[column].GetDouble();

            if (val1 == val2) return 0;

            return ascending ? (val1 < val2 ? -1 : 1) :
                               (val2 < val1 ? -1 : 1);
        }

        case Column::Integer:
        {
            long val1 = node1->values[column].GetInteger();
            long val2 = node2->values[column].GetInteger();

            if (val1 == val2) return 0;

            return ascending ? (val1 < val2 ? -1 : 1) :
                               (val2 < val1 ? -1 : 1);
        }

        case Column::Boolean:
        {
            bool val1 = node1->values[column].GetBool();
            bool val2 = node2->values[column].GetBool();

            if (val1 == val2) return 0;

            return ascending ? (!val1 ? -1 : 1) : (val1 ? -1 : 1);
        }

        case Column::Pointer:
        {
            void* val1 = node1->values[column].GetVoidPtr();
            void* val2 = node2->values[column].GetVoidPtr();

            if (val1 == val2) return 0;

            return ascending ? (val1 < val2 ? -1 : 1) :
                               (val2 < val1 ? -1 : 1);
        }

        case Column::Icon:
        {
            return 0; // no sense in comparing icons
        }
        
        default:
            break; // break to return 0
    };

	return 0;
}

int TreeModel::CompareStringVariants(const wxVariant& a, const wxVariant& b)
{
    return a.GetString().CmpNoCase(b.GetString());
}

int TreeModel::CompareIconTextVariants(const wxVariant& a, const wxVariant& b)
{
    wxDataViewIconText aValue;
    aValue << a;

    wxDataViewIconText bValue;
    bValue << b;

    return aValue.GetText().CmpNoCase(bValue.GetText());
}

bool TreeModel::CompareFoldersFirst(const wxDataViewItem& a, const wxDataViewItem& b, 
                                    const TreeModel::Column& stringColumn,
                                    const std::function<int(const wxVariant&, const wxVariant&)>& stringCompareFunc, 
                                    const TreeModel::Column& isFolderCol,
                                    const std::function<int(const wxDataViewItem&, const wxDataViewItem&)>& folderCompareFunc)
{
	// Check if A or B are folders
	wxVariant aIsFolder, bIsFolder;
	GetValue(aIsFolder, a, isFolderCol.getColumnIndex());
	GetValue(bIsFolder, b, isFolderCol.getColumnIndex());

	if (aIsFolder)
	{
		// A is a folder, check if B is as well
		if (bIsFolder)
		{
			// A and B are both folders

            // Ask the special compare function first
            if (folderCompareFunc)
            {
                int customResult = folderCompareFunc(a, b);

                // If the custom functor returns "equal", we continue with our algorithm
                if (customResult != 0)
                {
                    return customResult < 0;
                }
            }
			
			// Compare folder names
			// greebo: We're not checking for equality here, shader names are unique
			wxVariant aName, bName;
			GetValue(aName, a, stringColumn.getColumnIndex());
			GetValue(bName, b, stringColumn.getColumnIndex());

            return stringCompareFunc(aName, bName) < 0;
		}
		else
		{
			// A is a folder, B is not, A sorts before
			return true;
		}
	}
	else
	{
		// A is not a folder, check if B is one
		if (bIsFolder)
		{
			// A is not a folder, B is, so B sorts before A
			return false;
		}
		else
		{
			// Neither A nor B are folders, compare names
			// greebo: We're not checking for equality here, names are unique
			wxVariant aName, bName;
			GetValue(aName, a, stringColumn.getColumnIndex());
			GetValue(bName, b, stringColumn.getColumnIndex());

            return stringCompareFunc(aName, bName) < 0;
		}
	}
} 

// Search functor which tries to find the next match for the given search string
// is agnostic of the search direction, it just gets invoked for each row.
class TreeModel::SearchFunctor
{
private:
	const std::vector<TreeModel::Column>& _columns;

	wxDataViewItem _previousMatch;
	wxDataViewItem _match;

	enum SearchState
	{
		SearchingForLastMatch,
		Searching,
		Found,
	};

	SearchState _state;

	wxString _searchString;

public:
	SearchFunctor(const wxString& searchString,
				  const std::vector<TreeModel::Column>& columns, 
				  const wxDataViewItem& previousMatch) :
		_columns(columns),
		_previousMatch(previousMatch),
		_state(previousMatch.IsOk() ? SearchingForLastMatch : Searching),
		_searchString(searchString.Lower())
	{}

	const wxDataViewItem& getMatch() const
	{
		return _match;
	}

	void operator()(TreeModel::Row& row)
	{
		switch (_state)
		{
		case SearchingForLastMatch:
			if (row.getItem() == _previousMatch)
			{
				_state = Searching;
				return;
			}
			else
			{
				return; // skip until previousMatch is found
			}

		case Searching:
            if (TreeModel::RowContainsString(row, _searchString, _columns, true))
            {
                _match = row.getItem();
                _state = Found;
            }
			break;

		case Found:
			return; // done here
		}
	}
};

wxDataViewItem TreeModel::FindNextString(const wxString& needle,
	const std::vector<TreeModel::Column>& columns, const wxDataViewItem& previousMatch)
{
	SearchFunctor functor(needle, columns, previousMatch);

	ForeachNode([&] (Row& row)
	{
		functor(row);
	});

	return functor.getMatch();
}

// Search for an item in the given columns (backwards), using previousMatch as reference point 
wxDataViewItem TreeModel::FindPrevString(const wxString& needle,
	const std::vector<TreeModel::Column>& columns, const wxDataViewItem& previousMatch)
{
	SearchFunctor functor(needle, columns, previousMatch);

	ForeachNodeReverse([&] (Row& row)
	{
		functor(row);
	});

	return functor.getMatch();
}

bool TreeModel::IsEnabled(const wxDataViewItem& item, unsigned int col) const
{
	Node* owningNode = item.IsOk() ? static_cast<Node*>(item.GetID()) : _rootNode.get();

	if (col < owningNode->enabledFlags.size())
	{
		return owningNode->enabledFlags[col];
	}
	
	// Column values without flags render as enabled by default
	return true;
}

void TreeModel::SetEnabled(const wxDataViewItem& item, unsigned int col, bool enabled)
{
	if (!item.IsOk())
	{
		return;
	}

	Node* owningNode = static_cast<Node*>(item.GetID());

	if (owningNode->enabledFlags.size() < col + 1)
	{
		owningNode->enabledFlags.resize(col + 1, true); // fill with true by default
	}

	owningNode->enabledFlags[col] = enabled;
}

void TreeModel::SendSubtreeRefreshEvents(wxDataViewItem& parentItem)
{
    wxDataViewItemArray children;
    GetChildren(parentItem, children);

    for (auto child : children)
    {
        ItemDeleted(parentItem, child);
        ItemAdded(parentItem, child);
    }
}

} // namespace
