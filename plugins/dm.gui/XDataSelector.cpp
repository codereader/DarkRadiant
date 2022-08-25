#include "XDataSelector.h"

#include "i18n.h"
#include "ui/imainframe.h"

#include "ReadableEditorDialog.h"

#include "wxutil/Bitmap.h"
#include <wx/sizer.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Choose an XData Definition...");
	const int WINDOW_WIDTH = 500;
	const int WINDOW_HEIGHT = 600;

	const char* const XDATA_ICON = "sr_icon_readable.png";
	const char* const FOLDER_ICON = "folder16.png";
}

XDataSelector::XDataSelector(const XData::StringVectorMap& files, ReadableEditorDialog* editorDialog) :
	DialogBase(_(WINDOW_TITLE), editorDialog),
	_store(new wxutil::TreeModel(_columns)),
	_files(files),
	_editorDialog(editorDialog),
	_xdataIcon(wxutil::GetLocalBitmap(XDATA_ICON)),
	_folderIcon(wxutil::GetLocalBitmap(FOLDER_ICON))
{
	fillTree();

	SetSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	// Add a vbox for the dialog elements
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Add a vbox for the dialog elements
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

	// Create the treeview
	_view = wxutil::TreeView::CreateWithModel(this, _store.get(), wxDV_NO_HEADER);

	_view->AppendIconTextColumn(_("Xdata Path"), _columns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_view->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(XDataSelector::onSelectionChanged), NULL, this);

	// Use the TreeModel's full string search function
	_view->AddSearchColumn(_columns.name);

	vbox->Add(_view, 1, wxEXPAND | wxBOTTOM, 6);
	vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT);

	FindWindowById(wxID_OK, this)->Enable(false);

	CenterOnParent();
}

std::string XDataSelector::run(const XData::StringVectorMap& files, ReadableEditorDialog* editorDialog)
{
	XDataSelector* dialog = new XDataSelector(files, editorDialog);
	
	std::string rv = "";

	if (dialog->ShowModal() == wxID_OK)
	{
		rv = dialog->_selection;
	}

	dialog->Destroy();

	return rv;
}

void XDataSelector::visit(wxutil::TreeModel& /* store */, wxutil::TreeModel::Row& row,
			   const std::string& path, bool isExplicit)
{
	row[_columns.name] = wxVariant(wxDataViewIconText(path.substr(path.rfind("/") + 1),
		isExplicit ? _xdataIcon : _folderIcon));
	row[_columns.fullName] = path;
	row[_columns.isFolder] = !isExplicit;

	row.SendItemAdded();
}

void XDataSelector::fillTree()
{
	// Start adding to tree.
	wxutil::VFSTreePopulator populator(_store);

	for (XData::StringVectorMap::const_iterator it = _files.begin(); it != _files.end(); ++it)
	{
		populator.addPath(it->first);
	}

	populator.forEachNode(*this);

	_store->SortModelFoldersFirst(_columns.name, _columns.isFolder);
}

void XDataSelector::onSelectionChanged(wxDataViewEvent& ev)
{
	wxDataViewItem item = _view->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_store);

		if (!row[_columns.isFolder].getBool())
		{
			_selection = row[_columns.fullName];
			_editorDialog->updateGuiView(this, "", _selection);

			FindWindowById(wxID_OK, this)->Enable(true);
			return;
		}
	}

	FindWindowById(wxID_OK, this)->Enable(false);
}

} // namespace ui
