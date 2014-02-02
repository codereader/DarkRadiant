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
			String,
			Integer,
			Double,
			Bool,
			Icon,
		};

		Type type;
		std::string name;

		Column(Type type_, const std::string& name_ = "") :
			type(type_),
			name(name_)
		{}

		wxString getType() const
		{
			static std::vector<wxString> types;

			if (types.empty())
			{
				types.push_back("string");
				types.push_back("long");
				types.push_back("double");
				types.push_back("bool");
				types.push_back("icon");
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
public:
	TreeModel() :
		_rootNode(NULL)
	{}

	TreeModel(const ColumnRecord& columns) :
		_columns(columns),
		_rootNode(NULL)
	{}

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

		if (col >= owningNode->values.size())
		{
			return false;
		}

		owningNode->values[col] = variant;
		return true;
	}

	virtual wxDataViewItem GetParent( const wxDataViewItem &item ) const
	{
		Node* owningNode = static_cast<Node*>(item.GetID());

		return owningNode->parent->item;
	}

    virtual bool IsContainer( const wxDataViewItem &item ) const
	{
		Node* owningNode = static_cast<Node*>(item.GetID());

		return owningNode != NULL && !owningNode->children.empty();
	}

	virtual unsigned int GetChildren( const wxDataViewItem &item, wxDataViewItemArray &children ) const
	{
		Node* owningNode = static_cast<Node*>(item.GetID());

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
		/*wxDataViewTreeStoreNode* node1 = FindNode(item1);
		wxDataViewTreeStoreNode* node2 = FindNode(item2);

		if (!node1 || !node2)
			return 0;

		wxDataViewTreeStoreContainerNode* parent1 =
			(wxDataViewTreeStoreContainerNode*) node1->GetParent();
		wxDataViewTreeStoreContainerNode* parent2 =
			(wxDataViewTreeStoreContainerNode*) node2->GetParent();

		if (parent1 != parent2)
		{
			wxLogError( wxT("Comparing items with different parent.") );
			return 0;
		}

		if (node1->IsContainer() && !node2->IsContainer())
			return -1;

		if (node2->IsContainer() && !node1->IsContainer())
			return 1;*/

		//return node1->GetText().CompareTo(node2->GetText());
		return 0;
	}
};


} // namespace
