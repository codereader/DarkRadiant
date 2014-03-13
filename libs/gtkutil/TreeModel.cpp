#include "TreeModel.h"

namespace wxutil
{

wxString TreeModel::Column::getWxType() const
{
	static std::vector<wxString> types(NumTypes);

	if (types.empty())
	{
		types[String] = "string";
		types[Integer] = "long";
		types[Double] = "double";
		types[Bool] = "bool";
		types[Icon] = "icon";
		types[IconText] = "wxDataViewIconText";
	}

	return types[type];
}

// TreeModel nodes form a directed acyclic graph
class TreeModel::Node :
	public boost::noncopyable
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

	// Public constructor, does not accept NULL pointers
	Node(Node* parent_) :
		parent(parent_),
		item(reinterpret_cast<wxDataViewItem::Type>(this))
	{
		// Use CreateRoot() instead of passing NULL
		assert(parent_ != NULL);
	}

private:
	// Private constructor creates a root node, has a wxDataViewItem ID == NULL
	Node() :
		parent(NULL),
		item(NULL)
	{}

public:
	static NodePtr createRoot()
	{
		return NodePtr(new Node);
	}

	bool remove(TreeModel::Node* child)
	{
		for (Children::const_iterator i = children.begin();
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

TreeModel::TreeModel(const ColumnRecord& columns) :
	_columns(columns),
	_rootNode(Node::createRoot()),
	_defaultStringSortColumn(-1),
	_hasDefaultCompare(true)
{
	// Use the first text-column for default sort 
	for (std::size_t i = 0; i < _columns.size(); ++i)
	{
		if (_columns[i].type == Column::String)
		{
			_defaultStringSortColumn = static_cast<int>(i);
			break;
		}
	}
}

TreeModel::~TreeModel()
{}

TreeModel::Row TreeModel::AddItem()
{
	return AddItem(_rootNode->item);
}

TreeModel::Row TreeModel::AddItem(wxDataViewItem& parent)
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
		// Remove these items
		std::for_each(itemsToDelete.begin(), itemsToDelete.end(), [&] (const wxDataViewItem& item)
		{
			Node* nodeToDelete = static_cast<Node*>(item.GetID());
			parentNode->remove(nodeToDelete);
			deleteCount++;
		});

		ItemsDeleted(parent, itemsToDelete);
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

void TreeModel::SortModel(const TreeModel::SortFunction& sortFunction)
{
	SortModelRecursive(_rootNode, sortFunction);
}

void TreeModel::SortModelRecursive(const TreeModel::NodePtr& node, const TreeModel::SortFunction& sortFunction)
{
	// Use std::sort algorithm and small lambda to only pass wxDataViewItems to the client sort function
	std::sort(node->children.begin(), node->children.end(), [&] (const NodePtr& a, const NodePtr& b)->bool
	{
		return sortFunction(a->item, b->item);
	});

	// Enter recursion
	std::for_each(node->children.begin(), node->children.end(), [&] (const NodePtr& child)
	{
		SortModelRecursive(child, sortFunction);
	});
}

wxDataViewItem TreeModel::FindString(const std::string& needle, int column)
{
	return FindRecursive(_rootNode, [&] (const Node& node)->bool
	{
		return node.values.size() > column && static_cast<std::string>(node.values[column]) == needle;
	});
}

wxDataViewItem TreeModel::FindInteger(long needle, int column)
{
	return FindRecursive(_rootNode, [&] (const Node& node)->bool
	{
		return node.values.size() > column && static_cast<long>(node.values[column]) == needle;
	});
}

wxDataViewItem TreeModel::FindRecursive(const TreeModel::NodePtr& node, const std::function<bool (const TreeModel::Node&)>& predicate)
{
	// Test the node itself
	if (predicate(*node))
	{
		return node->item;
	}

	// Then test all children, aborting on first success
	for (Node::Children::const_iterator i = node->children.begin(); i != node->children.end(); ++i)
	{
		wxDataViewItem item = FindRecursive(*i, predicate);

		if (item.IsOk())
		{
			return item;
		}
	}

	// Return an empty data item, which is "not ok"
	return wxDataViewItem();
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

	owningNode->values.resize(col + 1);
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

	owningNode->attributes.resize(col + 1);
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

	Node* owningNode = static_cast<Node*>(item.GetID());

	return owningNode != NULL && !owningNode->children.empty();
}

unsigned int TreeModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
{
	// Requests for invalid items are asking for our root children, actually
	Node* owningNode = !item.IsOk() ? _rootNode.get() : static_cast<Node*>(item.GetID());

	for (Node::Children::const_iterator iter = owningNode->children.begin(); iter != owningNode->children.end(); ++iter)
	{
		children.Add((*iter)->item);
	}

	return owningNode->children.size();
}

wxDataViewItem TreeModel::GetRoot()
{
	// The root item carries the NULL pointer, the other methods need to be able to deal with that
	return _rootNode->item;
}

int TreeModel::Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending) const
{
	Node* node1 = static_cast<Node*>(item1.GetID());
	Node* node2 = static_cast<Node*>(item2.GetID());

	if (!node1 || !node2)
		return 0;

	if (!node1->children.empty() && node2->children.empty())
		return -1;

	if (!node2->children.empty() && node1->children.empty())
		return 1;

	if (_defaultStringSortColumn >= 0)
	{
		return node1->values[_defaultStringSortColumn].GetString().CompareTo(
			node2->values[_defaultStringSortColumn].GetString(), wxString::ignoreCase);
	}

	return 0;
}

} // namespace

#include <gtkmm/treeview.h>
#include <boost/algorithm/string/find.hpp>

namespace gtkutil
{

bool TreeModel::equalFuncStringContains(const Glib::RefPtr<Gtk::TreeModel>& model,
										int column,
										const Glib::ustring& key,
										const Gtk::TreeModel::iterator& iter)
{
	// Retrieve the string from the model
	std::string str;
	iter->get_value(column, str);

	// Use a case-insensitive search
	boost::iterator_range<std::string::iterator> range = boost::algorithm::ifind_first(str, key);

	// Returning FALSE means "match".
	return (!range.empty()) ? false : true;
}

TreeModel::SelectionFinder::SelectionFinder(const std::string& selection, int column) :
	_selection(selection),
	_needle(0),
	_column(column),
	_searchForInt(false)
{}

TreeModel::SelectionFinder::SelectionFinder(int needle, int column) :
	_selection(""),
	_needle(needle),
	_column(column),
	_searchForInt(true)
{}

const Gtk::TreeModel::iterator TreeModel::SelectionFinder::getIter() const
{
	return _foundIter;
}

bool TreeModel::SelectionFinder::forEach(const Gtk::TreeModel::iterator& iter)
{
	// If the visited row matches the string/int to find
	if (_searchForInt)
	{
		int value;
		iter->get_value(_column, value);

		if (value == _needle)
		{
			_foundIter = iter;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// Search for string
		Glib::ustring value;
		iter->get_value(_column, value);

		if (value == _selection)
		{
			_foundIter = iter;
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool TreeModel::findAndSelectString(Gtk::TreeView* view, const std::string& needle, int column)
{
	SelectionFinder finder(needle, column);

	view->get_model()->foreach_iter(sigc::mem_fun(finder, &SelectionFinder::forEach));

	if (finder.getIter())
	{
		Gtk::TreeModel::Path path(finder.getIter());

		// Expand the treeview to display the target row
		view->expand_to_path(path);
		// Highlight the target row
		view->set_cursor(path);
		// Make the selected row visible
		view->scroll_to_row(path, 0.3f);

		return true; // found
	}

	return false; // not found
}

bool TreeModel::findAndSelectInteger(Gtk::TreeView* view, int needle, int column)
{
	SelectionFinder finder(needle, column);

	view->get_model()->foreach_iter(sigc::mem_fun(finder, &SelectionFinder::forEach));

	if (finder.getIter())
	{
		Gtk::TreeModel::Path path(finder.getIter());

		// Expand the treeview to display the target row
		view->expand_to_path(path);
		// Highlight the target row
		view->set_cursor(path);
		// Make the selected row visible
		view->scroll_to_row(path, 0.3f);

		return true; // found
	}

	return false; // not found
}

bool TreeModel::findAndSelectString(Gtk::TreeView* view, const std::string& needle,
									const Gtk::TreeModelColumn<Glib::ustring>& column)
{
	return findAndSelectString(view, needle, column.index());
}

bool TreeModel::findAndSelectString(Gtk::TreeView* view, const std::string& needle,
									const Gtk::TreeModelColumn<std::string>& column)
{
	return findAndSelectString(view, needle, column.index());
}

bool TreeModel::findAndSelectInteger(Gtk::TreeView* view, int needle,
									const Gtk::TreeModelColumn<int>& column)
{
	return findAndSelectInteger(view, needle, column.index());
}

int TreeModel::sortFuncFoldersFirstStd(const Gtk::TreeModel::iterator& a,
									const Gtk::TreeModel::iterator& b,
									const Gtk::TreeModelColumn<std::string>& nameColumn,
									const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	Gtk::TreeModel::Row rowA = *a;
	Gtk::TreeModel::Row rowB = *b;

	// Check if A or B are folders
	bool aIsFolder = rowA[isFolderColumn];
	bool bIsFolder = rowB[isFolderColumn];

	if (aIsFolder)
	{
		// A is a folder, check if B is as well
		if (bIsFolder)
		{
			// A and B are both folders, compare names
			// greebo: We're not checking for equality here, names should be unique
			return std::string(rowA[nameColumn]) < std::string(rowB[nameColumn]) ? -1 : 1;
		}
		else
		{
			// A is a folder, B is not, A sorts before
			return -1;
		}
	}
	else
	{
		// A is not a folder, check if B is one
		if (bIsFolder)
		{
			// A is not a folder, B is, so B sorts before A
			return 1;
		}
		else
		{
			// Neither A nor B are folders, compare names
			// greebo: We're not checking for equality here, names should be unique
			return std::string(rowA[nameColumn]) < std::string(rowB[nameColumn]) ? -1 : 1;
		}
	}
}

void TreeModel::applyFoldersFirstSortFunc(const Glib::RefPtr<Gtk::TreeSortable>& model,
										  const Gtk::TreeModelColumn<std::string>& nameColumn,
										  const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	// Set the sort column ID to nameCol
	model->set_sort_column(nameColumn, Gtk::SORT_ASCENDING);

	// Then apply the custom sort function, bind the folder column to the functor
	model->set_sort_func(nameColumn, sigc::bind(sigc::ptr_fun(sortFuncFoldersFirstStd), nameColumn, isFolderColumn));
}

int TreeModel::sortFuncFoldersFirst(const Gtk::TreeModel::iterator& a,
									  const Gtk::TreeModel::iterator& b,
									  const Gtk::TreeModelColumn<Glib::ustring>& nameColumn,
									  const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	Gtk::TreeModel::Row rowA = *a;
	Gtk::TreeModel::Row rowB = *b;

	// Check if A or B are folders
	bool aIsFolder = rowA[isFolderColumn];
	bool bIsFolder = rowB[isFolderColumn];

	if (aIsFolder)
	{
		// A is a folder, check if B is as well
		if (bIsFolder)
		{
			// A and B are both folders, compare names
			// greebo: We're not checking for equality here, names should be unique
			return (Glib::ustring(rowA[nameColumn]) < Glib::ustring(rowB[nameColumn])) ? -1 : 1;
		}
		else
		{
			// A is a folder, B is not, A sorts before
			return -1;
		}
	}
	else
	{
		// A is not a folder, check if B is one
		if (bIsFolder)
		{
			// A is not a folder, B is, so B sorts before A
			return 1;
		}
		else
		{
			// Neither A nor B are folders, compare names
			// greebo: We're not checking for equality here, names should be unique
			return (Glib::ustring(rowA[nameColumn]) < Glib::ustring(rowB[nameColumn])) ? -1 : 1;
		}
	}
}

void TreeModel::applyFoldersFirstSortFunc(const Glib::RefPtr<Gtk::TreeSortable>& model,
										  const Gtk::TreeModelColumn<Glib::ustring>& nameColumn,
										  const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	// Set the sort column ID to nameCol
	model->set_sort_column(nameColumn, Gtk::SORT_ASCENDING);

	// Then apply the custom sort function, bind the folder column to the functor
	model->set_sort_func(nameColumn, sigc::bind(sigc::ptr_fun(sortFuncFoldersFirst), nameColumn, isFolderColumn));
}

} // namespace gtkutil
