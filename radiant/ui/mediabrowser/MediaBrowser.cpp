#include "MediaBrowser.h"
#include "TextureDirectoryLoader.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ipreferencesystem.h"
#include "ishaders.h"
#include "ieventmanager.h"

#include "gtkutil/MultiMonitor.h"

#include <glibmm/thread.h>

#include "gtkutil/menu/IconTextMenuItem.h"
#include <wx/treectrl.h>
#include <wx/dataview.h>
#include <wx/artprov.h>

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

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash/hash.hpp>

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

const std::string RKEY_MEDIA_BROWSER_PRELOAD = "user/ui/mediaBrowser/preLoadMediaTree";
const char* const OTHER_MATERIALS_FOLDER = N_("Other Materials");

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
	wxutil::TreeModel* _store;
	const MediaBrowser::TreeColumns& _columns;
	wxDataViewItem _root;

	std::string _otherMaterialsPath;

	// Maps of names to corresponding treemodel items, for both intermediate
	// paths and explicitly presented paths
	typedef std::map<std::string, wxDataViewItem, ShaderNameCompareFunctor> NamedIterMap;
	NamedIterMap _iters;

	wxIcon _folderIcon;
	wxIcon _textureIcon;

	ShaderNameFunctor(wxutil::TreeModel* store, const MediaBrowser::TreeColumns& columns) :
		_store(store),
		_columns(columns),
		_root(_store->GetRoot()),
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
		wxutil::TreeModel::Row row = _store->AddItem(parIter);

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
		wxutil::TreeModel::Row row = _store->AddItem(parent);

		std::string leafName = slashPos != std::string::npos ? name.substr(slashPos + 1) : name;

		row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, _textureIcon)); 
		row[_columns.leafName] = leafName;
		row[_columns.fullName] = name; 
		row[_columns.isFolder] = false;
		row[_columns.isOtherMaterialsFolder] = false;
	}
};

} // namespace

class MediaBrowser::Populator
{
private:
	// The event handler to notify on completion
	wxEvtHandler* _finishedHandler;

    // Column specification struct
    const MediaBrowser::TreeColumns& _columns;

    // The tree store to populate. We must operate on our own tree store, since
    // updating the MediaBrowser's tree store from a different thread
    // wouldn't be safe
	wxutil::TreeModel* _treeStore;

    // The thread object
    Glib::Thread* _thread;

	// Mutex needed in all thread-joining methods
	Glib::Mutex _mutex;

private:

    // The worker function that will execute in the thread
    void run()
    {
        // Create new treestoree
		_treeStore = new wxutil::TreeModel(_columns);
		_treeStore->SetHasDefaultCompare(false);
		
        ShaderNameFunctor functor(_treeStore, _columns);
		GlobalMaterialManager().foreachShaderName(boost::bind(&ShaderNameFunctor::visit, &functor, _1));

		// Sort the model while we're still in the worker thread
		_treeStore->SortModel(std::bind(&MediaBrowser::Populator::sortFunction, 
			this, std::placeholders::_1, std::placeholders::_2));

		wxutil::TreeModel::PopulationFinishedEvent finishedEvent;
		finishedEvent.SetTreeModel(_treeStore);

		_finishedHandler->AddPendingEvent(finishedEvent);
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

				return aName.GetString().CompareTo(bName.GetString(), wxString::ignoreCase) < 0;
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

				return aName.GetString().CompareTo(bName.GetString(), wxString::ignoreCase) < 0;
			}
		}
	} 
	
	// Ensures that the worker thread (if it exists) is finished before leaving this method
	// it's important that the join() and the NULL assignment is happening in a controlled
	// fashion to avoid calling join() on a thread object already free'd by gthread.
	void joinThreadSafe()
	{
		Glib::Mutex::Lock lock(_mutex); // only one thread should be able to execute this method at a time

		if (_thread == NULL)
		{
			// There is no thread, it might be possible that it has been free'd already
			return;
		}

        _thread->join();

		// set the thread object to NULL before releasing the lock, 
		// there might be another thread waiting to enter this block, and
		// the since the object has been freed by gthread already, it must be NULLified
		_thread = NULL; 
	}

public:

    // Construct and initialise variables
    Populator(const MediaBrowser::TreeColumns& cols, wxEvtHandler* finishedHandler) : 
		_finishedHandler(finishedHandler),
		_columns(cols), 
		_thread(NULL)
    {}

	wxutil::TreeModel* getTreeStoreAndQuit()
    {
		joinThreadSafe();

        return _treeStore;
    }

	void waitUntilFinished()
	{
		joinThreadSafe();
	}

    // Start loading entity classes in a new thread
    void populate()
    {
		Glib::Mutex::Lock lock(_mutex); // avoid concurrency with joinThreadSafe() above

		if (_thread != NULL)
		{
			return; // there is already a worker thread running
		}

        _thread = Glib::Thread::create(sigc::mem_fun(*this, &Populator::run), true);
    }
};

// Constructor
MediaBrowser::MediaBrowser() : 
	_tempParent(NULL),
	_mainWidget(NULL),
	_treeView(NULL),
	_treeStore(NULL),
	_populator(new Populator(_columns, this)),
	_preview(NULL),
	_isPopulated(false)
{}

void MediaBrowser::construct()
{
	if (_mainWidget != NULL)
	{
		return;
	}

	_tempParent = new wxFrame(NULL, wxID_ANY, "");
	_tempParent->Hide();

	_treeStore = new wxutil::TreeModel(_columns);
	// The wxWidgets algorithm sucks at sorting large flat lists of strings,
	// so we do that ourselves
	_treeStore->SetHasDefaultCompare(false);

	_mainWidget = new wxPanel(_tempParent, wxID_ANY); 
	_mainWidget->SetSizer(new wxBoxSizer(wxVERTICAL));

	_treeView = new wxDataViewCtrl(_mainWidget, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE | wxDV_NO_HEADER);
	_mainWidget->GetSizer()->Add(_treeView, 1, wxEXPAND);

	_popupMenu.reset(new wxutil::PopupMenu);

	wxDataViewColumn* textCol = _treeView->AppendIconTextColumn(
		_("Shader"), _columns.iconAndName.getColumnIndex(), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

	_treeView->SetExpanderColumn(textCol);
	textCol->SetWidth(300);

	_treeView->AssociateModel(_treeStore);
	_treeStore->DecRef();

	// Connect up the selection changed callback
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxTreeEventHandler(MediaBrowser::_onSelectionChanged), NULL, this);
	_treeView->Connect(wxEVT_PAINT, wxPaintEventHandler(MediaBrowser::_onExpose), NULL, this);

	_treeView->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, 
		wxDataViewEventHandler(MediaBrowser::_onContextMenu), NULL, this);

	// Add the info pane
	_preview = new TexturePreviewCombo(_mainWidget);
	_mainWidget->GetSizer()->Add(_preview, 0, wxEXPAND);

	// Construct the popup context menu
	_popupMenu->addItem(
		new wxutil::IconTextMenuItem(_(LOAD_TEXTURE_TEXT), LOAD_TEXTURE_ICON),
		boost::bind(&MediaBrowser::_onLoadInTexView, this),
		boost::bind(&MediaBrowser::_testLoadInTexView, this)
	);
	_popupMenu->addItem(
		new wxutil::IconTextMenuItem(_(APPLY_TEXTURE_TEXT), APPLY_TEXTURE_ICON),
		boost::bind(&MediaBrowser::_onApplyToSel, this),
		boost::bind(&MediaBrowser::_testSingleTexSel, this)
	);
	_popupMenu->addItem(
		new wxutil::IconTextMenuItem(_(SHOW_SHADER_DEF_TEXT), SHOW_SHADER_DEF_ICON),
		boost::bind(&MediaBrowser::_onShowShaderDefinition, this),
		boost::bind(&MediaBrowser::_testSingleTexSel, this)
	);

	// Connect the finish callback to load the treestore
	Connect(wxutil::EV_TREEMODEL_POPULATION_FINISHED, 
		TreeModelPopulationFinishedHandler(MediaBrowser::onTreeStorePopulationFinished), NULL, this);

	GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(*this, &MediaBrowser::onRadiantShutdown)
    );
}

wxWindow* MediaBrowser::getWidget()
{
	construct();

	return _mainWidget;
}

/* Tree query functions */

bool MediaBrowser::isDirectorySelected()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk()) return false;

	wxutil::TreeModel::Row row(item, *_treeView->GetModel());

	return row[_columns.isFolder].getBool();
}

std::string MediaBrowser::getSelectedName()
{
	// Get the selected value
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk()) return ""; // nothing selected

	// Cast to TreeModel::Row and get the full name
	wxutil::TreeModel::Row row(item, *_treeView->GetModel());
	
	return row[_columns.fullName];
}

void MediaBrowser::onRadiantShutdown()
{
	_tempParent->Destroy();

	GlobalMaterialManager().detach(*this);

	// Delete the singleton instance on shutdown
	getInstancePtr().reset();
}

/** Return the singleton instance.
 */
MediaBrowser& MediaBrowser::getInstance()
{
	MediaBrowserPtr& instancePtr = getInstancePtr();

	if (instancePtr == NULL)
	{
		instancePtr.reset(new MediaBrowser);
	}

	return *instancePtr;
}

MediaBrowserPtr& MediaBrowser::getInstancePtr()
{
	static MediaBrowserPtr _instancePtr;
	return _instancePtr;
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
	}
}

void MediaBrowser::reloadMedia()
{
	// Remove all items and clear the "isPopulated" flag
	_treeStore->Clear();
	_isPopulated = false;

	// Trigger an "expose" event
	_treeView->Refresh();
}

void MediaBrowser::init()
{
	// Create the widgets now
	getInstance().construct();

	// Check for pre-loading the textures
	if (registry::getValue<bool>(RKEY_MEDIA_BROWSER_PRELOAD))
	{
		getInstance().populate();
	}

	// Attach to the MaterialManager to get notified on unrealise/realise
	// events, in which case we're reloading the media tree
	GlobalMaterialManager().attach(getInstance());
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
	_populator->populate();
}

void MediaBrowser::onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev)
{
	_treeStore = ev.GetTreeModel();

	_treeView->AssociateModel(_treeStore);
	_treeStore->DecRef();
}

/* gtkutil::PopupMenu callbacks */

void MediaBrowser::_onLoadInTexView()
{
	// Use a TextureDirectoryLoader functor to search the directory. This
	// may throw an exception if cancelled by user.
	TextureDirectoryLoader loader(getSelectedName());

	try
	{
		GlobalMaterialManager().foreachShaderName(boost::bind(&TextureDirectoryLoader::visit, &loader, _1));
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
	selection::algorithm::applyShaderToSelection(getSelectedName());
}

// Check if a single non-directory texture is selected (used by multiple menu
// options).
bool MediaBrowser::_testSingleTexSel()
{
	if (!isDirectorySelected() && !getSelectedName().empty())
		return true;
	else
		return false;
}

void MediaBrowser::_onShowShaderDefinition()
{
	std::string shaderName = getSelectedName();

	// Construct a shader view and pass the shader name
	ShaderDefinitionView::ShowDialog(shaderName);
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

void MediaBrowser::_onSelectionChanged(wxTreeEvent& ev)
{
	// Update the preview if a texture is selected
	if (!isDirectorySelected())
	{
		_preview->SetTexture(getSelectedName());
		GlobalShaderClipboard().setSource(getSelectedName());
	}
	else
	{
		// Nothing selected, clear the clipboard
		GlobalShaderClipboard().clear();
	}
}

void MediaBrowser::toggle(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage("mediabrowser");
}

void MediaBrowser::registerCommandsAndPreferences()
{
	// Add a page to the given group
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Media Browser"));
	page->appendCheckBox("", _("Load media tree at startup"), RKEY_MEDIA_BROWSER_PRELOAD);

	GlobalCommandSystem().addCommand("ToggleMediaBrowser", toggle);
	GlobalEventManager().addCommand("ToggleMediaBrowser", "ToggleMediaBrowser");
}

} // namespace
