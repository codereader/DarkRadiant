#pragma once

#include <wx/dataview.h>

namespace wxutil
{

// Extends the default tree store by more efficient comparison
class TreeModel :
	public wxDataViewModel
{
public:
	class Column
	{
	public:
		enum Type
		{
			String = 0,
			Integer,
			Double,
			Bool,
			Icon,
			NumTypes
		};

		Type type;
		std::string name;

		Column(Type type_, const std::string& name_ = "") :
			type(type_),
			name(name_)
		{}

		wxString getType() const
		{
			static std::vector<wxString> types(NumTypes);

			if (types.empty())
			{
				types[String] = "string";
				types[Integer] = "long";
				types[Double] = "double";
				types[Bool] = "bool";
				types[Icon] = "icon";
			}

			return types[type];
		}
	};
	typedef std::vector<Column> ColumnRecord;

	// Treemodel node
	struct Node
	{
		Node* parent; // NULL for the root node

		// The item will use a Node* pointer as ID, which is possible
		// since the wxDataViewItem is using void* as ID type
		wxDataViewItem item;

		typedef std::vector<wxVariant> Values;
		Values values;

		typedef std::vector<Node> Children;
		Children children;

		Node(Node* parent_ = NULL) :
			parent(parent_),
			item(reinterpret_cast<wxDataViewItem::Type>(this))
		{}

		Node(const Node& other) :
			parent(other.parent),
			item(reinterpret_cast<wxDataViewItem::Type>(this)),
			values(other.values),
			children(other.children)
		{}
	};
private:
	ColumnRecord _columns;

	Node _rootNode;

	int _sortColumn;
public:
	TreeModel() :
		_rootNode(NULL),
		_sortColumn(-1)
	{}

	TreeModel(const ColumnRecord& columns) :
		_columns(columns),
		_rootNode(NULL),
		_sortColumn(-1)
	{
		// Use the first text-column for default sort 
		for (std::size_t i = 0; i < _columns.size(); ++i)
		{
			if (_columns[i].type == Column::String)
			{
				_sortColumn = i;
				break;
			}
		}
	}

	virtual ~TreeModel()
	{}

	virtual wxDataViewItem AddItem(wxDataViewItem& parent)
	{
		if (parent.GetID() != NULL)
		{
			Node* parentNode = static_cast<Node*>(parent.GetID());

			parentNode->children.push_back(Node(parentNode));

			return parentNode->children.back().item;
		}
		else
		{
			return wxDataViewItem();
		}
	}

	virtual bool HasDefaultCompare() const
	{
		return true;
	}

	virtual unsigned int GetColumnCount() const
	{
		return static_cast<unsigned int>(_columns.size());
	};

    // return type as reported by wxVariant
    virtual wxString GetColumnType(unsigned int col) const
	{
		return _columns[col].getType();
	}

    // get value into a wxVariant
    virtual void GetValue( wxVariant &variant,
                           const wxDataViewItem &item, unsigned int col ) const
	{
		Node* owningNode = static_cast<Node*>(item.GetID());

		if (col < owningNode->values.size())
		{
			variant = owningNode->values[col];
		}
	}

	virtual bool SetValue(const wxVariant &variant,
                          const wxDataViewItem &item,
                          unsigned int col)
	{
		Node* owningNode = static_cast<Node*>(item.GetID());

		owningNode->values.resize(col + 1);
		owningNode->values[col] = variant;

		return true;
	}

	virtual wxDataViewItem GetParent( const wxDataViewItem &item ) const
	{
		// It's ok to ask for invisible root node's parent
		if (!item.IsOk())
		{
			return wxDataViewItem(NULL);
		}

		Node* owningNode = static_cast<Node*>(item.GetID());

		return owningNode->parent != NULL ? owningNode->parent->item : wxDataViewItem(NULL);
	}

    virtual bool IsContainer(const wxDataViewItem& item) const
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

	virtual unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
	{
		Node* owningNode = static_cast<Node*>(item.GetID());

		// Requests for invalid items are asking for our root node, actually
		if (!owningNode)
		{
			children.Add(_rootNode.item);
			return 1;
		} 

		for (Node::Children::const_iterator iter = owningNode->children.begin(); iter != owningNode->children.end(); ++iter)
		{
			children.Add(iter->item);
		}

		return owningNode->children.size();
	}

	virtual wxDataViewItem GetRoot()
	{
		return _rootNode.item;
	}

	virtual int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending) const
	{
		Node* node1 = static_cast<Node*>(item1.GetID());
		Node* node2 = static_cast<Node*>(item2.GetID());

		if (!node1 || !node2)
			return 0;

		if (!node1->children.empty() && node2->children.empty())
			return -1;

		if (!node2->children.empty() && node1->children.empty())
			return 1;

		if (_sortColumn > 0)
		{
			return node1->values[_sortColumn].GetString().CompareTo(node2->values[_sortColumn].GetString());
		}

		return 0;
	}
};


} // namespace
