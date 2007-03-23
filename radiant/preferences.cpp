/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

//
// User preferences
//
// Leonardo Zide (leo@lokigames.com)
//

#include "preferences.h"

#include "debugging/debugging.h"

#include <gtk/gtk.h>
#include <iostream>

#include "generic/callback.h"
#include "string/string.h"
#include "stream/stringstream.h"
#include "os/file.h"
#include "os/path.h"
#include "os/dir.h"
#include "gtkutil/dialog.h"
#include "gtkutil/image.h"
#include "gtkutil/filechooser.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/LeftAlignment.h"
#include "cmdlib.h"
#include "plugin.h"
#include "gtkmisc.h"

#include "environment.h"
#include "error.h"
#include "console.h"
#include "mainframe.h"
#include "qe3.h"
#include "gtkdlgs.h"
#include "settings/GameManager.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

	namespace {
		// The treestore enumeration for the preference tree
		enum {
			NAME_COL,		// The column with the caption (for lookups)
			PREFPAGE_COL,	// The pointer to the preference page 
		};
		typedef std::vector<std::string> StringVector;
	}

#include <map>

#include "warnings.h"
#include "stream/textfilestream.h"
#include "container/array.h"
#include "xml/ixml.h"
#include "xml/xmlparser.h"
#include "xml/xmlwriter.h"

#include "preferencedictionary.h"
#include "stringio.h"

const char* const PREFERENCES_VERSION = "1.0";

/*bool Preferences_Load(PreferenceDictionary& preferences, const char* filename)
{
  TextFileInputStream file(filename);
  if(!file.failed())
  {
    XMLStreamParser parser(file);
    XMLPreferenceDictionaryImporter importer(preferences, PREFERENCES_VERSION);
    parser.exportXML(importer);
    return true;
  }
  return false;
}

bool Preferences_Save(PreferenceDictionary& preferences, const char* filename)
{
  TextFileOutputStream file(filename);
  if(!file.failed())
  {
    XMLStreamWriter writer(file);
    XMLPreferenceDictionaryExporter exporter(preferences, PREFERENCES_VERSION);
    exporter.exportXML(writer);
    return true;
  }
  return false;
}

bool Preferences_Save_Safe(PreferenceDictionary& preferences, const char* filename)
{
  Array<char> tmpName(filename, filename + strlen(filename) + 1 + 3);
  *(tmpName.end() - 4) = 'T';
  *(tmpName.end() - 3) = 'M';
  *(tmpName.end() - 2) = 'P';
  *(tmpName.end() - 1) = '\0';

  return Preferences_Save(preferences, tmpName.data())
    && (!file_exists(filename) || file_remove(filename))
    && file_move(tmpName.data(), filename);
}*/

PreferenceDictionary g_global_preferences;

void resetPreferences() {
	// Load user preferences, these overwrite any values that have defined before
	// The called method also checks for any upgrades that have to be performed
	const std::string userSettingsFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + "user.xml";
	if (file_exists(userSettingsFile.c_str())) {
		file_remove(userSettingsFile.c_str());
		GlobalRegistry().set(RKEY_SKIP_REGISTRY_SAVE, "1");
	}
}

void Widget_updateDependency(GtkWidget* self, GtkWidget* toggleButton)
{
  gtk_widget_set_sensitive(self, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggleButton)) && GTK_WIDGET_IS_SENSITIVE(toggleButton));
}

void ToggleButton_toggled_Widget_updateDependency(GtkWidget *toggleButton, GtkWidget* self)
{
  Widget_updateDependency(self, toggleButton);
}

void ToggleButton_state_changed_Widget_updateDependency(GtkWidget* toggleButton, GtkStateType state, GtkWidget* self)
{
  if(state == GTK_STATE_INSENSITIVE)
  {
    Widget_updateDependency(self, toggleButton);
  }
}

void Widget_connectToggleDependency(GtkWidget* self, GtkWidget* toggleButton)
{
  g_signal_connect(G_OBJECT(toggleButton), "state_changed", G_CALLBACK(ToggleButton_state_changed_Widget_updateDependency), self);
  g_signal_connect(G_OBJECT(toggleButton), "toggled", G_CALLBACK(ToggleButton_toggled_Widget_updateDependency), self);
  Widget_updateDependency(self, toggleButton);
}

/** greebo: A hybrid walker that is used twice:
 * 			First time to add each PrefPage to the TreeStore using a VFSTreePopulator
 * 			Second time as VFSTreeVisitor to store the data into the treestore	
 */
class PrefTreePopulator :
	public PrefPage::Visitor,
	public gtkutil::VFSTreePopulator::Visitor 
{
	// The helper class creating the GtkTreeIter
	gtkutil::VFSTreePopulator& _vfsPopulator;
	
	PrefsDlg* _dialog;
	
public:
	PrefTreePopulator(gtkutil::VFSTreePopulator& vfsPopulator, PrefsDlg* dialog) :
		_vfsPopulator(vfsPopulator),
		_dialog(dialog)
	{}

	void visit(PrefPagePtr prefPage) {
		// Check for an empty path (this would be the root item)
		if (!prefPage->getPath().empty()) {
			// Tell the VFSTreePopulator to add the item with this path
			_vfsPopulator.addPath(prefPage->getPath());
		}
	}
	
	void visit(GtkTreeStore* store, GtkTreeIter* iter, 
			   const std::string& path, bool isExplicit)
	{
		// Do not process add the root item
		if (!path.empty()) {
			// Get the leaf name (truncate the path)
			std::string leafName = path.substr(path.rfind("/")+1);
			
			// Get a reference to the page defined by this path
			PrefPagePtr page = _dialog->createOrFindPage(path);
			
			if (page != NULL) {
				// Add the caption to the liststore
				gtk_tree_store_set(store, iter, 
								   NAME_COL, leafName.c_str(),
								   PREFPAGE_COL, page->getWidget(), 
								   -1);
			}
		}
	}
};

/** greebo: A walker searching for a page matching the given path.
 * 			The result is stored in the passed PrefPagePtr&
 */
class PrefPageFinder :
	public PrefPage::Visitor 
{
	// The helper class creating the GtkTreeIter
	PrefPagePtr& _page;
	
	// The path to look up
	std::string _path;
public:
	PrefPageFinder(const std::string& path, PrefPagePtr& page) :
		_page(page),
		_path(path)
	{
		// Initialise the search result to "empty"
		_page = PrefPagePtr();
	}

	void visit(PrefPagePtr prefPage) {
		// Check for a match
		if (prefPage->getPath() == _path) {
			_page = prefPage;
		}
	}
};

PrefsDlg::PrefsDlg() :
	_dialog(NULL),
	_packed(false),
	_isModal(false)
{
	// Create a treestore with a name and a pointer
	_prefTree = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	
	// Create all the widgets
	populateWindow();
	
	// Create the root element with the Notebook and Connector references 
	_root = PrefPagePtr(new PrefPage("", "", _notebook, _registryConnector));
}

void PrefsDlg::populateWindow() {
	// The overall dialog vbox
	_overallVBox = gtk_vbox_new(FALSE, 8);
	
	GtkWidget* hbox = gtk_hbox_new(FALSE, 8);
	
	_treeView = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(_prefTree)));
	g_object_unref(G_OBJECT(_prefTree));
	
	_selection = gtk_tree_view_get_selection(_treeView);
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(onPrefPageSelect), this);
	
	gtk_tree_view_set_headers_visible(_treeView, FALSE);
	gtk_tree_view_append_column(_treeView, gtkutil::TextColumn("Category", 0)); 
	
	gtk_widget_set_size_request(GTK_WIDGET(_treeView), 170, -1);
	GtkWidget* scrolledFrame = gtkutil::ScrolledFrame(GTK_WIDGET(_treeView));
	gtk_box_pack_start(GTK_BOX(hbox), scrolledFrame, FALSE, FALSE, 0);
	
	_notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(_notebook), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), _notebook, TRUE, TRUE, 0);
	
	// Pack the notebook and the treeview into the overall dialog vbox
	gtk_box_pack_start(GTK_BOX(_overallVBox), hbox, TRUE, TRUE, 0);
	
	// Create the buttons
	GtkWidget* buttonHBox = gtk_hbox_new(FALSE, 0);
	
	GtkWidget* saveButton = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_box_pack_end(GTK_BOX(buttonHBox), saveButton, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(saveButton), "clicked", G_CALLBACK(onSave), this);
	
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_box_pack_end(GTK_BOX(buttonHBox), cancelButton, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
	
	gtk_box_pack_start(GTK_BOX(_overallVBox), buttonHBox, FALSE, FALSE, 0);
}

void PrefsDlg::updateTreeStore() {
	// Clear the tree before populating it
	gtk_tree_store_clear(_prefTree);
	
	// Instantiate a new populator class
	gtkutil::VFSTreePopulator vfsTreePopulator(_prefTree);
	
	PrefTreePopulator visitor(vfsTreePopulator, this);
	
	// Visit each page with the PrefTreePopulator 
	// (which in turn is using the VFSTreePopulator helper)
	_root->foreachPage(visitor);
	
	// All the GtkTreeIters are available, we should add the data now
	// re-use the visitor, it provides both visit() methods
	vfsTreePopulator.forEachNode(visitor);
}

PrefPagePtr PrefsDlg::createOrFindPage(const std::string& path) {
	// Pass the call to the root page
	return _root->createOrFindPage(path);
}

void PrefsDlg::initDialog() {
	
	_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(_dialog), "DarkRadiant Preferences");
	gtk_window_set_modal(GTK_WINDOW(_dialog), TRUE);
	gtk_window_set_position(GTK_WINDOW(_dialog), GTK_WIN_POS_CENTER);
		
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 8);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", G_CALLBACK(onDelete), this);
	
	gtk_container_add(GTK_CONTAINER(_dialog), _overallVBox);
}

void PrefsDlg::shutdown() {
	if (_dialog != NULL) {
		gtk_widget_hide(_dialog);
	}
}

void PrefsDlg::toggleWindow(bool isModal) {
	// Pass the call to the utility methods that save/restore the window position
	if (_dialog != NULL && GTK_WIDGET_VISIBLE(_dialog)) {
		gtk_widget_hide_all(_dialog);
	}
	else {
		if (!_packed) {
			// Window container not created yet
			_packed = true;
			initDialog();
		}
		
		// Import the registry keys 
		_registryConnector.importValues();
		
		// Rebuild the tree and expand it
		updateTreeStore();
		gtk_tree_view_expand_all(_treeView);
		
		// Now show the dialog window again
		gtk_widget_show_all(_dialog);
		
		// Is there a specific page display request?
		if (!_requestedPage.empty()) {
			showPage(_requestedPage);
		}
		
		if (isModal) {
			_isModal = true;
			
			// Resize the window to fit the widgets exactly
			gtk_widget_set_size_request(_dialog, -1, -1);
			// Reposition the modal dialog, it has been reset by the size_request call
			gtk_window_set_position(GTK_WINDOW(_dialog), GTK_WIN_POS_CENTER);
			// Enter the main loop, gtk_main_quit() is called by the buttons
			gtk_main();
		}
	}
}

void PrefsDlg::toggle() {
	Instance().toggleWindow();
}

PrefsDlg& PrefsDlg::Instance() {
	static PrefsDlg _instance;
	return _instance;
}

void PrefsDlg::selectPage() {
	// Get the widget* pointer from the current selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(
		_selection, &model, &iter
	);
	
	if (anythingSelected) {
		// Retrieve the pointer from the current row and cast it to a GtkWidget*
		gpointer widgetPtr = gtkutil::TreeModel::getPointer(model, &iter, PREFPAGE_COL);
		GtkWidget* page = reinterpret_cast<GtkWidget*>(widgetPtr);
		
		int pagenum = gtk_notebook_page_num(GTK_NOTEBOOK(_notebook), page);
		if (gtk_notebook_get_current_page(GTK_NOTEBOOK(_notebook)) != pagenum) {
			gtk_notebook_set_current_page(GTK_NOTEBOOK(_notebook), pagenum);
		}
	}
}

void PrefsDlg::showPage(const std::string& path) {
	PrefPagePtr page;
	
	PrefPageFinder finder(path, page);
	_root->foreachPage(finder);
	
	if (page != NULL) {
		GtkWidget* notebookPage = page->getWidget();
		int pagenum = gtk_notebook_page_num(GTK_NOTEBOOK(_notebook), notebookPage);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(_notebook), pagenum);
	}
}

void PrefsDlg::save() {
	_registryConnector.exportValues();
	
	if (_isModal) {
		gtk_main_quit();
	}
	toggleWindow();
	_requestedPage = "";
	_isModal = false;
	UpdateAllWindows();
}

void PrefsDlg::cancel() {
	if (_isModal) {
		gtk_main_quit();
	}
	toggleWindow();
	_requestedPage = "";
	_isModal = false;
}

void PrefsDlg::showModal(const std::string& path) {
	if (!Instance().isVisible()) {
		Instance()._requestedPage = path;
		Instance().toggleWindow(true);
	}
}

void PrefsDlg::showProjectSettings() {
	showModal("Game");
}

bool PrefsDlg::isVisible() const {
	return (_dialog != NULL && GTK_WIDGET_VISIBLE(_dialog));
}

// Static GTK Callbacks
void PrefsDlg::onSave(GtkWidget* button, PrefsDlg* self) {
	self->save();
}

void PrefsDlg::onCancel(GtkWidget* button, PrefsDlg* self) {
	self->cancel();
} 

void PrefsDlg::onPrefPageSelect(GtkTreeSelection* treeselection, PrefsDlg* self) {
	self->selectPage();
}

gboolean PrefsDlg::onDelete(GtkWidget* widget, GdkEvent* event, PrefsDlg* self) {
	// Closing the dialog is equivalent to CANCEL
	self->cancel();

	// Don't propagate the delete event
	return true;
}

PreferenceDictionary g_preferences;

PreferenceSystem& GetPreferenceSystem() {
  return g_preferences;
}

class PreferenceSystemAPI
{
  PreferenceSystem* m_preferencesystem;
public:
  typedef PreferenceSystem Type;
  STRING_CONSTANT(Name, "*");

  PreferenceSystemAPI()
  {
    m_preferencesystem = &GetPreferenceSystem();
  }
  PreferenceSystem* getTable()
  {
    return m_preferencesystem;
  }
};

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

typedef SingletonModule<PreferenceSystemAPI> PreferenceSystemModule;
typedef Static<PreferenceSystemModule> StaticPreferenceSystemModule;
StaticRegisterModule staticRegisterPreferenceSystem(StaticPreferenceSystemModule::instance());

/*void Preferences_Load()
{
  // load global .pref file
	std::string globalPrefFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + "global.pref";
	globalOutputStream() << "loading global preferences from " << makeQuoted(globalPrefFile.c_str()) << "\n";

	if (!Preferences_Load(g_global_preferences, globalPrefFile.c_str())) {
		globalOutputStream() << "failed to load global preferences from " << globalPrefFile.c_str() << "\n";
	}

  globalOutputStream() << "loading local preferences from " << PrefsDlg::Instance().m_inipath.c_str() << "\n";

  if(!Preferences_Load(g_preferences, PrefsDlg::Instance().m_inipath.c_str()))
  {
    globalOutputStream() << "failed to load local preferences from " << PrefsDlg::Instance().m_inipath.c_str() << "\n";
  }
}

void Preferences_Save()
{
  std::string globalPrefFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + "global.pref";

	globalOutputStream() << "saving global preferences to " << globalPrefFile.c_str() << "\n";

	if (!Preferences_Save_Safe(g_global_preferences, globalPrefFile.c_str())) {
		globalOutputStream() << "failed to save global preferences to " << globalPrefFile.c_str() << "\n";
	}

  globalOutputStream() << "saving local preferences to " << PrefsDlg::Instance().m_inipath.c_str() << "\n";

  if(!Preferences_Save_Safe(g_preferences, PrefsDlg::Instance().m_inipath.c_str()))
  {
    globalOutputStream() << "failed to save local preferences to " << PrefsDlg::Instance().m_inipath.c_str() << "\n";
  }
}

void Preferences_Reset()
{
  file_remove(PrefsDlg::Instance().m_inipath.c_str());
}*/
