#include "SkinChooser.h"

#include "i18n.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "itextstream.h"
#include "modelskin.h"

#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/splitter.h>

#include "wxutil/TreeView.h"
#include "wxutil/VFSTreePopulator.h"

namespace ui
{

/* CONSTANTS */

namespace
{
	const char* FOLDER_ICON = "folder16.png";
	const char* SKIN_ICON = "skin16.png";

	const char* const WINDOW_TITLE = N_("Choose Skin");
}

// Constructor
SkinChooser::SkinChooser() :
	DialogBase(_(WINDOW_TITLE)),
	_treeStore(nullptr),
	_treeView(nullptr),
	_lastSkin("")
{
	FitToScreen(0.6f, 0.6f);

	populateWindow();
}

SkinChooser& SkinChooser::Instance()
{
	SkinChooserPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new SkinChooser);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &SkinChooser::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

SkinChooserPtr& SkinChooser::InstancePtr()
{
	static SkinChooserPtr _instancePtr;
	return _instancePtr;
}

void SkinChooser::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
                                                      wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

	// Create the treestore
	_treeStore = new wxutil::TreeModel(_columns);

	// Create the tree view
    _treeView = wxutil::TreeView::CreateWithModel(splitter, _treeStore, wxDV_NO_HEADER);
	_treeView->SetMinClientSize(wxSize(GetSize().GetWidth() / 5, -1));
	
	// Single column to display the skin name
	_treeView->AppendIconTextColumn(_("Skin"), _columns.displayName.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	// Connect up selection changed callback
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
		wxDataViewEventHandler(SkinChooser::_onSelChanged), NULL, this);

	// Preview
    _preview.reset(new wxutil::ModelPreview(splitter));
	_preview->getWidget()->SetMinClientSize(wxSize(GetSize().GetWidth() / 3, -1));

	// Pack treeview and preview
    splitter->SplitVertically(_treeView, _preview->getWidget());

    FitToScreen(0.6f, 0.8f);

    // Set the default size of the window
    splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.3f));

	// Overall vbox for treeview/preview and buttons
	vbox->Add(splitter, 1, wxEXPAND);
	vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxTOP, 12);
}

int SkinChooser::ShowModal()
{
	populateSkins();

	// Display the model in the window title
	SetTitle(std::string(_(WINDOW_TITLE)) + ": " + _model);

    setSelectedSkin(_prevSkin);

	int returnCode = DialogBase::ShowModal();

	if (returnCode == wxID_OK)
	{
		// Get the selected skin
		_lastSkin = getSelectedSkin();
	}
	else
	{
		// Revert to previous skin on everything other than OK
		_lastSkin = _prevSkin;
	}

	_preview->setModel(""); // release model

	return returnCode;
}

namespace
{

/*
 * Visitor class to fill in column data for the skins tree.
 */
class SkinTreeVisitor : 
	public wxutil::VFSTreePopulator::Visitor
{
private:
	const SkinChooser::TreeColumns& _columns;

	wxIcon _skinIcon;
	wxIcon _folderIcon;

public:
	SkinTreeVisitor(const SkinChooser::TreeColumns& columns) :
		_columns(columns)
	{
		_skinIcon.CopyFromBitmap(
			wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + SKIN_ICON));
		_folderIcon.CopyFromBitmap(
			wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
	}

    virtual ~SkinTreeVisitor() {}

	// Required visit function
	void visit(wxutil::TreeModel& /* store */, wxutil::TreeModel::Row& row,
			   const std::string& path, bool isExplicit)
	{
		// Get the display path, everything after rightmost slash
		std::string displayPath = path.substr(path.rfind("/") + 1);

		row[_columns.displayName] = wxVariant(wxDataViewIconText(displayPath,
			isExplicit ? _skinIcon : _folderIcon));
		row[_columns.fullName] = path;

		row.SendItemAdded();
	}
};

} // namespace

// Populate the list of skins
void SkinChooser::populateSkins()
{
	// Clear the treestore
	_treeStore->Clear();

	wxIcon folderIcon;
	folderIcon.CopyFromBitmap(
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));

	wxIcon skinIcon;
	skinIcon.CopyFromBitmap(
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + SKIN_ICON));

	// Add the "Matching skins" toplevel node
	wxutil::TreeModel::Row matchingSkins = _treeStore->AddItem();

	matchingSkins[_columns.displayName] = wxVariant(wxDataViewIconText(_("Matching skins"), folderIcon));
	matchingSkins[_columns.fullName] = "";

	matchingSkins.SendItemAdded();

	// Get the skins for the associated model, and add them as matching skins
	const StringList& matchList = GlobalModelSkinCache().getSkinsForModel(_model);

	for (StringList::const_iterator i = matchList.begin();
		 i != matchList.end();
		 ++i)
	{
		wxutil::TreeModel::Row skinRow = _treeStore->AddItem(matchingSkins.getItem());

		skinRow[_columns.displayName] = wxVariant(wxDataViewIconText(*i, skinIcon));
		skinRow[_columns.fullName] = *i;

		skinRow.SendItemAdded();
	}

	// Add "All skins" toplevel node
	wxutil::TreeModel::Row allSkins = _treeStore->AddItem();

	allSkins[_columns.displayName] = wxVariant(wxDataViewIconText(_("All skins"), folderIcon));
	allSkins[_columns.fullName] = "";

	allSkins.SendItemAdded();

	// Get the list of skins for the model
	const StringList& skins = GlobalModelSkinCache().getAllSkins();

	// Create a TreePopulator for the tree store and pass in each of the
	// skin names.
	wxutil::VFSTreePopulator pop(_treeStore, allSkins.getItem());

	for (StringList::const_iterator i = skins.begin();
		 i != skins.end();
		 ++i)
	{
		pop.addPath(*i);
	}

	// Visit the tree populator in order to fill in the column data
	SkinTreeVisitor visitor(_columns);
	pop.forEachNode(visitor);
}

std::string SkinChooser::getSelectedSkin()
{
	// Get the selected skin
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_treeStore);
		return row[_columns.fullName];
	}
	else
	{
		return "";
	}
}

void SkinChooser::setSelectedSkin(const std::string& skin)
{
    wxDataViewItem item = _treeStore->FindString(skin, _columns.fullName);

    if (item.IsOk())
    {
        _treeView->Select(item);
    }
    else
    {
        _treeView->UnselectAll();
    }

    handleSelectionChange();
}

// Static method to display singleton instance and choose a skin
std::string SkinChooser::chooseSkin(const std::string& model,
									const std::string& prev)
{
	Instance()._model = model;
	Instance()._prevSkin = prev;

	Instance().ShowModal();
	Instance().Hide();
	
	return Instance()._lastSkin;
}

void SkinChooser::onRadiantShutdown()
{
	rMessage() << "SkinChooser shutting down." << std::endl;

	_preview.reset();

	// Destroy the window (after it has been disconnected from the Eventmanager)
	SendDestroyEvent();
	InstancePtr().reset();
}

void SkinChooser::handleSelectionChange()
{
    // Set the model preview to show the model with the selected skin
    _preview->setModel(_model);
    _preview->setSkin(getSelectedSkin());
}

void SkinChooser::_onSelChanged(wxDataViewEvent& ev)
{
    handleSelectionChange();
}

} // namespace
