#pragma once

#include <wx/dataview.h>

namespace wxutil
{

// Extends the default tree store by more efficient comparison
class TreeStore :
	public wxDataViewModel
{
public:
	TreeStore()
	{}

	virtual unsigned int GetColumnCount() const
	{
		return 0;
	};

    // return type as reported by wxVariant
    virtual wxString GetColumnType( unsigned int col ) const
	{
		return wxString("");
	}

    // get value into a wxVariant
    virtual void GetValue( wxVariant &variant,
                           const wxDataViewItem &item, unsigned int col ) const
	{

	}

	virtual bool SetValue(const wxVariant &variant,
                          const wxDataViewItem &item,
                          unsigned int col)
	{
		return false;
	}

	virtual wxDataViewItem GetParent( const wxDataViewItem &item ) const
	{
		return wxDataViewItem();
	}

    virtual bool IsContainer( const wxDataViewItem &item ) const
	{
		return false;
	}

	virtual unsigned int GetChildren( const wxDataViewItem &item, wxDataViewItemArray &children ) const
	{
		return 0;
	}

	virtual wxDataViewItem GetRoot()
	{
		return wxDataViewItem();
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
