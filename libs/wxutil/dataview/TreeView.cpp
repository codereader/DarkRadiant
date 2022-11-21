#include "TreeView.h"

#include "i18n.h"

#include <wx/eventfilter.h>
#include <wx/bmpbuttn.h>
#include <wx/popupwin.h>
#include <wx/stattext.h>
#include <wx/app.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include "wxutil/Bitmap.h"
#include <wx/textctrl.h>

namespace wxutil
{

namespace
{
	const int MSECS_TO_AUTO_CLOSE_POPUP = 6000;
	const int MSECS_TO_HIGHLIGHT_MATCH = 250;
}

class TreeView::Search :
	public wxEvtHandler
{
private:
	TreeView& _treeView;
	SearchPopupWindow* _popup;
	wxDataViewItem _curSearchMatch;
	wxTimer _closeTimer;
	wxTimer _highlightSearchTimer;

public:
	Search(TreeView& treeView);

	~Search();

    SearchPopupWindow* GetPopupWindow();

	void HandleKeyEvent(wxKeyEvent& ev);

	void HighlightNextMatch();
	void HighlightPrevMatch();

	void Close();

private:
	void HighlightMatch(const wxDataViewItem& item);
	void _onIntervalReached(wxTimerEvent& ev);
	void _onTreeViewCharHook(wxKeyEvent& keyEvent);
};

TreeView::TreeView(wxWindow* parent, wxDataViewModel* model, long style) :
	wxDataViewCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style),
	_collapseRecursively(true),
    _searchPopupEnabled(true)
{
	EnableAutoColumnWidthFix();

	if (model)
	{
		AssociateModel(model);
	}

	Bind(wxEVT_CHAR, &TreeView::_onChar, this);
	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &TreeView::_onItemActivated, this);	
	Bind(wxEVT_DATAVIEW_ITEM_COLLAPSING, &TreeView::_onItemCollapsing, this);
}

TreeView* TreeView::Create(wxWindow* parent, long style)
{
	return new TreeView(parent, nullptr, style);
}

TreeView* TreeView::CreateWithModel(wxWindow* parent, wxDataViewModel* model, long style)
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

#ifdef __WXGTK__ 
	// In case EnsureVisible has been called in the past with no idle event
	// in between, the wxDataViewCtrl::m_ensureVisibleDefered member will 
	// still be pointing to the old tree model.
	// The member is set in EnsureVisible and cleared only after processing,
	// so the idle event would be running into a stale pointer crash.
	// Call EnsureVisible() with an not-OK item to clear it before switching models.  
	if (GetModel() != nullptr)
	{ 
		EnsureVisible(wxDataViewItem(nullptr));
	}
#endif

    // Pass the call to the regular routine
    return wxDataViewCtrl::AssociateModel(model);
}

// Enable the automatic recalculation of column widths
void TreeView::EnableAutoColumnWidthFix(bool enable)
{
	if (enable)
	{
		Bind(wxEVT_DATAVIEW_ITEM_EXPANDED, &TreeView::_onItemExpanded, this);
	}
	else
	{
		Unbind(wxEVT_DATAVIEW_ITEM_EXPANDED, &TreeView::_onItemExpanded, this);
	}
}

void TreeView::TriggerColumnSizeEvent(const wxDataViewItem& item)
{
    if (GetModel() == nullptr) return;

    // Trigger a column size event on the first row
    wxDataViewItemArray children;
    GetModel()->GetChildren(item, children);

	for (const auto& child : children)
    {
        GetModel()->ItemChanged(child);
    }
}

void TreeView::ExpandTopLevelItems()
{
	TreeModel* model = dynamic_cast<TreeModel*>(GetModel());

	if (model == nullptr) return;

	// Expand the first layer
	wxDataViewItemArray children;
	model->GetChildren(model->GetRoot(), children);

	for (const auto& item : children)
	{
		Expand(item);
	}
}

void TreeView::EnableRecursiveCollapse(bool enabled)
{
	_collapseRecursively = enabled;
}

void TreeView::ResetSortingOnAllColumns()
{
#if wxCHECK_VERSION(3, 1, 0) && defined(__WXMSW__ )
	ResetAllSortColumns();
#else
	// We don't have ResetAllSortColumns in wxWidgets 3.0.x
	wxDataViewColumn* col = GetSortingColumn();

	if (col != nullptr)
	{
		col->UnsetAsSortKey();
	}
#endif
}

void TreeView::AddSearchColumn(const TreeModel::Column& column)
{
	// Only text columns are supported right now
	assert(column.type == TreeModel::Column::String || column.type == TreeModel::Column::IconText);

	_colsToSearch.push_back(column);
}

bool TreeView::HasActiveSearchPopup()
{
    return _search != nullptr;
}

void TreeView::EnableSearchPopup(bool enabled)
{
    _searchPopupEnabled = enabled;

    if (!_searchPopupEnabled)
    {
        CloseSearch();
    }
}

#if !defined(__linux__)
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
#endif

void TreeView::CancelEditing()
{
    for (unsigned i = 0; i < GetColumnCount(); ++i)
    {
        auto column = GetColumn(i);

        if (column->GetRenderer()->GetEditorCtrl())
        {
            column->GetRenderer()->CancelEditing();
        }
    }
}

void TreeView::SendSelectionChangeEvent(const wxDataViewItem& item)
{
    // In wxWidgets 3.1.x the wxDataViewEvent constructors have changed, switch on it
#if wxCHECK_VERSION(3, 1, 0)
    wxDataViewEvent le(wxEVT_DATAVIEW_SELECTION_CHANGED, this, item);
#else
    wxDataViewEvent le(wxEVT_DATAVIEW_SELECTION_CHANGED, GetId());

    le.SetEventObject(this);
    le.SetModel(GetModel());
    le.SetItem(item);
#endif

    ProcessWindowEvent(le);
}

void TreeView::CollapseChildren(const wxDataViewItem& item)
{
	// Collapse all children that are currently expanded
	wxDataViewItemArray children;
	GetModel()->GetChildren(item, children);

	for (const auto& child : children)
	{
		if (IsExpanded(child))
		{
			Collapse(child);
		}
	}
}

void TreeView::_onItemCollapsing(wxDataViewEvent& ev)
{
	ev.Skip();

	if (_collapseRecursively && GetModel() != nullptr)
	{
		CollapseChildren(ev.GetItem());
		return;
	}
}

void TreeView::_onItemExpanded(wxDataViewEvent& ev)
{
	// This should force a recalculation of the column width
	if (GetModel() != nullptr)
	{
		GetModel()->ItemChanged(ev.GetItem());
	}

	ev.Skip();
}

void TreeView::_onItemActivated(wxDataViewEvent& ev)
{
	if (!IsExpanded(ev.GetItem()))
	{
		Expand(ev.GetItem());
        ev.Skip(); // let parent dialogs handle double clicks (e.g. auto close)
	}
	else
	{
		Collapse(ev.GetItem());
	}
}

// The custom popup window containing our search box
class TreeView::SearchPopupWindow :
	public wxPopupWindow,
	public wxEventFilter
{
private:
	TreeView* _treeView;
	Search& _owner;
	wxTextCtrl* _entry;

public:
	SearchPopupWindow(TreeView* treeView, Search& owner) :
		wxPopupWindow(treeView, wxBORDER_SIMPLE),
		_treeView(treeView),
		_owner(owner),
		_entry(nullptr)
	{
		SetSizer(new wxBoxSizer(wxHORIZONTAL));

		auto label = new wxStaticText(this, wxID_ANY, _("Find: "));

		_entry = new wxTextCtrl(this, wxID_ANY);

		auto nextImg = wxutil::GetLocalBitmap("arrow_down.png");
		auto nextButton = new wxBitmapButton(this, wxID_ANY, nextImg);

		auto prevImg = wxutil::GetLocalBitmap("arrow_up.png");
		auto prevButton = new wxBitmapButton(this, wxID_ANY, prevImg);
		
		nextButton->SetSize(wxSize(16, 16));
		prevButton->SetSize(wxSize(16, 16));

		nextButton->SetToolTip(_("Go to next match"));
		prevButton->SetToolTip(_("Go to previous match"));

		nextButton->Bind(wxEVT_BUTTON, [this] (wxCommandEvent& ev) { _owner.HighlightNextMatch(); });
		prevButton->Bind(wxEVT_BUTTON, [this] (wxCommandEvent& ev) { _owner.HighlightPrevMatch(); });

		GetSizer()->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
		GetSizer()->Add(_entry, 1, wxEXPAND | wxALL, 6);
		GetSizer()->Add(prevButton, 0, wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, 6);
		GetSizer()->Add(nextButton, 0, wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, 6);

		Layout();
		Fit();

		Reposition();

		// Subscribe to the parent window's visibility and iconise events to avoid 
		// the popup from lingering around long after the tree is gone (#5095)
		wxWindow* parentWindow = wxGetTopLevelParent(treeView);

		if (parentWindow != nullptr)
		{
			parentWindow->Bind(wxEVT_SHOW, &SearchPopupWindow::_onParentVisibilityChanged, this);
			parentWindow->Bind(wxEVT_ICONIZE, &SearchPopupWindow::_onParentMinimized, this);

			// Detect when the parent window is losing focus (e.g. by alt-tabbing)
			parentWindow->Bind(wxEVT_ACTIVATE, &SearchPopupWindow::_onParentActivate, this);

			// Detect parent window movements to reposition ourselves
			parentWindow->Bind(wxEVT_MOVE, &SearchPopupWindow::_onParentMoved, this);
		}

		// Register as global filter to catch mouse events
		wxEvtHandler::AddFilter(this);
	}

	virtual ~SearchPopupWindow()
	{
		wxEvtHandler::RemoveFilter(this);
	}

	int FilterEvent(wxEvent& ev) override
	{
		const wxEventType t = ev.GetEventType();

		if (t == wxEVT_LEFT_UP || t == wxEVT_RIGHT_UP)
		{
			for (wxWindow* win = wxDynamicCast(ev.GetEventObject(), wxWindow); 
				win != nullptr; win = win->GetParent())
			{
				if (win == this || win == _treeView)
				{
					// Ignore any clicks on this popup or the owning treeview
					return Event_Skip;
				}
			}

			// User clicked on a window which is not a child of this popup => close it
			// But we can't call _owner.Close() immediately since this
			// will fire the destructor and crash the event filter handler
			// Register for the next idle event to close this popup
			wxTheApp->Bind(wxEVT_IDLE, &SearchPopupWindow::_onIdleClose, this);
		}

		// Continue processing the event normally 
		return Event_Skip;
	}

	wxString GetSearchString()
	{
		return _entry->GetValue();
	}

	void SetSearchString(const wxString& str)
	{
		_entry->SetValue(str);
	}

private:
	void Reposition()
	{
		// Position this control in the bottom right corner
		wxPoint popupPos = GetParent()->GetScreenPosition() + GetParent()->GetSize() - GetSize();
		Position(popupPos, wxSize(0, 0));
	}

	void _onIdleClose(wxIdleEvent& ev)
	{
		_owner.Close();
		ev.Skip();
	}

	void _onParentActivate(wxActivateEvent& ev)
	{
		if (!ev.GetActive())
		{
			_owner.Close();
		}
	}

	void _onParentMoved(wxMoveEvent&)
	{
		Reposition();
	}

	void _onParentMinimized(wxIconizeEvent&)
	{
		// Close any searches when the parent window is minimized
		_owner.Close();
	}

	void _onParentVisibilityChanged(wxShowEvent& ev)
	{
		if (!ev.IsShown())
		{
			// Close any searches when the parent window is hidden
			_owner.Close();
		}
	}
};

TreeView::Search::Search(TreeView& treeView) :
	_treeView(treeView),
	_closeTimer(this),
    _highlightSearchTimer(this)
{
	_popup = new SearchPopupWindow(&_treeView, *this);
	_popup->Show();
	_curSearchMatch = wxDataViewItem();

	_treeView.Bind(wxEVT_CHAR_HOOK, &Search::_onTreeViewCharHook, this);
	Bind(wxEVT_TIMER, &Search::_onIntervalReached, this);

	_closeTimer.Start(MSECS_TO_AUTO_CLOSE_POPUP);
}

TreeView::Search::~Search()
{
	_closeTimer.Stop();

	_treeView.Unbind(wxEVT_CHAR_HOOK, &Search::_onTreeViewCharHook, this);
	Unbind(wxEVT_TIMER, &Search::_onIntervalReached, this);

	// Always hide popup windows before destroying them, otherwise the
	// wx-internal wxCurrentPopupWindow pointer doesn't get cleared (in MSW at least)
	_popup->Hide();
	_popup->Destroy();
	_popup = nullptr;
	_curSearchMatch = wxDataViewItem();
}

void TreeView::Search::_onTreeViewCharHook(wxKeyEvent& ev)
{
	if (ev.GetKeyCode() == WXK_ESCAPE)
	{
		// The parent dialog listens to ESC keys to close the window
		// While this search window is alive, eat the event and close the popup instead
		_treeView.CloseSearch();
		return;
	}

	ev.Skip();
}

void TreeView::Search::_onIntervalReached(wxTimerEvent& ev)
{
    if (ev.GetTimer().GetId() == _closeTimer.GetId())
    {
        // Disconnect the timing event
        _closeTimer.Stop();

        _treeView.CloseSearch();
    }
    else if (ev.GetTimer().GetId() == _highlightSearchTimer.GetId())
    {
        auto* model = dynamic_cast<TreeModel*>(_treeView.GetModel());
        if (model == nullptr) return;

        HighlightMatch(model->FindNextString(_popup->GetSearchString(), _treeView._colsToSearch));
    }
}

void TreeView::Search::HighlightMatch(const wxDataViewItem& item)
{
	_closeTimer.Start(MSECS_TO_AUTO_CLOSE_POPUP); // restart

	_curSearchMatch = item;
	_treeView.JumpToSearchMatch(_curSearchMatch);
}

TreeView::SearchPopupWindow* TreeView::Search::GetPopupWindow()
{
    return _popup;
}

void TreeView::Search::HandleKeyEvent(wxKeyEvent& ev)
{
	TreeModel* model = dynamic_cast<TreeModel*>(_treeView.GetModel());

	if (model == nullptr)
	{
		ev.Skip();
		return;
	}

	// Adapted this from the wxWidgets docs
	wxChar uc = ev.GetUnicodeKey();

	if (uc != WXK_NONE)
	{
		// It's a "normal" character. Notice that this includes
		// control characters in 1..31 range, e.g. WXK_RETURN or
		// WXK_BACK, so check for them explicitly.
		if (uc >= 32)
		{
			_popup->SetSearchString(_popup->GetSearchString() + ev.GetUnicodeKey());

            // Perform the actual search a bit later (#5745)
            _highlightSearchTimer.Start(MSECS_TO_HIGHLIGHT_MATCH, true);
		}
		else if (ev.GetKeyCode() == WXK_ESCAPE)
		{
			_treeView.CloseSearch();
		}
		else if (ev.GetKeyCode() == WXK_BACK)
		{
			_popup->SetSearchString(_popup->GetSearchString().RemoveLast(1));
            
            // Perform the actual search a bit later (#5745)
            _highlightSearchTimer.Start(MSECS_TO_HIGHLIGHT_MATCH, true);
		}
		else
		{
			ev.Skip();
		}
	}
	// No Unicode equivalent, might be an arrow key
	else if (ev.GetKeyCode() == WXK_UP)
	{
		HighlightPrevMatch();
	}
	else if (ev.GetKeyCode() == WXK_DOWN)
	{
		HighlightNextMatch();
	}
	else
	{
		ev.Skip();
	}
}

void TreeView::Search::HighlightNextMatch()
{
	TreeModel* model = dynamic_cast<TreeModel*>(_treeView.GetModel());

	if (model == nullptr)
	{
		return;
	}

	HighlightMatch(model->FindNextString(_popup->GetSearchString(), _treeView._colsToSearch, _curSearchMatch));
}

void TreeView::Search::HighlightPrevMatch()
{
	TreeModel* model = dynamic_cast<TreeModel*>(_treeView.GetModel());

	if (model == nullptr)
	{
		return;
	}

	HighlightMatch(model->FindPrevString(_popup->GetSearchString(), _treeView._colsToSearch, _curSearchMatch));
}

void TreeView::Search::Close()
{
    _highlightSearchTimer.Stop();
	_treeView.CloseSearch();
}

void TreeView::CloseSearch()
{
	_search.reset();
}

void TreeView::_onChar(wxKeyEvent& ev)
{
	if (!_searchPopupEnabled || GetModel() == nullptr || _colsToSearch.empty())
	{
		ev.Skip();
		return;
	}

	// Adapted this from the wxWidgets docs
	wxChar uc = ev.GetUnicodeKey();

	// Start a search operation on any "normal" character
	if (uc != WXK_NONE && uc >= 32 && !_search)
	{
		_search = std::make_unique<Search>(*this);
	}

	if (_search)
	{
		// Forward the key event to the search helper
		_search->HandleKeyEvent(ev);
	}
	else
	{
		// Don't eat the event
		ev.Skip();
	}
}

void TreeView::JumpToSearchMatch(const wxDataViewItem& item)
{
	TreeModel* model = dynamic_cast<TreeModel*>(GetModel());

	if (model == nullptr)
	{
		return;
	}

	if (GetSelection() != item && item.IsOk())
	{
        UnselectAll();
		Select(item);
		EnsureVisible(item);

#ifdef __WXMSW__
        // #5511: Avoid the search box blocking the found item
        auto itemRect = GetItemRect(item);

        ClientToScreen(&itemRect.x, &itemRect.y);
        if (_search && _search->GetPopupWindow() != nullptr && 
            _search->GetPopupWindow()->GetRect().Intersects(itemRect))
        {
            // Scroll down by two rows
            int itemRow = GetRowByItem(item);
            EnsureVisibleRowCol(itemRow + 2, 0);
        }
#endif

		// Synthesise a selection changed signal
        SendSelectionChangeEvent(item);
	}
}
} // namespace
