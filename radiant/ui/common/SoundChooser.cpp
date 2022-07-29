#include "SoundChooser.h"

#include "i18n.h"
#include "isound.h"
#include "ideclmanager.h"
#include "ifavourites.h"
#include "registry/registry.h"

#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "debugging/ScopedDebugTimer.h"
#include "ui/UserInterfaceModule.h"
#include "os/path.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>

#include <sigc++/functors/mem_fun.h>

namespace ui
{

namespace
{
	constexpr const char* const SHADER_ICON = "icon_sound.png";

    constexpr const char* const RKEY_WINDOW_STATE = "user/ui/soundChooser/window";
    constexpr const char* const RKEY_LAST_SELECTED_SHADER = "user/ui/soundChooser/lastSelectedShader";
}

// Local class for loading sound shader definitions in a separate thread
class ThreadedSoundShaderLoader :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    const wxutil::DeclarationTreeView::Columns& _columns;

public:
    ThreadedSoundShaderLoader(const wxutil::DeclarationTreeView::Columns& columns) :
        ThreadedDeclarationTreePopulator(decl::Type::SoundShader, columns, SHADER_ICON),
        _columns(columns)
    {}

    ~ThreadedSoundShaderLoader()
    {
        EnsureStopped();
    }

    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        ScopedDebugTimer timer("ThreadedSoundShaderLoader::run()");

        wxutil::VFSTreePopulator populator(model);
        
        // Visit all sound shaders and collect them for later insertion
        GlobalSoundManager().forEachShader([&](const ISoundShader::Ptr& shader)
        {
            ThrowIfCancellationRequested();

            // Construct a "path" into the sound shader tree,
            // using the mod name as first folder level
            // angua: if there is a displayFolder present, put it between the mod name and the shader name
            auto displayFolder = shader->getDisplayFolder();

            // Some shaders contain backslashes, sort them in the tree by replacing the backslashes
            auto shaderNameForwardSlashes = os::standardPath(shader->getDeclName());

            auto fullPath = !displayFolder.empty() ?
                shader->getModName() + "/" + displayFolder + "/" + shaderNameForwardSlashes :
                shader->getModName() + "/" + shaderNameForwardSlashes;

            // Sort the shader into the tree and set the values
            populator.addPath(fullPath, [&](wxutil::TreeModel::Row& row, 
                const std::string& path, const std::string& leafName, bool isFolder)
            {
                AssignValuesToRow(row, path, isFolder ? path : shader->getDeclName(), leafName, isFolder);
            });
        });
    }

    void SortModel(const wxutil::TreeModel::Ptr& model) override
    {
        // angua: Ensure sound shaders are sorted before giving them to the tree view
        model->SortModelFoldersFirst(_columns.iconAndName, _columns.isFolder);
    }
};

// Constructor
SoundChooser::SoundChooser(wxWindow* parent) :
	DialogBase(_("Choose sound"), parent),
	_treeView(nullptr),
	_preview(new SoundShaderPreview(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	
    auto* buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    auto* reloadButton = new wxButton(this, wxID_ANY, _("Reload Sounds"));
    reloadButton->Bind(wxEVT_BUTTON, &SoundChooser::_onReloadSounds, this);

    buttonSizer->Prepend(reloadButton, 0, wxRIGHT, 32);
    auto* dblClickHint = new wxStaticText( this, wxID_ANY, _( "Ctrl + Double Click on treeview items for quick play" ), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    auto* grid = new wxFlexGridSizer( 2 );
    grid->AddGrowableCol( 1 );
    grid->Add( dblClickHint, 0, wxALIGN_CENTER_VERTICAL );
    grid->Add( buttonSizer, 0, wxALIGN_RIGHT );

    createTreeView(this);
    auto* toolbar = new wxutil::ResourceTreeViewToolbar(this, _treeView);

	GetSizer()->Add(toolbar, 0, wxEXPAND | wxALIGN_LEFT | wxALL, 12);
	GetSizer()->Add(_treeView, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
    GetSizer()->Add(_preview, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	GetSizer()->Add(grid, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 12);

    _windowPosition.initialise(this, RKEY_WINDOW_STATE, 0.5f, 0.7f);

    // Load the shaders
    loadSoundShaders();
}

void SoundChooser::createTreeView(wxWindow* parent)
{
    // Tree view with single text icon column
	_treeView = new wxutil::DeclarationTreeView(parent, decl::Type::SoundShader, _columns);

    _treeView->AppendIconTextColumn(_("Soundshader"), _columns.iconAndName.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Use the TreeModel's full string search function
	_treeView->AddSearchColumn(_columns.iconAndName);
    _treeView->SetExpandTopLevelItemsAfterPopulation(true);

	// Get selection and connect the changed callback
	_treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SoundChooser::_onSelectionChange, this);
	_treeView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &SoundChooser::_onItemActivated, this);
}

void SoundChooser::loadSoundShaders()
{
    _treeView->Populate(std::make_shared<ThreadedSoundShaderLoader>(_columns));
}

const std::string& SoundChooser::getSelectedShader() const
{
	return _selectedShader;
}

void SoundChooser::setSelectedShader(const std::string& shader)
{
    _treeView->SetSelectedDeclName(shader);
}

void SoundChooser::handleSelectionChange()
{
    _selectedShader = !_treeView->IsDirectorySelected() ? 
        _treeView->GetSelectedDeclName() : std::string();

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
		wxutil::TreeModel::Row row(item, *_treeView->GetTreeModel());

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

        if ( !wxGetKeyState( WXK_CONTROL ) ) { // simple double click closes modal, ctrl+dblclk plays sound
            EndModal( wxID_OK ); 
            return;
        }

		// It's a regular item, try to play it back
		_preview->playRandomSoundFile();
	}
}

int SoundChooser::ShowModal()
{
    _windowPosition.applyPosition();

    _shadersReloaded = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::SoundShader)
        .connect(sigc::mem_fun(this, &SoundChooser::onShadersReloaded));

	int returnCode = DialogBase::ShowModal();

	if (returnCode != wxID_OK)
	{
		_selectedShader.clear();
	}

    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

	return returnCode;
}

void SoundChooser::_onReloadSounds(wxCommandEvent& ev)
{
    _preview->setSoundShader(std::string());

    // Send the command to the SoundManager
    // After parsing it will fire the sounds reloaded signal => onShadersReloaded()
    GlobalCommandSystem().executeCommand("ReloadSounds");
}

void SoundChooser::onShadersReloaded()
{
    // This signal is fired by the SoundManager, possibly from a non-UI thread
    GetUserInterfaceModule().dispatch([this]()
    {
        loadSoundShaders();
    });
}

std::string SoundChooser::chooseResource(const std::string& shaderToPreselect)
{
    auto preselected = !shaderToPreselect.empty() ? shaderToPreselect :
        registry::getValue<std::string>(RKEY_LAST_SELECTED_SHADER);

	if (!preselected.empty())
	{
		setSelectedShader(preselected);
	}

	std::string selectedShader;

	if (ShowModal() == wxID_OK)
	{
		selectedShader = getSelectedShader();

        if (!selectedShader.empty())
        {
            registry::setValue(RKEY_LAST_SELECTED_SHADER, selectedShader);
        }
	}

	return selectedShader;
}

void SoundChooser::destroyDialog()
{
    _shadersReloaded.disconnect();
	Destroy();
}

} // namespace
