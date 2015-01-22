#include "TreeView.h"

#include <wx/popupwin.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <algorithm>

namespace wxutil
{

TreeView::TreeView(wxWindow* parent, TreeModel::Ptr model, long style) :
	wxDataViewCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style),
	_searchPopup(NULL)
{
	EnableAutoColumnWidthFix();

	if (model)
	{
		AssociateModel(model.get());
	}

	Connect(wxEVT_CHAR, wxKeyEventHandler(TreeView::_onChar), NULL, this);
	Connect(EV_TREEVIEW_SEARCH_EVENT, SearchEventHandler(TreeView::_onSearch), NULL, this);
}

TreeView* TreeView::Create(wxWindow* parent, long style)
{
	return new TreeView(parent, TreeModel::Ptr(), style);
}

// Construct a TreeView using the given TreeModel, which will be associated
// with this view (refcount is automatically decreased by one).
TreeView* TreeView::CreateWithModel(wxWindow* parent, TreeModel::Ptr model, long style)
{
	return new TreeView(parent, model, style);
}

TreeView::~TreeView()
{}

bool TreeView::AssociateModel(wxDataViewModel* model)
{
    // We're changing models, so unselect everything first,
    // even if it's the same model again, the tree might have changed.
    UnselectAll();

    // Pass the call to the regular routine
    return wxDataViewCtrl::AssociateModel(model);
}

// Enable the automatic recalculation of column widths
void TreeView::EnableAutoColumnWidthFix(bool enable)
{
	if (enable)
	{
		Connect(wxEVT_DATAVIEW_ITEM_EXPANDED, wxDataViewEventHandler(TreeView::_onItemExpanded), NULL, this);
	}
	else
	{
		Disconnect(wxEVT_DATAVIEW_ITEM_EXPANDED, wxDataViewEventHandler(TreeView::_onItemExpanded), NULL, this);
	}
}

void TreeView::TriggerColumnSizeEvent(const wxDataViewItem& item)
{
    if (GetModel() == NULL) return;

    // Trigger a column size event on the first row
    wxDataViewItemArray children;
    GetModel()->GetChildren(item, children);

    std::for_each(children.begin(), children.end(), [&](wxDataViewItem& item)
    {
        GetModel()->ItemChanged(item);
    });
}

void TreeView::AddSearchColumn(const TreeModel::Column& column)
{
	// Only text columns are supported right now
	assert(column.type == TreeModel::Column::String || column.type == TreeModel::Column::IconText);

	_colsToSearch.push_back(column);
}

bool TreeView::HasActiveSearchPopup()
{
    return _searchPopup != NULL;
}

void TreeView::Rebuild()
{
    TreeModel* model = dynamic_cast<TreeModel*>(GetModel());

    if (model == nullptr) return;

    // Trigger a rebuild of the tree
    wxDataViewItemArray children;
    wxDataViewItem root = model->GetRoot();
    model->GetChildren(root, children);

    // By calling deleted and added, the internal dataview's nodes 
    // are invalidated which effectively is a rebuild of everything.
    for (auto child : children)
    {
        model->ItemDeleted(root, child);
        model->ItemAdded(root, child);
    }
}

void TreeView::_onItemExpanded(wxDataViewEvent& ev)
{
	// This should force a recalculation of the column width
	if (GetModel() != NULL)
	{
		GetModel()->ItemChanged(ev.GetItem());
	}

	ev.Skip();
}

wxDEFINE_EVENT(EV_TREEVIEW_SEARCH_EVENT, TreeView::SearchEvent);

TreeView::SearchEvent::SearchEvent(int id) :
	wxEvent(id, EV_TREEVIEW_SEARCH_EVENT)
{}

TreeView::SearchEvent::SearchEvent(const wxString& searchString, int id) :
	wxEvent(id, EV_TREEVIEW_SEARCH_EVENT),
	_searchString(searchString)
{}

// You *must* copy here the data to be transported
TreeView::SearchEvent::SearchEvent(const TreeView::SearchEvent& ev) :
	wxEvent(ev),
	_searchString(ev._searchString)
{}

// Required for sending with wxPostEvent()
wxEvent* TreeView::SearchEvent::Clone() const
{
	return new SearchEvent(*this);
}

const wxString& TreeView::SearchEvent::GetSearchString() const
{
	return _searchString;
}

// The custom popup window containing our search box
class TreeView::SearchPopupWindow :
	public wxPopupTransientWindow
{
private:
	TreeView* _owner;
	wxTextCtrl* _entry;

public:
	SearchPopupWindow(TreeView* owner) :
		wxPopupTransientWindow(owner),
		_owner(owner),
		_entry(NULL)
	{
		SetSizer(new wxBoxSizer(wxVERTICAL));

		_entry = new wxTextCtrl(this, wxID_ANY);

		GetSizer()->Add(_entry, 1, wxEXPAND | wxALL, 6);

		Layout();
		Fit();

		// Position this control in the bottom right corner
		wxPoint popupPos = owner->GetScreenPosition() + owner->GetSize() - GetSize();
		Position(popupPos, wxSize(0, 0));

		Connect(wxEVT_CHAR, wxKeyEventHandler(SearchPopupWindow::OnChar), NULL, this);
	}

	void OnChar(wxKeyEvent& ev)
	{
		HandleKey(ev);
	}

	virtual void OnDismiss()
	{
		// Send an event to the parent TreeView
		SearchEvent searchEvent("", SearchEvent::POPUP_DISMISSED);
		_owner->HandleWindowEvent(searchEvent);

		wxPopupTransientWindow::OnDismiss();
	}

	void HandleKey(wxKeyEvent& ev)
	{
		// Adapted this from the wxWidgets docs
		wxChar uc = ev.GetUnicodeKey();

		if (uc != WXK_NONE)
		{
			// It's a "normal" character. Notice that this includes
			// control characters in 1..31 range, e.g. WXK_RETURN or
			// WXK_BACK, so check for them explicitly.
			if (uc >= 32)
			{
				_entry->SetValue(_entry->GetValue() + ev.GetUnicodeKey());

				// Send an event to the parent TreeView
				SearchEvent searchEvent(_entry->GetValue(), SearchEvent::SEARCH);
				_owner->HandleWindowEvent(searchEvent);
			}
			else if (ev.GetKeyCode() == WXK_ESCAPE)
			{
				DismissAndNotify();
			}
			else if (ev.GetKeyCode() == WXK_BACK)
			{
				_entry->SetValue(_entry->GetValue().RemoveLast(1));

				// Send an event to the parent TreeView
				SearchEvent searchEvent(_entry->GetValue(), SearchEvent::SEARCH);
				_owner->HandleWindowEvent(searchEvent);
			}
		}
		else // No Unicode equivalent.
		{
			// Cursor events are special 
			if (ev.GetKeyCode() == WXK_UP || ev.GetKeyCode() == WXK_DOWN)
			{
				SearchEvent searchEvent(_entry->GetValue(),
					ev.GetKeyCode() == WXK_UP ? SearchEvent::SEARCH_PREV_MATCH : SearchEvent::SEARCH_NEXT_MATCH);
				_owner->HandleWindowEvent(searchEvent);
			}
		}
	}
};

void TreeView::_onChar(wxKeyEvent& ev)
{
	if (GetModel() == NULL || _colsToSearch.empty())
	{
		ev.Skip();
		return;
	}

	// Adapted this from the wxWidgets docs
	wxChar uc = ev.GetUnicodeKey();

	if (uc != WXK_NONE && uc >= 32)
	{
		// It's a "normal" character, start the search
		if (_searchPopup == NULL)
		{
			_searchPopup = new SearchPopupWindow(this);
			_searchPopup->Popup();
			_curSearchMatch = wxDataViewItem();
		}

		// Handle the first key immediately
		_searchPopup->HandleKey(ev);
	}

	// Don't eat the event
	ev.Skip();
}

void TreeView::_onSearch(SearchEvent& ev)
{
	if (GetModel() == NULL)
	{
		ev.Skip(); // no model attached
		return;
	}

	TreeModel* model = dynamic_cast<TreeModel*>(GetModel());

	if (model == NULL)
	{
		ev.Skip(); // not a TreeModel
		return;
	}

	wxDataViewItem oldMatch = _curSearchMatch;

	// Handle the search
	switch (ev.GetId())
	{
	case SearchEvent::SEARCH:
		_curSearchMatch = model->FindNextString(ev.GetSearchString(), _colsToSearch);
		break;

	case SearchEvent::SEARCH_NEXT_MATCH:
		_curSearchMatch = model->FindNextString(ev.GetSearchString(), _colsToSearch, _curSearchMatch);
		break;

	case SearchEvent::SEARCH_PREV_MATCH:
		_curSearchMatch = model->FindPrevString(ev.GetSearchString(), _colsToSearch, _curSearchMatch);
		break;

	case SearchEvent::POPUP_DISMISSED:
		_searchPopup = NULL;
		_curSearchMatch = wxDataViewItem();
		break;
	};

	if (oldMatch != _curSearchMatch && _curSearchMatch.IsOk())
	{
        UnselectAll();
		Select(_curSearchMatch);
		EnsureVisible(_curSearchMatch);

		// Synthesise a selection changed signal
		wxDataViewEvent le(wxEVT_DATAVIEW_SELECTION_CHANGED, GetId());

		le.SetEventObject(this);
		le.SetModel(GetModel());
		le.SetItem(_curSearchMatch);

		ProcessWindowEvent(le);
	}
}

} // namespace
