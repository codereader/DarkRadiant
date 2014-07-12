#include "SoundChooser.h"

#include "i18n.h"
#include "iuimanager.h"
#include "isound.h"
#include "imainframe.h"

#include "wxutil/VFSTreePopulator.h"

#include <wx/sizer.h>
#include <wx/artprov.h>

#include <boost/bind.hpp>

namespace ui
{

namespace
{
	const char* const SHADER_ICON = "icon_sound.png";
	const char* const FOLDER_ICON = "folder16.png";
}

// Constructor
SoundChooser::SoundChooser() :
	DialogBase(_("Choose sound")),
	_treeStore(new wxutil::TreeModel(_columns)),
	_treeView(NULL),
	_preview(new SoundShaderPreview(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	
	GetSizer()->Add(createTreeView(this), 1, wxEXPAND | wxALL, 12);
    GetSizer()->Add(_preview, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxLEFT | wxRIGHT, 12);

	FitToScreen(0.5f, 0.5f);
}

namespace
{

/**
 * Visitor class to enumerate sound shaders and add them to the tree store.
 */
class SoundShaderPopulator :
	public wxutil::VFSTreePopulator,
	public wxutil::VFSTreePopulator::Visitor
{
private:
	const SoundChooser::TreeColumns& _columns;

	wxIcon _shaderIcon;
	wxIcon _folderIcon;
public:
	// Constructor
	SoundShaderPopulator(wxutil::TreeModel* treeStore,
						 const SoundChooser::TreeColumns& columns) :
		VFSTreePopulator(treeStore),
		_columns(columns)
	{
		_shaderIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + SHADER_ICON));
		_folderIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
	}

    // Invoked for each sound shader
	void addShader(const ISoundShader& shader)
	{
		// Construct a "path" into the sound shader tree,
		// using the mod name as first folder level
		// angua: if there is a displayFolder present, put it between the mod name and the shader name
		std::string displayFolder = shader.getDisplayFolder();
		if (!displayFolder.empty())
		{
			addPath(shader.getModName() + "/" + displayFolder + "/" + shader.getName());
		}
		else
		{
			addPath(shader.getModName() + "/" + shader.getName());
		}
	}

	// Required visit function
	void visit(wxutil::TreeModel* store, wxutil::TreeModel::Row& row,
				const std::string& path, bool isExplicit)
	{
		// Get the display name by stripping off everything before the last slash
		std::string displayName = path.substr(path.rfind('/') + 1);

		// Fill in the column values
		row[_columns.displayName] = wxVariant(
			wxDataViewIconText(displayName, isExplicit ? _shaderIcon : _folderIcon)
		);

		// angua: we need to remove mod name and displayfolder
		// it's not possible right now to have slashes in the shader name
		row[_columns.shaderName] = displayName;
		row[_columns.isFolder] = !isExplicit;
	}
};


} // namespace

// Create the tree view
wxWindow* SoundChooser::createTreeView(wxWindow* parent)
{
	// Populate the tree store with sound shaders, using a VFS tree populator
	SoundShaderPopulator pop(_treeStore, _columns);

	// Visit all sound shaders and collect them for later insertion
	GlobalSoundManager().forEachShader(
        boost::bind(&SoundShaderPopulator::addShader, boost::ref(pop), _1)
    );

	// Let the populator iterate over all collected elements
	// and insert them in the treestore
	pop.forEachNode(pop);

	// angua: Ensure sound shaders are sorted before giving them to the tree view
	_treeStore->SortModelFoldersFirst(_columns.displayName, _columns.isFolder);

	// Tree view with single text icon column
	_treeView = wxutil::TreeView::CreateWithModel(parent, _treeStore);

	_treeView->AppendIconTextColumn(_("Soundshader"), _columns.displayName.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Use the TreeModel's full string search function
	// wxTODO _treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	// Get selection and connect the changed callback
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(SoundChooser::_onSelectionChange), NULL, this);

	// Expand the first-level row
	_treeView->Expand(_treeStore->GetRoot());

	return _treeView;
}

const std::string& SoundChooser::getSelectedShader() const
{
	return _selectedShader;
}

// Set the selected sound shader, and focuses the treeview to the new selection
void SoundChooser::setSelectedShader(const std::string& shader)
{
	wxDataViewItem found = _treeStore->FindString(shader, _columns.shaderName);

	if (found.IsOk())
	{
		_treeView->Select(found);
	}
	else
	{
		_treeView->UnselectAll();
	}
}

void SoundChooser::_onSelectionChange(wxDataViewEvent& ev)
{
	wxDataViewItem item = ev.GetItem();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_treeStore);

		bool isFolder = row[_columns.isFolder].getBool();

		_selectedShader = isFolder ? "" : static_cast<std::string>(row[_columns.shaderName]);
	}
	else
	{
		_selectedShader.clear();
	}

	// Notify the preview widget about the change
	_preview->setSoundShader(_selectedShader);
}

int SoundChooser::ShowModal()
{
	int returnCode = DialogBase::ShowModal();

	if (returnCode != wxID_OK)
	{
		_selectedShader.clear();
	}

	return returnCode;
}

std::string SoundChooser::ChooseSound(const std::string& preSelectedShader)
{
	SoundChooser* chooser = new SoundChooser;

	if (!preSelectedShader.empty())
	{
		chooser->setSelectedShader(preSelectedShader);
	}

	std::string selectedShader;

	if (chooser->ShowModal() == wxID_OK)
	{
		selectedShader = chooser->getSelectedShader();
	}

	chooser->Destroy();

	return selectedShader;
}

} // namespace
