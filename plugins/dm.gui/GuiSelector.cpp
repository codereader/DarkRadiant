#include "GuiSelector.h"

#include "wxutil/dataview/VFSTreePopulator.h"

#include "i18n.h"
#include "ui/imainframe.h"
#include "gui/GuiManager.h"
#include "wxutil/dialog/MessageBox.h"

#include "ReadablePopulator.h"
#include "ReadableEditorDialog.h"

#include "wxutil/Bitmap.h"
#include <wx/notebook.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Choose a Gui Definition...");

	const int WINDOW_WIDTH = 400;
	const int WINDOW_HEIGHT = 500;

	const char* const GUI_ICON = "sr_icon_readable.png";
	const char* const FOLDER_ICON = "folder16.png";
}

GuiSelector::GuiSelector(bool twoSided, ReadableEditorDialog* editorDialog) :
	DialogBase(_(WINDOW_TITLE), editorDialog),
	_editorDialog(editorDialog),
	_notebook(NULL),
	_oneSidedStore(new wxutil::TreeModel(_columns)),
	_twoSidedStore(new wxutil::TreeModel(_columns)),
	_oneSidedView(NULL),
	_twoSidedView(NULL),
	_guiIcon(wxutil::GetLocalBitmap(GUI_ICON)),
	_folderIcon(wxutil::GetLocalBitmap(FOLDER_ICON))
{
	// Set the windowsize and default border width in accordance to the HIG
	SetSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	populateWindow();

	// Set the current page and connect the switch-page signal afterwards.
	_notebook->SetSelection(twoSided ? 1 : 0);
	_notebook->Connect(wxEVT_NOTEBOOK_PAGE_CHANGED, 
		wxBookCtrlEventHandler(GuiSelector::onPageSwitch), NULL, this);

	// We start with an empty selection, so de-sensitise the OK button
	FindWindowById(wxID_OK, this)->Enable(false);
}

bool GuiSelector::Destroy()
{
	// Prevent the page switch event from firing after window destruction
	// In wxGTK the window might not be destroyed right away (only later in an idle event)
	// which will trigger page switch events and inadvertently change our GUI selection.
	_notebook->Disconnect(wxEVT_NOTEBOOK_PAGE_CHANGED,
		wxBookCtrlEventHandler(GuiSelector::onPageSwitch), NULL, this);

	return wxutil::DialogBase::Destroy();
}

std::string GuiSelector::Run(bool twoSided, ReadableEditorDialog* editorDialog)
{
	GuiSelector* dialog = new GuiSelector(twoSided, editorDialog);

	std::string rv = "";

	try
	{
		dialog->fillTrees(); // may throw OperationAbortedException
		
		if (dialog->ShowModal() == wxID_OK)
		{
			rv = "guis/" + dialog->_name;
		}
	}
	catch (wxutil::ModalProgressDialog::OperationAbortedException&)
	{
		rv = "";
	}

	dialog->Destroy();

	return rv;
}

void GuiSelector::visit(wxutil::TreeModel& /* store */, wxutil::TreeModel::Row& row,
			   const std::string& path, bool isExplicit)
{
	// Get the display name by stripping off everything before the last
	// slash
	std::string displayName = path.substr(path.rfind("/") + 1);
	displayName = displayName.substr(0,displayName.rfind("."));

	// Fill in the column values
	row[_columns.name] = wxVariant(wxDataViewIconText(displayName, isExplicit ? _guiIcon : _folderIcon));
	row[_columns.fullName] = path;
	row[_columns.isFolder] = !isExplicit;

	row.SendItemAdded();
}

void GuiSelector::fillTrees()
{
	wxutil::VFSTreePopulator popOne(_oneSidedStore);
	wxutil::VFSTreePopulator popTwo(_twoSidedStore);

	ReadablePopulator walker(popOne, popTwo);
	GlobalGuiManager().foreachGui(walker);

	popOne.forEachNode(*this);
	popTwo.forEachNode(*this);

	_oneSidedStore->SortModelFoldersFirst(_columns.name, _columns.isFolder);
	_twoSidedStore->SortModelFoldersFirst(_columns.name, _columns.isFolder);

	_oneSidedView->AssociateModel(_oneSidedStore.get());
	_twoSidedView->AssociateModel(_twoSidedStore.get());
}

void GuiSelector::populateWindow()
{
	// Add a vbox for the dialog elements
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Add a vbox for the dialog elements
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

	// Create the tabs
	_notebook = new wxNotebook(this, wxID_ANY);

	// One-Sided Readables Tab
	auto tempOneSidedModel = wxutil::TreeModel::Ptr(new wxutil::TreeModel(_columns));
	_oneSidedView = wxutil::TreeView::CreateWithModel(_notebook, tempOneSidedModel.get(), wxDV_NO_HEADER);
	_oneSidedView->AppendIconTextColumn(_("Gui Path"), _columns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
	_oneSidedView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(GuiSelector::onSelectionChanged), NULL, this);

	_notebook->AddPage(_oneSidedView, _("One-Sided Readable Guis"));

	// Two-Sided Readables Tab
	auto tempTwoSidedModel = wxutil::TreeModel::Ptr(new wxutil::TreeModel(_columns));
	_twoSidedView = wxutil::TreeView::CreateWithModel(_notebook, tempTwoSidedModel.get(), wxDV_NO_HEADER);
	_twoSidedView->AppendIconTextColumn(_("Gui Path"), _columns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
	_twoSidedView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(GuiSelector::onSelectionChanged), NULL, this);

	_notebook->AddPage(_twoSidedView, _("Two-Sided Readable Guis"));

	// Packing
	vbox->Add(_notebook, 1, wxEXPAND | wxBOTTOM, 6);
	vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT);
}

void GuiSelector::onPageSwitch(wxBookCtrlEvent& ev)
{
	if (ev.GetSelection() == 0)
	{
		_editorDialog->useOneSidedEditing();
	}
	else
	{
		_editorDialog->useTwoSidedEditing();
	}
}

void GuiSelector::onSelectionChanged(wxDataViewEvent& ev)
{
	wxutil::TreeView* view = dynamic_cast<wxutil::TreeView*>(ev.GetEventObject());

	assert(view != NULL);

	wxDataViewItem item = view->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *view->GetModel());

		if (!row[_columns.isFolder].getBool())
		{
			_name = row[_columns.fullName];
			std::string guiPath = "guis/" + _name;

			_editorDialog->updateGuiView(this, guiPath);
			FindWindowById(wxID_OK, this)->Enable(true);
			return;
		}
	}
	
	FindWindowById(wxID_OK, this)->Enable(false);
}

} // namespace
