#include "MediaBrowser.h"
#include "TextureDirectoryLoader.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ipreferencesystem.h"
#include "ishaders.h"
#include "ieventmanager.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"

//#include "debugging/ScopedDebugTimer.h"

#include <gtkmm/box.h>
#include <gtkmm/treeview.h>
#include <gtkmm/frame.h>
#include <gtkmm/dialog.h>
#include <gtkmm/stock.h>
#include <glibmm/thread.h>

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
	const MediaBrowser::TreeColumns& _columns;
	Glib::RefPtr<Gtk::TreeStore> _store;

	std::string _otherMaterialsPath;

	// Toplevel node to add children under
	Gtk::TreeModel::iterator _topLevel;

	// Maps of names to corresponding treemodel iterators, for both intermediate
	// paths and explicitly presented paths
	typedef std::map<std::string, Gtk::TreeModel::iterator, ShaderNameCompareFunctor> NamedIterMap;
	NamedIterMap _iters;

	Glib::RefPtr<Gdk::Pixbuf> _folderIcon;
	Glib::RefPtr<Gdk::Pixbuf> _textureIcon;

	ShaderNameFunctor(const Glib::RefPtr<Gtk::TreeStore>& store,
					 const MediaBrowser::TreeColumns& columns) :
		_columns(columns),
		_store(store),
		_otherMaterialsPath(_(OTHER_MATERIALS_FOLDER)),
		_folderIcon(GlobalUIManager().getLocalPixbuf(FOLDER_ICON)),
		_textureIcon(GlobalUIManager().getLocalPixbuf(TEXTURE_ICON))
	{}

	// Recursive add function
	Gtk::TreeModel::iterator& addRecursive(const std::string& path)
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
		const Gtk::TreeModel::iterator& parIter =
			slashPos != std::string::npos ? addRecursive(path.substr(0, slashPos)) : _topLevel;

		// Append a node to the tree view for this child
		Gtk::TreeModel::iterator iter = parIter ? _store->append(parIter->children()) : _store->append();

		Gtk::TreeModel::Row row = *iter;

		row[_columns.displayName] = path.substr(slashPos + 1);
		row[_columns.fullName] = path;
		row[_columns.icon] = _folderIcon;
		row[_columns.isFolder] = true;
		row[_columns.isOtherMaterialsFolder] = path.length() == _otherMaterialsPath.length() && path == _otherMaterialsPath;

		// Add a copy of the Gtk::TreeModel::iterator to our hashmap and return it
		std::pair<NamedIterMap::iterator, bool> result = _iters.insert(
			NamedIterMap::value_type(path, iter));

		return result.first->second;
	}

	void visit(const std::string& name)
	{
		// If the name starts with "textures/", add it to the treestore.
		Gtk::TreeModel::iterator& iter =
			boost::algorithm::istarts_with(name, GlobalTexturePrefix_get()) ? addRecursive(name) : addRecursive(_otherMaterialsPath + "/" + name);

		// Check the position of the last slash
		std::size_t slashPos = name.rfind("/");

		Gtk::TreeModel::Row row = *iter;

		row[_columns.displayName] = name.substr(slashPos + 1);
		row[_columns.fullName] = name;
		row[_columns.icon] = _textureIcon;
		row[_columns.isFolder] = false;
		row[_columns.isOtherMaterialsFolder] = false;
	}
};

} // namespace

class MediaBrowser::Populator
{
private:
    // The dispatcher object
    Glib::Dispatcher _dispatcher;

    // Column specification struct
    const MediaBrowser::TreeColumns& _columns;

    // The tree store to populate. We must operate on our own tree store, since
    // updating the MediaBrowser's tree store from a different thread
    // wouldn't be safe
    Glib::RefPtr<Gtk::TreeStore> _treeStore;

    // The thread object
    Glib::Thread* _thread;

	// Mutex needed in all thread-joining methods
	Glib::Mutex _mutex;

private:

    // The worker function that will execute in the thread
    void run()
    {
        ScopedDebugTimer timer("MediaBrowser::Populator::run()");

        // Create new treestoree
        _treeStore = Gtk::TreeStore::create(_columns);

        ShaderNameFunctor functor(_treeStore, _columns);
		GlobalMaterialManager().foreachShaderName(boost::bind(&ShaderNameFunctor::visit, &functor, _1));

		// Set the tree store to sort on this column (triggers sorting)
		_treeStore->set_sort_column(_columns.displayName, Gtk::SORT_ASCENDING);
		_treeStore->set_sort_func(_columns.displayName, sigc::mem_fun(MediaBrowser::getInstance(), &MediaBrowser::treeViewSortFunc));

        // Invoke dispatcher to notify the MediaBrowser
        _dispatcher();
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
    Populator(const MediaBrowser::TreeColumns& cols) : 
		_columns(cols), 
		_thread(NULL)
    {}

    // Connect the given slot to be invoked when population has finished.
    // The slot will be invoked in the main thread (to be precise, the thread
    // that called connectFinishedSlot()).
    void connectFinishedSlot(const sigc::slot<void>& slot)
    {
        _dispatcher.connect(slot);
    }

    // Return the populated treestore, and join the thread (wait for it to
    // finish and release its resources).
    Glib::RefPtr<Gtk::TreeStore> getTreeStoreAndQuit()
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
MediaBrowser::MediaBrowser()
: _treeStore(Gtk::TreeStore::create(_columns)),
  _treeView(Gtk::manage(new Gtk::TreeView(_treeStore))),
  _selection(_treeView->get_selection()),
  _populator(new Populator(_columns)),
  _popupMenu(_treeView),
  _preview(Gtk::manage(new TexturePreviewCombo)),
  _isPopulated(false)
{
	// Allocate a new top-level widget
	_widget.reset(new Gtk::VBox(false, 0));

	// Create the treeview
	_treeView->set_headers_visible(false);
	_widget->signal_expose_event().connect(sigc::mem_fun(*this, &MediaBrowser::_onExpose));

	_treeView->append_column(*Gtk::manage(new gtkutil::IconTextColumn(
		_("Shader"), _columns.displayName, _columns.icon)));

	// Use the TreeModel's full string search function
	_treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	// Pack the treeview into a scrollwindow, frame and then into the vbox
	Gtk::ScrolledWindow* scroll = Gtk::manage(new gtkutil::ScrolledFrame(*_treeView));

	Gtk::Frame* frame = Gtk::manage(new Gtk::Frame);
	frame->add(*scroll);

	_widget->pack_start(*frame, true, true, 0);

	// Connect up the selection changed callback
	_selection->signal_changed().connect(sigc::mem_fun(*this, &MediaBrowser::_onSelectionChanged));

	// Construct the popup context menu
	_popupMenu.addItem(
		Gtk::manage(new gtkutil::IconTextMenuItem(
			GlobalUIManager().getLocalPixbuf(LOAD_TEXTURE_ICON),
			_(LOAD_TEXTURE_TEXT)
		)),
		boost::bind(&MediaBrowser::_onLoadInTexView, this),
		boost::bind(&MediaBrowser::_testLoadInTexView, this)
	);
	_popupMenu.addItem(
		Gtk::manage(new gtkutil::IconTextMenuItem(
			GlobalUIManager().getLocalPixbuf(APPLY_TEXTURE_ICON),
			_(APPLY_TEXTURE_TEXT)
		)),
		boost::bind(&MediaBrowser::_onApplyToSel, this),
		boost::bind(&MediaBrowser::_testSingleTexSel, this)
	);
	_popupMenu.addItem(
		Gtk::manage(new gtkutil::IconTextMenuItem(
			GlobalUIManager().getLocalPixbuf(SHOW_SHADER_DEF_ICON),
			_(SHOW_SHADER_DEF_TEXT)
		)),
		boost::bind(&MediaBrowser::_onShowShaderDefinition, this),
		boost::bind(&MediaBrowser::_testSingleTexSel, this)
	);

	// Pack in the TexturePreviewCombo widgets
	_widget->pack_end(*_preview, false, false, 0);

	// Connect the finish callback to load the treestore
	_populator->connectFinishedSlot(
        sigc::mem_fun(*this, &MediaBrowser::getTreeStoreFromLoader)
    );

	GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(*this, &MediaBrowser::onRadiantShutdown)
    );
}

/* Tree query functions */

bool MediaBrowser::isDirectorySelected()
{
	// Get the selected value
	Gtk::TreeModel::iterator iter = _selection->get_selected();

	if (!iter) return false; // nothing selected

	// Cast to TreeModel::Row and return the full name
	return (*iter)[_columns.isFolder];
}

std::string MediaBrowser::getSelectedName()
{
	// Get the selected value
	Gtk::TreeModel::iterator iter = _selection->get_selected();

	if (!iter) return ""; // nothing selected

	// Cast to TreeModel::Row and get the full name
	std::string rv = (*iter)[_columns.fullName];

	// Strip off "Other Materials" if we need to
	std::string otherMaterialsFolder = std::string(_(OTHER_MATERIALS_FOLDER)) + "/";

	if (boost::algorithm::starts_with(rv, otherMaterialsFolder))
	{
		rv = rv.substr(otherMaterialsFolder.length());
	}

	return rv;
}

void MediaBrowser::onRadiantShutdown()
{
	GlobalMaterialManager().detach(*this);

	// Destroy our main widget
	_widget.reset();

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
		_treeView->collapse_all();
		return;
	}

	// Use the gtkutil routines to walk the TreeModel
	gtkutil::TreeModel::findAndSelectString(_treeView, selection, _columns.fullName);
}

void MediaBrowser::reloadMedia()
{
	// Remove all items and clear the "isPopulated" flag
	_treeStore->clear();
	_isPopulated = false;

	// Trigger an "expose" event
	_widget->queue_draw();
}

void MediaBrowser::init()
{
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
	_treeStore->clear();
	_isPopulated = false;
}

void MediaBrowser::populate()
{
	if (!_isPopulated)
	{
		// Clear our treestore and put a single item in it
		_treeStore->clear(); 

		Gtk::TreeModel::iterator iter = _treeStore->append();
		Gtk::TreeModel::Row row = *iter;

		row[_columns.displayName] = _("Loading, please wait...");
		row[_columns.fullName] = _("Loading, please wait...");
		row[_columns.icon] = GlobalUIManager().getLocalPixbuf(TEXTURE_ICON);
		row[_columns.isFolder] = false;
		row[_columns.isOtherMaterialsFolder] = false;
	}

	// Set the flag to true to avoid double-entering this function
	_isPopulated = true;

	// Start the background thread
	_populator->populate();
}

void MediaBrowser::getTreeStoreFromLoader()
{
    _treeStore = _populator->getTreeStoreAndQuit();
    
	_treeView->set_model(_treeStore);
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
	catch (gtkutil::ModalProgressDialog::OperationAbortedException&)
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
	ShaderDefinitionView* view = Gtk::manage(new ShaderDefinitionView);
	view->setShader(shaderName);

	Gtk::Dialog dialog(_("View Shader Definition"), GlobalMainFrame().getTopLevelWindow(), true);

	dialog.add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_OK);
	dialog.set_border_width(12);

	dialog.get_vbox()->add(*view);

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());
	dialog.set_default_size(static_cast<int>(rect.get_width()/2), static_cast<int>(2*rect.get_height()/3));

	dialog.show_all();

	// Show and block
	dialog.run();
}


bool MediaBrowser::_onExpose(GdkEventExpose* ev)
{
	// Populate the tree view if it is not already populated
	if (!_isPopulated)
	{
		populate();
	}

	return false; // progapagate event
}

void MediaBrowser::_onSelectionChanged()
{
	// Update the preview if a texture is selected
	if (!isDirectorySelected())
	{
		_preview->setTexture(getSelectedName());
		GlobalShaderClipboard().setSource(getSelectedName());
	}
	else
	{
		// Nothing selected, clear the clipboard
		GlobalShaderClipboard().clear();
	}
}

int MediaBrowser::treeViewSortFunc(const Gtk::TreeModel::iterator& a, const Gtk::TreeModel::iterator& b)
{
	Gtk::TreeModel::Row rowA = *a;
	Gtk::TreeModel::Row rowB = *b;

	// Check if A or B are folders
	bool aIsFolder = rowA[_columns.isFolder];
	bool bIsFolder = rowB[_columns.isFolder];

	if (aIsFolder)
	{
		// A is a folder, check if B is as well
		if (bIsFolder)
		{
			// A and B are both folders

			// Special treatment for "Other Materials" folder, which always comes last
			if (rowA[_columns.isOtherMaterialsFolder])
			{
				return 1;
			}

			if (rowB[_columns.isOtherMaterialsFolder])
			{
				return -1;
			}

			// Compare folder names
			// greebo: We're not checking for equality here, shader names are unique
			return a->get_value(_columns.displayName) < b->get_value(_columns.displayName) ? -1 : 1;
		}
		else
		{
			// A is a folder, B is not, A sorts before
			return -1;
		}
	}
	else
	{
		// A is not a folder, check if B is one
		if (bIsFolder)
		{
			// A is not a folder, B is, so B sorts before A
			return 1;
		}
		else
		{
			// Neither A nor B are folders, compare names
			// greebo: We're not checking for equality here, shader names are unique
			return a->get_value(_columns.displayName) < b->get_value(_columns.displayName) ? -1 : 1;
		}
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
