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
#include <wx/radiobut.h>

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
#include "string/predicate.h"

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

const char* const ADD_TO_FAVOURITES = N_("Add to Favourites");
const char* const REMOVE_FROM_FAVOURITES = N_("Remove from Favourites");

const char* const RKEY_FAVOURITES_ROOT = "user/ui/mediaBrowser/favourites";

}

// The set of favourite materials, which can be persisted to the registry
class MediaBrowser::Favourites :
	public std::set<std::string>
{
public:
	void loadFromRegistry()
	{
		xml::NodeList favourites = GlobalRegistry().findXPath(std::string(RKEY_FAVOURITES_ROOT) + "//favourite");

		for (xml::Node& node : favourites)
		{
			this->insert(node.getAttributeValue("value"));
		}
	}

	void saveToRegistry()
	{
		GlobalRegistry().deleteXPath(std::string(RKEY_FAVOURITES_ROOT) + "//favourite");

		xml::Node favourites = GlobalRegistry().createKey(RKEY_FAVOURITES_ROOT);

		for (const auto& favourite : *this)
		{
			xml::Node node = favourites.createChild("favourite");
			node.setAttributeValue("value", favourite);
		}
	}
};

namespace 
{

// Get the item format for favourites / non-favourites
inline wxDataViewItemAttr getItemFormat(bool isFavourite)
{
	if (isFavourite)
	{
		wxDataViewItemAttr blueBold;
		blueBold.SetColour(wxColor(0, 0, 255));
		blueBold.SetBold(true);

		return blueBold;
	}
	else
	{
		return wxDataViewItemAttr();
	}
}

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
	const MediaBrowser::Favourites& _favourites;
	wxDataViewItem _root;

	std::string _otherMaterialsPath;

	// Maps of names to corresponding treemodel items, for both intermediate
	// paths and explicitly presented paths
	typedef std::map<std::string, wxDataViewItem, ShaderNameCompareFunctor> NamedIterMap;
	NamedIterMap _iters;

	wxIcon _folderIcon;
	wxIcon _textureIcon;

	ShaderNameFunctor(wxutil::TreeModel& store, const MediaBrowser::TreeColumns& columns, const MediaBrowser::Favourites& favourites) :
		_store(store),
		_columns(columns),
		_favourites(favourites),
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
		row[_columns.isFavourite] = false; // folders are not favourites

		// Add a copy of the wxDataViewItem to our hashmap and return it
		std::pair<NamedIterMap::iterator, bool> result = _iters.insert(
			NamedIterMap::value_type(path, row.getItem()));

		return result.first->second;
	}

	void visit(const std::string& name)
	{
		// Find rightmost slash
		std::size_t slashPos = name.rfind("/");

		wxDataViewItem parent;

		if (string::istarts_with(name, GlobalTexturePrefix_get()))
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

		bool isFavourite = _favourites.find(name) != _favourites.end();

		row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, _textureIcon)); 
		row[_columns.leafName] = leafName;
		row[_columns.fullName] = name; 
		row[_columns.isFolder] = false;
		row[_columns.isOtherMaterialsFolder] = false;
		row[_columns.isFavourite] = isFavourite;

		// Formatting
		row[_columns.iconAndName] = getItemFormat(isFavourite);
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

	// The set of favourites
	const MediaBrowser::Favourites& _favourites;

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
		
        ShaderNameFunctor functor(*_treeStore, _columns, _favourites);
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
    Populator(const MediaBrowser::TreeColumns& cols, wxEvtHandler* finishedHandler, const MediaBrowser::Favourites& favourites) : 
		wxThread(wxTHREAD_JOINABLE),
		_finishedHandler(finishedHandler),
		_columns(cols),
		_favourites(favourites)
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
	_mode(TreeMode::ShowAll),
	_favourites(new Favourites),
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

	// Hbox for the favourites selection widgets
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	_showAll = new wxRadioButton(_mainWidget, wxID_ANY, _("Show All"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	_showFavourites = new wxRadioButton(_mainWidget, wxID_ANY, _("Show Favourites"));
	
	hbox->Add(_showAll, 0, wxRIGHT, 0);
	hbox->Add(_showFavourites, 0, wxLEFT, 6);

	_mainWidget->GetSizer()->Add(hbox, 0, wxALIGN_LEFT | wxALL, 6);

	_showAll->SetValue(_mode == TreeMode::ShowAll);
	_showFavourites->SetValue(_mode == TreeMode::ShowFavourites);

	_showAll->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent& ev)
	{
		handleTreeModeChanged();
	});
	_showFavourites->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent& ev)
	{
		handleTreeModeChanged();
	});

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

	_treeView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [this](wxDataViewEvent& ev)
	{
		std::string selection = getSelection();

		if (!isDirectorySelected() && !selection.empty())
		{
			// Pass shader name to the selection system
			selection::algorithm::applyShaderToSelection(selection);
		}
	});

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

	_popupMenu->addSeparator();

	_popupMenu->addItem(
		new wxutil::StockIconTextMenuItem(_(ADD_TO_FAVOURITES), wxART_ADD_BOOKMARK),
		std::bind(&MediaBrowser::_onSetFavourite, this, true),
		std::bind(&MediaBrowser::_testAddToFavourites, this)
	);
	_popupMenu->addItem(
		new wxutil::StockIconTextMenuItem(_(REMOVE_FROM_FAVOURITES), wxART_DEL_BOOKMARK),
		std::bind(&MediaBrowser::_onSetFavourite, this, false),
		std::bind(&MediaBrowser::_testRemoveFromFavourites, this)
	);

	// Connect the finish callback to load the treestore
	Connect(wxutil::EV_TREEMODEL_POPULATION_FINISHED, 
		TreeModelPopulationFinishedHandler(MediaBrowser::onTreeStorePopulationFinished), nullptr, this);

	// When destroying the main widget clear out the held references.
	// The dying populator thread might have posted a finished message which 
	// runs into problems when the _treeView is still valid
	_mainWidget->Bind(wxEVT_DESTROY, [&](wxWindowDestroyEvent& ev)
	{
		_treeView = nullptr;
		ev.Skip();
	});
}

/* Tree query functions */

bool MediaBrowser::isDirectorySelected()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk()) return false;

	wxutil::TreeModel::Row row(item, *_treeView->GetModel());

	return row[_columns.isFolder].getBool();
}

bool MediaBrowser::isFavouriteSelected()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk()) return false;

	wxutil::TreeModel::Row row(item, *_treeView->GetModel());

	return row[_columns.isFavourite].getBool();
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

void MediaBrowser::onMaterialDefsLoaded()
{
	if (!_isPopulated)
	{
		populate();
	}
}

void MediaBrowser::onMaterialDefsUnloaded()
{
	// Stop any populator thread that might be running
	_populator.reset();

	// Clear the media browser on MaterialManager unrealisation
	_treeStore->Clear();
	_emptyFavouritesLabel = wxDataViewItem();

	_isPopulated = false;
}

void MediaBrowser::populate()
{
	if (!_isPopulated)
	{
		// Set the flag to true to avoid double-entering this function
		_isPopulated = true;

		// Clear our treestore and put a single item in it
		_treeStore->Clear();
		_emptyFavouritesLabel = wxDataViewItem();

		wxutil::TreeModel::Row row = _treeStore->AddItem();

		wxIcon icon;
		icon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + TEXTURE_ICON));
		row[_columns.iconAndName] = wxVariant(wxDataViewIconText(_("Loading, please wait..."), icon));
		row[_columns.isFavourite] = true;
		row[_columns.isFolder] = false;

		row.SendItemAdded();

		// Load list from registry
		_favourites->loadFromRegistry();

		// Start the background thread
		_populator.reset(new Populator(_columns, this, *_favourites));
		_populator->populate();
	}
}

bool MediaBrowser::treeModelFilterFunc(wxutil::TreeModel::Row& row)
{
	if (_mode == TreeMode::ShowAll) return true; // everything is visible

	// Favourites mode, check if this item or any descendant is visible
	if (row[_columns.isFavourite].getBool())
	{
		return true;
	}

	wxDataViewItemArray children;
	_treeStore->GetChildren(row.getItem(), children);

	// Enter the recursion for each of the children and bail out on the first visible one
	for (const wxDataViewItem& child : children)
	{
		wxutil::TreeModel::Row childRow(child, *_treeStore);
		
		if (treeModelFilterFunc(childRow))
		{
			return true;
		}
	}

	return false;
}

void MediaBrowser::setupTreeViewAndFilter()
{
	if (!_treeStore) return;

	// Set up the filter model
	_treeModelFilter.reset(new wxutil::TreeModelFilter(_treeStore));

	_treeModelFilter->SetVisibleFunc([this](wxutil::TreeModel::Row& row)
	{
		return treeModelFilterFunc(row);
	});

	_treeView->AssociateModel(_treeModelFilter.get());

	// Remove the dummy label in any case
	if (_emptyFavouritesLabel.IsOk())
	{
		_treeStore->RemoveItem(_emptyFavouritesLabel);
		_emptyFavouritesLabel = wxDataViewItem();
	}

	if (_mode == TreeMode::ShowFavourites)
	{
		wxDataViewItemArray visibleChildren;
		if (_treeModelFilter->GetChildren(_treeModelFilter->GetRoot(), visibleChildren) == 0)
		{
			// All items filtered out, show the dummy label
			if (!_emptyFavouritesLabel.IsOk())
			{
				wxutil::TreeModel::Row row = _treeStore->AddItem();
				_emptyFavouritesLabel = row.getItem();

				wxIcon icon;
				icon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + TEXTURE_ICON));
				row[_columns.iconAndName] = wxVariant(wxDataViewIconText(_("No favourites added so far"), icon));
				row[_columns.isFavourite] = true;
				row[_columns.isFolder] = false;

				row.SendItemAdded();
			}
		}

		_treeView->ExpandTopLevelItems();
	}
}

void MediaBrowser::handleTreeModeChanged()
{
	std::string previouslySelectedItem = getSelection();

	_mode = _showAll->GetValue() ? TreeMode::ShowAll : TreeMode::ShowFavourites;

	setupTreeViewAndFilter();

	// Try to select the same item we had as before
	setSelection(previouslySelectedItem);
}

void MediaBrowser::onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev)
{
	_emptyFavouritesLabel = wxDataViewItem();

	// Check if we still have a treeview to work with, we might be in the middle of a shutdown
	// with this event being posted on the thread's last breath
	if (_treeView == nullptr)
	{
		return;
	}

	_treeStore = ev.GetTreeModel();

	// Set up the filter model
	setupTreeViewAndFilter();
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

bool MediaBrowser::_testAddToFavourites()
{
	// Adding favourites is allowed for any folder and non-favourite items 
	return isDirectorySelected() || (!getSelection().empty() && !isFavouriteSelected());
}

bool MediaBrowser::_testRemoveFromFavourites()
{
	// We can run remove from favourites on any folder or on favourites themselves
	return isDirectorySelected() || isFavouriteSelected();
}

void MediaBrowser::setFavouriteRecursively(wxutil::TreeModel::Row& row, bool isFavourite)
{
	if (row[_columns.isFolder].getBool())
	{
		// Enter recursion for this folder
		wxDataViewItemArray children;
		_treeModelFilter->GetChildren(row.getItem(), children);

		for (const wxDataViewItem& child : children)
		{
			wxutil::TreeModel::Row childRow(child, *_treeModelFilter);
			setFavouriteRecursively(childRow, isFavourite);
		}

		return;
	}

	// Not a folder, set the desired status on this item
	row[_columns.isFavourite] = isFavourite;
	row[_columns.iconAndName] = getItemFormat(isFavourite);

	// Keep track of this choice
	if (isFavourite)
	{
		_favourites->insert(row[_columns.fullName]);
	}
	else
	{
		_favourites->erase(row[_columns.fullName]);
	}

	row.SendItemChanged();

	if (_mode != TreeMode::ShowAll)
	{
		if (!_treeModelFilter->ItemIsVisible(row))
		{
			row.SendItemDeleted();
		}
		else
		{
			row.SendItemAdded();
		}
	}
}

void MediaBrowser::_onSetFavourite(bool isFavourite)
{
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk()) return;

	// Grab this item and enter recursion, propagating the favourite status
	wxutil::TreeModel::Row row(item, *_treeView->GetModel());

	setFavouriteRecursively(row, isFavourite);

	// Store to registry on each change
	_favourites->saveToRegistry();

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
	_materialDefsLoaded = GlobalMaterialManager().signal_DefsLoaded().connect(
		sigc::mem_fun(*this, &MediaBrowser::onMaterialDefsLoaded)
	);

	_materialDefsUnloaded = GlobalMaterialManager().signal_DefsUnloaded().connect(
		sigc::mem_fun(*this, &MediaBrowser::onMaterialDefsUnloaded)
	);

	// Start loading materials
	populate();
}

void MediaBrowser::shutdownModule()
{
	_emptyFavouritesLabel = wxDataViewItem();
	_materialDefsLoaded.disconnect();
	_materialDefsUnloaded.disconnect();
}

// Static module
module::StaticModule<MediaBrowser> mediaBrowserModule;

} // namespace
