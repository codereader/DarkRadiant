#include "SoundChooser.h"

#include "i18n.h"
#include "iuimanager.h"
#include "isound.h"
#include "imainframe.h"

#include "wxutil/VFSTreePopulator.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "debugging/ScopedDebugTimer.h"
#include "ui/common/SoundShaderDefinitionView.h"
#include "ui/UserInterfaceModule.h"

#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/button.h>
#include <sigc++/functors/mem_fun.h>

#include <functional>

namespace ui
{

namespace
{
	const char* const SHADER_ICON = "icon_sound.png";
	const char* const FOLDER_ICON = "folder16.png";

    const char* const SHOW_SHADER_DEF_TEXT = N_("Show Shader Definition");
    const char* const SHOW_SHADER_DEF_ICON = "icon_script.png";
}

/**
* Visitor class to enumerate sound shaders and add them to the tree store.
*/
class SoundShaderPopulator :
    public wxutil::VFSTreePopulator
{
private:
    const SoundChooser::TreeColumns& _columns;

    wxIcon _shaderIcon;
    wxIcon _folderIcon;
public:
    // Constructor
    SoundShaderPopulator(wxutil::TreeModel::Ptr treeStore,
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

        // Some shaders contain backslashes, sort them in the tree by replacing the backslashes
        std::string shaderNameForwardSlashes = shader.getName();
        std::replace(shaderNameForwardSlashes.begin(), shaderNameForwardSlashes.end(), '\\', '/');

        std::string fullPath = !displayFolder.empty() ?
            shader.getModName() + "/" + displayFolder + "/" + shaderNameForwardSlashes :
            shader.getModName() + "/" + shaderNameForwardSlashes;

        // Sort the shader into the tree and set the values
        addPath(fullPath, [&](wxutil::TreeModel::Row& row, const std::string& path, 
            const std::string& leafName, bool isFolder)
        {
            row[_columns.displayName] = wxVariant(
                wxDataViewIconText(leafName, isFolder ? _folderIcon : _shaderIcon));
            row[_columns.shaderName] = !isFolder ?  shader.getName() : std::string();
            row[_columns.isFolder] = isFolder;
            row.SendItemAdded();
        });
    }
};


// Local class for loading sound shader definitions in a separate thread
class SoundChooser::ThreadedSoundShaderLoader :
    public wxThread
{
    // Column specification struct
    const SoundChooser::TreeColumns& _columns;

    // The tree store to populate. We must operate on our own tree store, since
    // updating the EntityClassChooser's tree store from a different thread
    // wouldn't be safe
    wxutil::TreeModel::Ptr _treeStore;

    // The class to be notified on finish
    wxEvtHandler* _finishedHandler;

public:

    // Construct and initialise variables
    ThreadedSoundShaderLoader(const SoundChooser::TreeColumns& cols,
                              wxEvtHandler* finishedHandler) :
                              wxThread(wxTHREAD_JOINABLE),
        _columns(cols),
        _finishedHandler(finishedHandler)
    {}

    ~ThreadedSoundShaderLoader()
    {
        if (IsRunning())
        {
            Delete();
        }
    }

    // The worker function that will execute in the thread
    ExitCode Entry()
    {
        ScopedDebugTimer timer("ThreadedSoundShaderLoader::run()");

        // Create new treestoree
        _treeStore = new wxutil::TreeModel(_columns);

        // Populate it with the list of sound shaders by using a visitor class.
        SoundShaderPopulator visitor(_treeStore, _columns);
        
        // Visit all sound shaders and collect them for later insertion
        GlobalSoundManager().forEachShader(
            std::bind(&SoundShaderPopulator::addShader, std::ref(visitor), std::placeholders::_1)
        );

        if (TestDestroy()) return static_cast<ExitCode>(0);

        // angua: Ensure sound shaders are sorted before giving them to the tree view
        _treeStore->SortModelFoldersFirst(_columns.displayName, _columns.isFolder);

        if (!TestDestroy())
        {
            wxQueueEvent(_finishedHandler, new wxutil::TreeModel::PopulationFinishedEvent(_treeStore));
        }

        return static_cast<ExitCode>(0);
    }
};

// Constructor
SoundChooser::SoundChooser(wxWindow* parent) :
	DialogBase(_("Choose sound"), parent),
	_treeStore(nullptr),
	_treeView(nullptr),
	_preview(new SoundShaderPreview(this)),
    _loadingShaders(false)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	
    auto* buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    auto* reloadButton = new wxButton(this, wxID_ANY, _("Reload Sounds"));
    reloadButton->Bind(wxEVT_BUTTON, &SoundChooser::_onReloadSounds, this);

    buttonSizer->Prepend(reloadButton, 0, wxRIGHT, 32);

	GetSizer()->Add(createTreeView(this), 1, wxEXPAND | wxALL, 12);
    GetSizer()->Add(_preview, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	GetSizer()->Add(buttonSizer, 0, wxALIGN_RIGHT | wxBOTTOM | wxLEFT | wxRIGHT, 12);

    _popupMenu.reset(new wxutil::PopupMenu);

    _popupMenu->addItem(
        new wxutil::IconTextMenuItem(_(SHOW_SHADER_DEF_TEXT), SHOW_SHADER_DEF_ICON),
        std::bind(&SoundChooser::onShowShaderDefinition, this),
        std::bind(&SoundChooser::testShowShaderDefinition, this)
    );

	FitToScreen(0.5f, 0.5f);

    // Connect the finish callback to load the treestore
    Bind(wxutil::EV_TREEMODEL_POPULATION_FINISHED, &SoundChooser::_onTreeStorePopulationFinished, this);

    // Load the shaders
    loadSoundShaders();
}

// Create the tree view
wxWindow* SoundChooser::createTreeView(wxWindow* parent)
{
    _treeStore = new wxutil::TreeModel(_columns);

    // Tree view with single text icon column
	_treeView = wxutil::TreeView::CreateWithModel(parent, _treeStore.get());

    _treeView->AppendIconTextColumn(_("Soundshader"), _columns.displayName.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Use the TreeModel's full string search function
	_treeView->AddSearchColumn(_columns.displayName);

	// Get selection and connect the changed callback
	_treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SoundChooser::_onSelectionChange, this);
	_treeView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &SoundChooser::_onItemActivated, this);
    _treeView->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &SoundChooser::_onContextMenu, this);

	return _treeView;
}

void SoundChooser::loadSoundShaders()
{
    // Clear the tree and display a new item
    _treeStore->Clear();

    wxutil::TreeModel::Row row = _treeStore->AddItem();
    row[_columns.displayName] = wxVariant(wxDataViewIconText(_("Loading...")));
    row[_columns.shaderName] = wxString();
    row[_columns.isFolder] = false;

    row.SendItemAdded();

    _loadingShaders = true;

    // Spawn a new thread to load the items
    _shaderLoader.reset(new ThreadedSoundShaderLoader(_columns, this));
    _shaderLoader->Run();
}

const std::string& SoundChooser::getSelectedShader() const
{
	return _selectedShader;
}

// Set the selected sound shader, and focuses the treeview to the new selection
void SoundChooser::setSelectedShader(const std::string& shader)
{
    // Select immediately if possible, otherwise remember class name for later
    // selection
    if (!_loadingShaders)
    {
        wxDataViewItem item = _treeStore->FindString(shader, _columns.shaderName);

        if (item.IsOk())
        {
            _treeView->Select(item);
            _treeView->EnsureVisible(item);
            handleSelectionChange();

            _shaderToSelect.clear();

            return;
        }
    }

    // Remember this for later code
    _shaderToSelect = shader;
}

void SoundChooser::handleSelectionChange()
{
    wxDataViewItem item = _treeView->GetSelection();

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

void SoundChooser::_onSelectionChange(wxDataViewEvent& ev)
{
    handleSelectionChange();
}

void SoundChooser::_onItemActivated(wxDataViewEvent& ev)
{
	wxDataViewItem item = ev.GetItem();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_treeStore);

		bool isFolder = row[_columns.isFolder].getBool();

		if (isFolder)
		{
			// In case we double-click a folder, toggle its expanded state
			if (_treeView->IsExpanded(item))
			{
				_treeView->Collapse(item);
			}
			else
			{
				_treeView->Expand(item);
			}

			return;
		}

		// It's a regular item, try to play it back
		_preview->playRandomSoundFile();
	}
}

void SoundChooser::setTreeViewModel()
{
    _treeView->AssociateModel(_treeStore.get());

    // Trigger a column size event on the first-level row
    _treeView->TriggerColumnSizeEvent();

    // Pre-select the given class if requested by setSelectedShader()
    if (!_shaderToSelect.empty())
    {
        assert(_treeStore);
        setSelectedShader(_shaderToSelect);
    }
}

void SoundChooser::_onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev)
{
    _loadingShaders = false;

    _treeStore = ev.GetTreeModel();
    setTreeViewModel();
}

void SoundChooser::_onContextMenu(wxDataViewEvent& ev)
{
    _popupMenu->show(_treeView);
}

void SoundChooser::onShowShaderDefinition()
{
    auto* view = new SoundShaderDefinitionView(getSelectedShader(), this);
    view->ShowModal();
    view->Destroy();
}

bool SoundChooser::testShowShaderDefinition()
{
    return !_selectedShader.empty();
}

int SoundChooser::ShowModal()
{
    _shadersReloaded = GlobalSoundManager().signal_soundShadersReloaded()
        .connect(sigc::mem_fun(this, &SoundChooser::onShadersReloaded));

	int returnCode = DialogBase::ShowModal();

	if (returnCode != wxID_OK)
	{
		_selectedShader.clear();
	}

	return returnCode;
}

void SoundChooser::_onReloadSounds(wxCommandEvent& ev)
{
    // Remember the last selected shader
    _shaderToSelect = getSelectedShader();

    _preview->setSoundShader(std::string());

    // Send the command to the SoundManager
    // After parsing it will fire the sounds reloaded signal => onShadersReloaded()
    GlobalCommandSystem().executeCommand("ReloadSounds");

    _treeStore->Clear();

    wxutil::TreeModel::Row row = _treeStore->AddItem();
    row[_columns.displayName] = wxVariant(wxDataViewIconText(_("Loading...")));
    row[_columns.shaderName] = wxString();
    row[_columns.isFolder] = false;

    row.SendItemAdded();
}

void SoundChooser::onShadersReloaded()
{
    // This signal is fired by the SoundManager, possibly from a non-UI thread
    GetUserInterfaceModule().dispatch([this]()
    {
        loadSoundShaders();
    });
}

std::string SoundChooser::chooseResource(const std::string& preselected)
{
	if (!preselected.empty())
	{
		setSelectedShader(preselected);
	}

	std::string selectedShader;

	if (ShowModal() == wxID_OK)
	{
		selectedShader = getSelectedShader();
	}

	return selectedShader;
}

void SoundChooser::destroyDialog()
{
    _shadersReloaded.disconnect();
	Destroy();
}

} // namespace
