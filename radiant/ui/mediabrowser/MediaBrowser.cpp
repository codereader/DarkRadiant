#include "MediaBrowser.h"
#include "TextureDirectoryLoader.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ipreferencesystem.h"
#include "ishaders.h"
#include "iregistry.h"
#include "select.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"

#include <gtkmm/box.h>
#include <gtkmm/treeview.h>
#include <gtkmm/frame.h>
#include <gtkmm/dialog.h>
#include <gtkmm/stock.h>

#include <iostream>
#include <map>

#include "selection/algorithm/Shader.h"
#include "selection/shaderclipboard/ShaderClipboard.h"

#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/common/ShaderDefinitionView.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash/hash.hpp>

namespace ui
{
	
/* CONSTANTS */

namespace {

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

// Constructor
MediaBrowser::MediaBrowser()
: _treeStore(Gtk::TreeStore::create(_columns)),
  _treeView(Gtk::manage(new Gtk::TreeView(_treeStore))),
  _selection(_treeView->get_selection()),
  _popupMenu(gtkutil::PopupMenu(_treeView)),
  _preview(Gtk::manage(new TexturePreviewCombo)),
  _isPopulated(false)
{
	// Allocate a new top-level widget
	_widget.reset(new Gtk::VBox(false, 0));

	// Create the treeview
	_treeView->set_headers_visible(false);
	_widget->signal_expose_event().connect(sigc::mem_fun(*this, &MediaBrowser::_onExpose));

	_treeView->append_column(*Gtk::manage(new gtkutil::IconTextColumnmm(
		_("Shader"), _columns.displayName, _columns.icon)));

	// Set the tree store to sort on this column
	_treeStore->set_sort_column(_columns.displayName, Gtk::SORT_ASCENDING);
	_treeStore->set_sort_func(_columns.displayName, sigc::mem_fun(*this, &MediaBrowser::treeViewSortFunc));

	// Use the TreeModel's full string search function
	_treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContainsmm));
	
	// Pack the treeview into a scrollwindow, frame and then into the vbox
	Gtk::ScrolledWindow* scroll = Gtk::manage(new gtkutil::ScrolledFramemm(*_treeView));

	Gtk::Frame* frame = Gtk::manage(new Gtk::Frame);
	frame->add(*scroll);

	_widget->pack_start(*frame, true, true, 0);
	
	// Connect up the selection changed callback
	_selection->signal_changed().connect(sigc::mem_fun(*this, &MediaBrowser::_onSelectionChanged));
	
	// Construct the popup context menu
	_popupMenu.addItem(
		Gtk::manage(new gtkutil::IconTextMenuItemmm(
			GlobalUIManager().getLocalPixbuf(LOAD_TEXTURE_ICON), 
			_(LOAD_TEXTURE_TEXT)
		)),
		boost::bind(&MediaBrowser::_onLoadInTexView, this), 
		boost::bind(&MediaBrowser::_testLoadInTexView, this)
	);
	_popupMenu.addItem(
		Gtk::manage(new gtkutil::IconTextMenuItemmm(
			GlobalUIManager().getLocalPixbuf(APPLY_TEXTURE_ICON), 
			_(APPLY_TEXTURE_TEXT)
		)),
		boost::bind(&MediaBrowser::_onApplyToSel, this), 
		boost::bind(&MediaBrowser::_testSingleTexSel, this)
	);
	_popupMenu.addItem(
		Gtk::manage(new gtkutil::IconTextMenuItemmm(
			GlobalUIManager().getLocalPixbuf(SHOW_SHADER_DEF_ICON), 
			_(SHOW_SHADER_DEF_TEXT)
		)),
		boost::bind(&MediaBrowser::_onShowShaderDefinition, this), 
		boost::bind(&MediaBrowser::_testSingleTexSel, this)
	);

	// Pack in the TexturePreviewCombo widgets
	_widget->pack_end(*_preview, false, false, 0);
}

/* Callback functor for processing shader names */

namespace
{

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

	ShaderNameFunctor(const Glib::RefPtr<Gtk::TreeStore>& store,
					 const MediaBrowser::TreeColumns& columns) :
		_store(store),
		_columns(columns),
		_otherMaterialsPath(_(OTHER_MATERIALS_FOLDER))
	{}
	
	// Recursive add function
	const Gtk::TreeModel::iterator& addRecursive(const std::string& path)
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
		row[_columns.icon] = GlobalUIManager().getLocalPixbuf(FOLDER_ICON);
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
		const Gtk::TreeModel::iterator& iter = 
			boost::algorithm::istarts_with(name, "textures/") ? addRecursive(name) : addRecursive(_otherMaterialsPath + "/" + name);
		
		// Check the position of the last slash
		std::size_t slashPos = name.rfind("/");

		Gtk::TreeModel::Row row = *iter;

		row[_columns.displayName] = name.substr(slashPos + 1);
		row[_columns.fullName] = name;
		row[_columns.icon] = GlobalUIManager().getLocalPixbuf(TEXTURE_ICON);
		row[_columns.isFolder] = false;
		row[_columns.isOtherMaterialsFolder] = false;
	}
};
	
} // namespace

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
	std::string rv = Glib::ustring((*iter)[_columns.fullName]);

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

		GlobalRadiant().addEventListener(instancePtr);
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
	if (!_isPopulated) {
		populate();
	}
	
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
	if (GlobalRegistry().get(RKEY_MEDIA_BROWSER_PRELOAD) == "1")
	{
		getInstance().populate();
	}
}

void MediaBrowser::populate()
{
	// Set the flag to true to avoid double-entering this function
	_isPopulated = true;

	ShaderNameFunctor functor(_treeStore, _columns);
	
	GlobalMaterialManager().foreachShaderName(boost::bind(&ShaderNameFunctor::visit, &functor, _1));
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

	// Special treatment for "Other Materials" folder, which always comes last
	if (rowA[_columns.isOtherMaterialsFolder])
	{
		return 1;
	}

	if (rowB[_columns.isOtherMaterialsFolder])
	{
		return -1;
	}

	// Check if A or B are folders
	bool aIsFolder = rowA[_columns.isFolder];
	bool bIsFolder = rowB[_columns.isFolder];;

	if (aIsFolder)
	{
		// A is a folder, check if B is as well
		if (bIsFolder)
		{
			// A and B are both folders, compare names
			// greebo: We're not checking for equality here, shader names are unique
			return (Glib::ustring(rowA[_columns.displayName]) < Glib::ustring(rowB[_columns.displayName])) ? -1 : 1;
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
			return (Glib::ustring(rowA[_columns.displayName]) < Glib::ustring(rowB[_columns.displayName])) ? -1 : 1;
		}
	}
}

void MediaBrowser::toggle(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage("mediabrowser");
}

void MediaBrowser::registerPreferences()
{
	// Add a page to the given group
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Media Browser"));
	
	page->appendCheckBox("", _("Load media tree at startup"), RKEY_MEDIA_BROWSER_PRELOAD);
}

} // namespace
