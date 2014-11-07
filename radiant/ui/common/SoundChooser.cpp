#include "SoundChooser.h"

#include "i18n.h"
#include "iuimanager.h"
#include "isound.h"
#include "imainframe.h"

#include "wxutil/VFSTreePopulator.h"
#include "debugging/ScopedDebugTimer.h"

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


// Local class for loading sound shader definitions in a separate thread
class SoundChooser::ThreadedSoundShaderLoader :
    public wxThread
{
    // Column specification struct
    const SoundChooser::TreeColumns& _columns;

    // The tree store to populate. We must operate on our own tree store, since
    // updating the EntityClassChooser's tree store from a different thread
    // wouldn't be safe
    wxutil::TreeModel* _treeStore;

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
            boost::bind(&SoundShaderPopulator::addShader, boost::ref(visitor), _1)
        );

        if (TestDestroy()) return static_cast<ExitCode>(0);

        // Let the populator iterate over all collected elements
        // and insert them in the treestore
        visitor.forEachNode(visitor);

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
SoundChooser::SoundChooser() :
	DialogBase(_("Choose sound")),
	_treeStore(NULL),
	_treeView(NULL),
	_preview(new SoundShaderPreview(this)),
    _loadingShaders(false)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	
	GetSizer()->Add(createTreeView(this), 1, wxEXPAND | wxALL, 12);
    GetSizer()->Add(_preview, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxLEFT | wxRIGHT, 12);

	FitToScreen(0.5f, 0.5f);

    // Connect the finish callback to load the treestore
    Connect(wxutil::EV_TREEMODEL_POPULATION_FINISHED,
            TreeModelPopulationFinishedHandler(SoundChooser::_onTreeStorePopulationFinished), NULL, this);

    // Load the shaders
    loadSoundShaders();
}

// Create the tree view
wxWindow* SoundChooser::createTreeView(wxWindow* parent)
{
    _treeStore = new wxutil::TreeModel(_columns);

    // Tree view with single text icon column
	_treeView = wxutil::TreeView::CreateWithModel(parent, _treeStore);

    _treeView->AppendIconTextColumn(_("Soundshader"), _columns.displayName.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Use the TreeModel's full string search function
	_treeView->AddSearchColumn(_columns.displayName);

	// Get selection and connect the changed callback
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(SoundChooser::_onSelectionChange), NULL, this);

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

void SoundChooser::setTreeViewModel()
{
    _treeView->AssociateModel(_treeStore);
    _treeStore->DecRef();

    // Trigger a column size event on the first-level row
    wxDataViewItemArray children;
    _treeStore->GetChildren(_treeStore->GetRoot(), children);

    std::for_each(children.begin(), children.end(), [&] (wxDataViewItem& item)
    {
        _treeStore->ItemChanged(item);
    });

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
