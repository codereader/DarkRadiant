#include "MediaBrowser.h"
#include "TextureDirectoryLoader.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ipreferencesystem.h"
#include "ishaders.h"
#include "ieventmanager.h"

#include "wxutil/MultiMonitor.h"

#include <wx/thread.h>

#include "wxutil/menu/IconTextMenuItem.h"
#include <wx/treectrl.h>
#include <wx/dataview.h>
#include <wx/artprov.h>
#include <wx/sizer.h>

#include <iostream>
#include <map>

#include "registry/registry.h"
#include "shaderlib.h"
#include "selection/algorithm/Shader.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "string/string.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/common/ShaderDefinitionView.h"
#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/common/TexturePreviewCombo.h"

#include "debugging/ScopedDebugTimer.h"
#include "modulesystem/StaticModule.h"

#include <functional>
#include <boost/algorithm/string/predicate.hpp>

namespace ui
{

/* CONSTANTS */

namespace
{

const char* FOLDER_ICON = "folder16.png";
const char* TEXTURE_ICON = "icon_texture.png";

const char* LOAD_TEXTURE_TEXT = N_("Load in Textures view");
const char* LOAD_TEXTURE_ICON = "textureLoadInTexWindow16.png";

const char* APPLY_TEXTURE_TEXT = N_("Apply to selection");
const char* APPLY_TEXTURE_ICON = "textureApplyToSelection16.png";

const char* SHOW_SHADER_DEF_TEXT = N_("Show Shader Definition");
const char* SHOW_SHADER_DEF_ICON = "icon_script.png";

const char* const OTHER_MATERIALS_FOLDER = N_("Other Materials");

const char* const SELECT_ITEMS = N_("Select elements using this shader");
const char* const DESELECT_ITEMS = N_("Deselect elements using this shader");

}

namespace 
{

/* Callback functor for processing shader names */
struct ShaderNameCompareFunctor :
	public std::binary_function<std::string, std::string, bool>
{
	bool operator()(const std::string& s1, const std::string& s2) const
	{
		// return boost::algorithm::ilexicographical_compare(s1, s2); // slow!
		return string_compare_nocase(s1.c_str(), s2.c_str()) < 0;
	}
};

struct ShaderNameFunctor
{
	// TreeStore to populate
	wxutil::TreeModel& _store;
	const MediaBrowser::TreeColumns& _columns;
	wxDataViewItem _root;

	std::string _otherMaterialsPath;

	// Maps of names to corresponding treemodel items, for both intermediate
	// paths and explicitly presented paths
	typedef std::map<std::string, wxDataViewItem, ShaderNameCompareFunctor> NamedIterMap;
	NamedIterMap _iters;

	wxIcon _folderIcon;
	wxIcon _textureIcon;

	ShaderNameFunctor(wxutil::TreeModel& store, const MediaBrowser::TreeColumns& columns) :
		_store(store),
		_columns(columns),
		_root(_store.GetRoot()),
		_otherMaterialsPath(_(OTHER_MATERIALS_FOLDER))
	{
		_folderIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
		_textureIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + TEXTURE_ICON));
	}

	// Recursive add function
	wxDataViewItem& addRecursive(const std::string& path, bool isOtherMaterial)
	{
		// Look up candidate in the map and return it if found
		NamedIterMap::iterator it = _iters.find(path);

		if (it != _iters.end())
		{
			return it->second;
		}

		/* Otherwise, split the path on its rightmost slash, call recursively on the
		 * first half in order to add the parent node, then add the second half as
		 * a child. Recursive bottom-out is when there is no slash (top-level node).
		 */
		// Find rightmost slash
		std::size_t slashPos = path.rfind("/");

		// Call recursively to get parent iter, leaving it at the toplevel if
		// there is no slash
		wxDataViewItem& parIter = 
			slashPos != std::string::npos ? addRecursive(path.substr(0, slashPos), isOtherMaterial) : _root;

		// Append a node to the tree view for this child
		wxutil::TreeModel::Row row = _store.AddItem(parIter);

		std::string name = slashPos != std::string::npos ? path.substr(slashPos + 1) : path;

		row[_columns.iconAndName] = wxVariant(wxDataViewIconText(name, _folderIcon));
		row[_columns.leafName] = name;
		row[_columns.fullName] = path; 
		row[_columns.isFolder] = true;
		row[_columns.isOtherMaterialsFolder] = isOtherMaterial;

		// Add a copy of the Gtk::TreeModel::iterator to our hashmap and return it
		std::pair<NamedIterMap::iterator, bool> result = _iters.insert(
			NamedIterMap::value_type(path, row.getItem()));

		return result.first->second;
	}

	void visit(const std::string& name)
	{
		// Find rightmost slash
		std::size_t slashPos = name.rfind("/");

		wxDataViewItem parent;

		if (boost::algorithm::istarts_with(name, GlobalTexturePrefix_get()))
		{
			// Regular texture, ensure parent folder
			parent = slashPos != std::string::npos ? addRecursive(name.substr(0, slashPos), false) : _root;
		}
		else 
		{
			// Put it under "other materials", ensure parent folder
			parent = slashPos != std::string::npos ? 
				addRecursive(_otherMaterialsPath + "/" + name.substr(0, slashPos), true) :
				addRecursive(_otherMaterialsPath, true);
		}

		// Insert the actual leaf
		wxutil::TreeModel::Row row = _store.AddItem(parent);

		std::string leafName = slashPos != std::string::npos ? name.substr(slashPos + 1) : name;

		row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, _textureIcon)); 
		row[_columns.leafName] = leafName;
		row[_columns.fullName] = name; 
		row[_columns.isFolder] = false;
		row[_columns.isOtherMaterialsFolder] = false;
	}
};

} // namespace

class MediaBrowser::Populator :
	public wxThread
{
private:
	// The event handler to notify on completion
	wxEvtHandler* _finishedHandler;

    // Column specification struct
    const MediaBrowser::TreeColumns& _columns;

    // The tree store to populate. We must operate on our own tree store, since
    // updating the MediaBrowser's tree store from a different thread
    // wouldn't be safe
	wxutil::TreeModel::Ptr _treeStore;

protected:

    // The worker function that will execute in the thread
    wxThread::ExitCode Entry()
    {
        // Create new treestoree
		_treeStore = new wxutil::TreeModel(_columns);
		_treeStore->SetHasDefaultCompare(false);
		
        ShaderNameFunctor functor(*_treeStore, _columns);
		GlobalMaterialManager().foreachShaderName(std::bind(&ShaderNameFunctor::visit, &functor, std::placeholders::_1));

		if (TestDestroy()) return static_cast<ExitCode>(0);

		// Sort the model while we're still in the worker thread
		_treeStore->SortModel(std::bind(&MediaBrowser::Populator::sortFunction, 
			this, std::placeholders::_1, std::placeholders::_2));

		if (!TestDestroy()) 
		{
			wxQueueEvent(_finishedHandler, new wxutil::TreeModel::PopulationFinishedEvent(_treeStore));
		}

		return static_cast<ExitCode>(0); 
    }

	bool sortFunction(const wxDataViewItem& a, const wxDataViewItem& b)
	{
		// Check if A or B are folders
		wxVariant aIsFolder, bIsFolder;
		_treeStore->GetValue(aIsFolder, a, _columns.isFolder.getColumnIndex());
		_treeStore->GetValue(bIsFolder, b, _columns.isFolder.getColumnIndex());

		if (aIsFolder)
		{
			// A is a folder, check if B is as well
			if (bIsFolder)
			{
				// A and B are both folders
				wxVariant aIsOtherMaterialsFolder, bIsOtherMaterialsFolder;

				_treeStore->GetValue(aIsOtherMaterialsFolder, a, _columns.isOtherMaterialsFolder.getColumnIndex());
				_treeStore->GetValue(bIsOtherMaterialsFolder, b, _columns.isOtherMaterialsFolder.getColumnIndex());

				// Special treatment for "Other Materials" folder, which always comes last
				if (aIsOtherMaterialsFolder)
				{
					return false;
				}

				if (bIsOtherMaterialsFolder)
				{
					return true;
				}

				// Compare folder names
				// greebo: We're not checking for equality here, shader names are unique
				wxVariant aName, bName;
				_treeStore->GetValue(aName, a, _columns.leafName.getColumnIndex());
				_treeStore->GetValue(bName, b, _columns.leafName.getColumnIndex());

				return aName.GetString().CmpNoCase(bName.GetString()) < 0;
			}
			else
			{
				// A is a folder, B is not, A sorts before
				return true;
			}
		}
		else
		{
			// A is not a folder, check if B is one
			if (bIsFolder)
			{
				// A is not a folder, B is, so B sorts before A
				return false;
			}
			else
			{
				// Neither A nor B are folders, compare names
				// greebo: We're not checking for equality here, shader names are unique
				wxVariant aName, bName;
				_treeStore->GetValue(aName, a, _columns.leafName.getColumnIndex());
				_treeStore->GetValue(bName, b, _columns.leafName.getColumnIndex());

				return aName.GetString().CmpNoCase(bName.GetString()) < 0;
			}
		}
	} 
	
public:

    // Construct and initialise variables
    Populator(const MediaBrowser::TreeColumns& cols, wxEvtHandler* finishedHandler) : 
		wxThread(wxTHREAD_JOINABLE),
		_finishedHandler(finishedHandler),
		_columns(cols)
    {}

	~Populator()
	{
		if (IsRunning())
		{
			Delete(); // cancel the running thread
		}
	}

	void waitUntilFinished()
	{
		if (IsRunning())
		{
			Wait();
		}
	}

    // Start loading entity classes in a new thread
    void populate()
    {
		if (IsRunning())
		{
			return;
		}

		Run();
    }
};

// Constructor
MediaBrowser::MediaBrowser() : 
	_tempParent(nullptr),
	_mainWidget(nullptr),
	_treeView(nullptr),
	_treeStore(nullptr),
	_preview(nullptr),
	_isPopulated(false)
{}

void MediaBrowser::construct()
{
	if (_mainWidget != nullptr)
	{
		return;
	}

	_tempParent = new wxFrame(nullptr, wxID_ANY, "");
	_tempParent->Hide();

	_treeStore = new wxutil::TreeModel(_columns);
	// The wxWidgets algorithm sucks at sorting large flat lists of strings,
	// so we do that ourselves
	_treeStore->SetHasDefaultCompare(false);

	_mainWidget = new wxPanel(_tempParent, wxID_ANY); 
	_mainWidget->SetSizer(new wxBoxSizer(wxVERTICAL));

	_treeView = wxutil::TreeView::Create(_mainWidget, wxDV_NO_HEADER);
	_mainWidget->GetSizer()->Add(_treeView, 1, wxEXPAND);

	_popupMenu.reset(new wxutil::PopupMenu);

	wxDataViewColumn* textCol = _treeView->AppendIconTextColumn(
		_("Shader"), _columns.iconAndName.getColumnIndex(), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

	_treeView->AddSearchColumn(_columns.iconAndName);

	_treeView->SetExpanderColumn(textCol);
	textCol->SetWidth(300);

	_treeView->AssociateModel(_treeStore.get());

	// Connect up the selection changed callback
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxTreeEventHandler(MediaBrowser::_onSelectionChanged), nullptr, this);
	_treeView->Connect(wxEVT_PAINT, wxPaintEventHandler(MediaBrowser::_onExpose), nullptr, this);

	_treeView->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, 
		wxDataViewEventHandler(MediaBrowser::_onContextMenu), nullptr, this);

	// Add the info pane
	_preview = new TexturePreviewCombo(_mainWidget);
	_mainWidget->GetSizer()->Add(_preview, 0, wxEXPAND);

	// Construct the popup context menu
	_popupMenu->addItem(
		new wxutil::IconTextMenuItem(_(LOAD_TEXTURE_TEXT), LOAD_TEXTURE_ICON),
		std::bind(&MediaBrowser::_onLoadInTexView, this),
		std::bind(&MediaBrowser::_testLoadInTexView, this)
	);
	_popupMenu->addItem(
		new wxutil::IconTextMenuItem(_(APPLY_TEXTURE_TEXT), APPLY_TEXTURE_ICON),
		std::bind(&MediaBrowser::_onApplyToSel, this),
		std::bind(&MediaBrowser::_testSingleTexSel, this)
	);
	_popupMenu->addItem(
		new wxutil::IconTextMenuItem(_(SHOW_SHADER_DEF_TEXT), SHOW_SHADER_DEF_ICON),
		std::bind(&MediaBrowser::_onShowShaderDefinition, this),
		std::bind(&MediaBrowser::_testSingleTexSel, this)
	);
    _popupMenu->addItem(
        new wxutil::IconTextMenuItem(_(SELECT_ITEMS), TEXTURE_ICON),
        std::bind(&MediaBrowser::_onSelectItems, this, true),
        std::bind(&MediaBrowser::_testSingleTexSel, this)
    );
    _popupMenu->addItem(
        new wxutil::IconTextMenuItem(_(DESELECT_ITEMS), TEXTURE_ICON),
        std::bind(&MediaBrowser::_onSelectItems, this, false),
        std::bind(&MediaBrowser::_testSingleTexSel, this)
    );

	// Connect the finish callback to load the treestore
	Connect(wxutil::EV_TREEMODEL_POPULATION_FINISHED, 
		TreeModelPopulationFinishedHandler(MediaBrowser::onTreeStorePopulationFinished), nullptr, this);
}

/* Tree query functions */

bool MediaBrowser::isDirectorySelected()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk()) return false;

	wxutil::TreeModel::Row row(item, *_treeView->GetModel());

	return row[_columns.isFolder].getBool();
}

void MediaBrowser::onRadiantStartup()
{
	// Add the Media Browser page
	IGroupDialog::PagePtr mediaBrowserPage(new IGroupDialog::Page);

	mediaBrowserPage->name = getGroupDialogTabName();
	mediaBrowserPage->windowLabel = _("Media");
	mediaBrowserPage->page = _mainWidget;
	mediaBrowserPage->tabIcon = "folder16.png";
	mediaBrowserPage->tabLabel = _("Media");
	mediaBrowserPage->position = IGroupDialog::Page::Position::MediaBrowser;

	GlobalGroupDialog().addPage(mediaBrowserPage);

	if (_tempParent != nullptr)
	{
		_tempParent->Destroy();
		_tempParent = nullptr;
	}
}

std::string MediaBrowser::getSelection()
{
	if (!_isPopulated)
	{
		return std::string();
	}

	// Get the selected value
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk())
	{
		return std::string(); // nothing selected
	}

	// Cast to TreeModel::Row and get the full name
	wxutil::TreeModel::Row row(item, *_treeView->GetModel());

	return row[_columns.fullName];
}

// Set the selection in the treeview
void MediaBrowser::setSelection(const std::string& selection)
{
	if (!_isPopulated)
	{
		populate();
	}

	// Make sure the treestore is finished loading
	_populator->waitUntilFinished();

	// If the selection string is empty, collapse the treeview and return with
	// no selection
	if (selection.empty())
	{
		_treeView->Collapse(_treeStore->GetRoot());
		return;
	}

	// Find the requested element
	wxDataViewItem item = _treeStore->FindString(selection, _columns.fullName);

	if (item.IsOk())
	{
		_treeView->Select(item);
		_treeView->EnsureVisible(item);
		handleSelectionChange();
	}
}

void MediaBrowser::realise()
{
	if (!_isPopulated)
	{
		populate();
	}
}

void MediaBrowser::unrealise()
{
	// Stop any populator thread that might be running
	_populator.reset();

	// Clear the media browser on MaterialManager unrealisation
	_treeStore->Clear();

	_isPopulated = false;
}

void MediaBrowser::populate()
{
	if (!_isPopulated)
	{
		// Clear our treestore and put a single item in it
		_treeStore->Clear();

		wxutil::TreeModel::Row row = _treeStore->AddItem();

		wxIcon icon;
		icon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + TEXTURE_ICON));
		row[_columns.iconAndName] = wxVariant(wxDataViewIconText(_("Loading, please wait..."), icon));

		row.SendItemAdded();
	}

	// Set the flag to true to avoid double-entering this function
	_isPopulated = true;

	// Start the background thread
	_populator.reset(new Populator(_columns, this));
	_populator->populate();
}

void MediaBrowser::onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev)
{
	_treeStore = ev.GetTreeModel();

	_treeView->AssociateModel(_treeStore.get());
}

/* wxutil::PopupMenu callbacks */

void MediaBrowser::_onLoadInTexView()
{
	// Use a TextureDirectoryLoader functor to search the directory. This
	// may throw an exception if cancelled by user.
	TextureDirectoryLoader loader(getSelection());

	try
	{
		GlobalMaterialManager().foreachShaderName(std::bind(&TextureDirectoryLoader::visit, &loader, std::placeholders::_1));
	}
	catch (wxutil::ModalProgressDialog::OperationAbortedException&)
	{
		// Ignore the error and return from the function normally
	}
}

bool MediaBrowser::_testLoadInTexView()
{
	// "Load in textures view" requires a directory selection
	if (isDirectorySelected())
		return true;
	else
		return false;
}

void MediaBrowser::_onApplyToSel()
{
	// Pass shader name to the selection system
	selection::algorithm::applyShaderToSelection(getSelection());
}

// Check if a single non-directory texture is selected (used by multiple menu
// options).
bool MediaBrowser::_testSingleTexSel()
{
	if (!isDirectorySelected() && !getSelection().empty())
		return true;
	else
		return false;
}

void MediaBrowser::_onShowShaderDefinition()
{
	std::string shaderName = getSelection();

	// Construct a shader view and pass the shader name
	ShaderDefinitionView::ShowDialog(shaderName);
}

void MediaBrowser::_onSelectItems(bool select)
{
    std::string shaderName = getSelection();

    if (select)
    {
        selection::algorithm::selectItemsByShader(shaderName);
    }
    else
    {
        selection::algorithm::deselectItemsByShader(shaderName);
    }
}

void MediaBrowser::_onContextMenu(wxDataViewEvent& ev)
{
	_popupMenu->show(_treeView);
}

void MediaBrowser::_onExpose(wxPaintEvent& ev)
{
	// Populate the tree view if it is not already populated
	if (!_isPopulated)
	{
		populate();
	}

	ev.Skip();
}

void MediaBrowser::handleSelectionChange()
{
	// Update the preview if a texture is selected
	if (!isDirectorySelected())
	{
		_preview->SetTexture(getSelection());
		GlobalShaderClipboard().setSource(getSelection());
	}
	else
	{
		_preview->SetTexture("");
		// Nothing selected, clear the clipboard
		GlobalShaderClipboard().clear();
	}
}

void MediaBrowser::_onSelectionChanged(wxTreeEvent& ev)
{
	handleSelectionChange();
}

void MediaBrowser::togglePage(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage(getGroupDialogTabName());
}

const std::string& MediaBrowser::getName() const
{
	static std::string _name("MediaBrowser");
	return _name;
}

const StringSet& MediaBrowser::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_SHADERSYSTEM);
		_dependencies.insert(MODULE_UIMANAGER);
	}

	return _dependencies;
}

void MediaBrowser::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	GlobalCommandSystem().addCommand("ToggleMediaBrowser", sigc::mem_fun(this, &MediaBrowser::togglePage));
	GlobalEventManager().addCommand("ToggleMediaBrowser", "ToggleMediaBrowser");

	// We need to create the liststore and widgets before attaching ourselves
	// to the material manager as observer, as the attach() call below
	// will invoke a realise() callback, which triggers a population
	construct();

	// The startup event will add this page to the group dialog tab
	GlobalRadiant().signal_radiantStarted().connect(
		sigc::mem_fun(*this, &MediaBrowser::onRadiantStartup)
	);

	// Attach to the MaterialManager to get notified on unrealise/realise
	// events, in which case we're reloading the media tree
	GlobalMaterialManager().attach(*this);
}

void MediaBrowser::shutdownModule()
{
	GlobalMaterialManager().detach(*this);
}

// Static module
module::StaticModule<MediaBrowser> mediaBrowserModule;

} // namespace
