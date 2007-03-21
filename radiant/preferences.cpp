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

void Interface_constructPreferences(PrefPage* page)
{
#ifdef WIN32
  //page->appendCheckBox("", "Native File-Chooser", g_FileChooser_nativeGUI);
  //page->appendCheckBox("", "Default Text Editor", g_TextEditor_useWin32Editor);
#else
  {
    GtkWidget* use_custom = page->appendCheckBox("Text Editor", "Custom", g_TextEditor_useCustomEditor);
    GtkWidget* custom_editor = page->appendPathEntry("Text Editor Command", g_TextEditor_editorCommand, true);
    Widget_connectToggleDependency(custom_editor, use_custom);
  }
#endif
}

/*!
=========================================================
Games selection dialog
=========================================================
*/

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

bool Preferences_Load(PreferenceDictionary& preferences, const char* filename)
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
}

PreferenceDictionary g_global_preferences;

std::string PrefPage::getPath() const {
	return _path;
}

std::string PrefPage::getName() const {
	return _name;
}

/** greebo: Returns the widget that can be used to determine
 * 			the notebook page number.
 */
GtkWidget* PrefPage::getWidget() {
	return _pageWidget;
}

void PrefPage::foreachPage(Visitor& visitor) {
	for (unsigned int i = 0; i < _children.size(); i++) {
		// Visit this instance
		visitor.visit(_children[i]);

		// Pass the visitor recursively
		_children[i]->foreachPage(visitor);
	}
}

GtkWidget* PrefPage::appendCheckBox(const char* name, const char* flag, bool& data) {
	return NULL; //return m_dialog.addCheckBox(m_vbox, name, flag, data);
}

GtkWidget* PrefPage::appendCheckBox(const char* name, const char* flag, const BoolImportCallback& importCallback, const BoolExportCallback& exportCallback) {
	return NULL; //return m_dialog.addCheckBox(m_vbox, name, flag, importCallback, exportCallback);
}

/* greebo: This adds a checkbox and connects it to an XMLRegistry key.
 * @returns: the pointer to the created GtkWidget */
GtkWidget* PrefPage::appendCheckBox(const std::string& name, const std::string& flag, const std::string& registryKey) {
	// Create a new checkbox with the given caption and display it
	GtkWidget* check = gtk_check_button_new_with_label(flag.c_str());
	
	// Connect the registry key to this toggle button
	_connector.connectGtkObject(GTK_OBJECT(check), registryKey);
	
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(DialogRow_new(name.c_str(), check)));
	return check;
}

/* greebo: This adds a horizontal slider to the internally referenced VBox and connects
 * it to the given registryKey. */
void PrefPage::appendSlider(const std::string& name, const std::string& registryKey, bool drawValue,
                            double value, double lower, double upper, double step_increment, double page_increment, double page_size) {
	// Create a new adjustment with the boundaries <lower> and <upper> and all the increments
	GtkObject* adj = gtk_adjustment_new(value, lower, upper, step_increment, page_increment, page_size);
	
	// Connect the registry key to this adjustment
	_connector.connectGtkObject(adj, registryKey);
	
	// scale
	GtkWidget* alignment = gtk_alignment_new(0.0, 0.5, 1.0, 0.0);
	gtk_widget_show(alignment);
	
	GtkWidget* scale = gtk_hscale_new(GTK_ADJUSTMENT(adj));
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_LEFT);
	gtk_widget_show(scale);
	gtk_container_add(GTK_CONTAINER(alignment), scale);
	
	gtk_scale_set_draw_value(GTK_SCALE (scale), drawValue);
	int digits = (step_increment < 1.0f) ? 2 : 0; 
	gtk_scale_set_digits(GTK_SCALE (scale), digits);
	
	GtkTable* row = DialogRow_new(name.c_str(), alignment);
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));
}

/* greebo: Use this to add a dropdown selection box with the given list of strings as captions. The value
 * stored in the registryKey is used to determine the currently selected combobox item */
void PrefPage::appendCombo(const std::string& name, const std::string& registryKey, const ComboBoxValueList& valueList) {
	GtkWidget* alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	
	{
		// Create a new combo box
		GtkWidget* combo = gtk_combo_box_new_text();
	
		// Add all the string values to the combo box
		for (ComboBoxValueList::const_iterator i = valueList.begin(); i != valueList.end(); i++) {
			// Add the current string value to the combo box  
			gtk_combo_box_append_text(GTK_COMBO_BOX(combo), i->c_str());
		}
		
		// Connect the registry key to the newly created combo box
		_connector.connectGtkObject(GTK_OBJECT(combo), registryKey);
		
		// Add it to the container 
		gtk_container_add(GTK_CONTAINER(alignment), combo);
	}
	
	// Add the widget to the dialog row
	GtkTable* row = DialogRow_new(name.c_str(), alignment);
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));
}

/* greebo: Appends an entry field with <name> as caption which is connected to the given registryKey
 */
GtkWidget* PrefPage::appendEntry(const std::string& name, const std::string& registryKey) {
	GtkWidget* alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(alignment);

	GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
	gtk_entry_set_width_chars(entry, std::max(GlobalRegistry().get(registryKey).size(), std::size_t(10)));
	gtk_container_add(GTK_CONTAINER(alignment), GTK_WIDGET(entry));
	
	// Connect the registry key to the newly created input field
	_connector.connectGtkObject(GTK_OBJECT(entry), registryKey);

	GtkTable* row = DialogRow_new(name.c_str(), GTK_WIDGET(alignment));
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));
	return GTK_WIDGET(entry);
}

/* greebo: Appends a label field with the given caption (static)
 */
GtkWidget* PrefPage::appendLabel(const std::string& caption) {
	GtkLabel* label = GTK_LABEL(gtk_label_new(""));
	gtk_label_set_markup(label, caption.c_str());
		
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(label));
	return GTK_WIDGET(label);
}

// greebo: Adds a PathEntry to choose files or directories (depending on the given boolean)
GtkWidget* PrefPage::appendPathEntry(const std::string& name, const std::string& registryKey, bool browseDirectories) {
	PathEntry pathEntry = PathEntry_new();
	g_signal_connect(
		G_OBJECT(pathEntry.m_button), 
		"clicked", 
		G_CALLBACK(browseDirectories ? button_clicked_entry_browse_directory : button_clicked_entry_browse_file), 
		pathEntry.m_entry
	);

	// Connect the registry key to the newly created input field
	_connector.connectGtkObject(GTK_OBJECT(pathEntry.m_entry), registryKey);

	GtkTable* row = DialogRow_new(name.c_str(), GTK_WIDGET(pathEntry.m_frame));
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));

	return GTK_WIDGET(row);
}

GtkSpinButton* Spinner_new(double value, double lower, double upper, int fraction) {
	double step = 1.0 / double(fraction);
	unsigned int digits = 0;
	for (;fraction > 1; fraction /= 10) {
		++digits;
	}
	GtkSpinButton* spin = GTK_SPIN_BUTTON(gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(value, lower, upper, step, 10, 10)), step, digits));
	gtk_widget_show(GTK_WIDGET(spin));
	gtk_widget_set_size_request(GTK_WIDGET(spin), 64, -1);
	return spin;
}

/* greebo: Appends an entry field with spinner buttons which retrieves its value from the given
 * RegistryKey. The lower and upper values have to be passed as well.
 */
GtkWidget* PrefPage::appendSpinner(const std::string& name, const std::string& registryKey,
                                   double lower, double upper, int fraction) {
	// Load the initial value (maybe unnecessary, as the value is loaded upon dialog show)
	float value = GlobalRegistry().getFloat(registryKey); 
	
	GtkWidget* alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(alignment);
	
	GtkSpinButton* spin = Spinner_new(value, lower, upper, fraction);
	gtk_container_add(GTK_CONTAINER(alignment), GTK_WIDGET(spin));
	
	GtkTable* row = DialogRow_new(name.c_str(), GTK_WIDGET(alignment));
	
	// Connect the registry key to the newly created input field
	_connector.connectGtkObject(GTK_OBJECT(spin), registryKey);

	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(row));
	return GTK_WIDGET(spin);
}

void PrefPage::appendRadioIcons(const std::string& name, const std::string& registryKey, 
		const IconList& iconList, const IconDescriptionList& iconDescriptions)
{
	if (iconList.size() != iconDescriptions.size()) {
		globalErrorStream() << "PrefPage: Inconsistent Icons/IconDescription vectors!\n";
		return;
	}
	
	GtkWidget* table = gtk_table_new(3, iconList.size(), FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	
	// The radio button group
	GSList* group = NULL;
	GtkWidget* radio = NULL;
	for (unsigned int i = 0; i < iconList.size(); i++) {
		GtkWidget* image = gtk_image_new_from_pixbuf(
			gtkutil::getLocalPixbuf(iconList[i])
		);
		gtk_table_attach(GTK_TABLE(table), image, i, i+1, 0, 1, 
						 (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);
		
		GtkWidget* label = gtk_label_new(iconDescriptions[i].c_str());
		gtk_misc_set_alignment(GTK_MISC(label), 0.5f, 0.5f);
		gtk_table_attach(GTK_TABLE(table), label, i, i+1, 1, 2,
						 (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);
		
		radio = gtk_radio_button_new(group);
		gtk_table_attach(GTK_TABLE(table), radio, i, i+1, 2, 3,
						 (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio));
	}
	
	// Connect the registry key to the newly created radio
	_connector.connectGtkObject(GTK_OBJECT(radio), registryKey);
	
	DialogVBox_packRow(GTK_VBOX(_vbox), GTK_WIDGET(DialogRow_new(name.c_str(), table)));
}

void PrefPage::appendCombo(const char* name, StringArrayRange values, const IntImportCallback& importCallback, const IntExportCallback& exportCallback) {
	//m_dialog.addCombo(m_vbox, name, values, importCallback, exportCallback);
}
void PrefPage::appendCombo(const char* name, int& data, StringArrayRange values) {
	//m_dialog.addCombo(m_vbox, name, data, values);
}
void PrefPage::appendSlider(const char* name, int& data, gboolean draw_value, const char* low, const char* high, double value, double lower, double upper, double step_increment, double page_increment, double page_size) {
	//m_dialog.addSlider(m_vbox, name, data, draw_value, low, high, value, lower, upper, step_increment, page_increment, page_size);
}
void PrefPage::appendRadio(const char* name, StringArrayRange names, const IntImportCallback& importCallback, const IntExportCallback& exportCallback) {
	// m_dialog.addRadio(m_vbox, name, names, importCallback, exportCallback);
}
void PrefPage::appendRadio(const char* name, int& data, StringArrayRange names) {
	//m_dialog.addRadio(m_vbox, name, data, names);
}
void PrefPage::appendRadioIcons(const char* name, StringArrayRange icons, const IntImportCallback& importCallback, const IntExportCallback& exportCallback) {
	//m_dialog.addRadioIcons(m_vbox, name, icons, importCallback, exportCallback);
}
void PrefPage::appendRadioIcons(const char* name, int& data, StringArrayRange icons) {
	//m_dialog.addRadioIcons(m_vbox, name, data, icons);
}
GtkWidget* PrefPage::appendEntry(const char* name, const IntImportCallback& importCallback, const IntExportCallback& exportCallback) {
	return NULL; //return m_dialog.addIntEntry(m_vbox, name, importCallback, exportCallback);
}
GtkWidget* PrefPage::appendEntry(const char* name, int& data) {
	return NULL; //return m_dialog.addEntry(m_vbox, name, data);
}
GtkWidget* PrefPage::appendEntry(const char* name, const SizeImportCallback& importCallback, const SizeExportCallback& exportCallback) {
	return NULL; //return m_dialog.addSizeEntry(m_vbox, name, importCallback, exportCallback);
}
GtkWidget* PrefPage::appendEntry(const char* name, std::size_t& data) {
	return NULL; //return m_dialog.addEntry(m_vbox, name, data);
}
GtkWidget* PrefPage::appendEntry(const char* name, const FloatImportCallback& importCallback, const FloatExportCallback& exportCallback) {
	return NULL; //return m_dialog.addFloatEntry(m_vbox, name, importCallback, exportCallback);
}
GtkWidget* PrefPage::appendEntry(const char* name, float& data) {
	return NULL; //return m_dialog.addEntry(m_vbox, name, data);
}
GtkWidget* PrefPage::appendPathEntry(const char* name, bool browse_directory, const StringImportCallback& importCallback, const StringExportCallback& exportCallback) {
	return NULL; //return m_dialog.addPathEntry(m_vbox, name, browse_directory, importCallback, exportCallback);
}
GtkWidget* PrefPage::appendPathEntry(const char* name, CopiedString& data, bool directory) {
	return NULL; //return m_dialog.addPathEntry(m_vbox, name, data, directory);
}
GtkWidget* PrefPage::appendSpinner(const char* name, int& data, double value, double lower, double upper) {
	return NULL; //return m_dialog.addSpinner(m_vbox, name, data, value, lower, upper);
}
GtkWidget* PrefPage::appendSpinner(const char* name, double value, double lower, double upper, const IntImportCallback& importCallback, const IntExportCallback& exportCallback) {
	return NULL; //return m_dialog.addSpinner(m_vbox, name, value, lower, upper, importCallback, exportCallback);
}
GtkWidget* PrefPage::appendSpinner(const char* name, double value, double lower, double upper, const FloatImportCallback& importCallback, const FloatExportCallback& exportCallback) {
	return NULL; //return m_dialog.addSpinner(m_vbox, name, value, lower, upper, importCallback, exportCallback);
}


// =============================================================================
// PrefsDlg class

/*
========

very first prefs init deals with selecting the game and the game tools path
then we can load .ini stuff

using prefs / ini settings:
those are per-game

look in ~/.radiant/<version>/gamename
========
*/

void PrefsDlg::Init() {
	// m_rc_path is for game specific preferences
	// takes the form: global-pref-path/gamename/prefs-file
	// this is common to win32 and Linux init now
	m_rc_path = GlobalRegistry().get(RKEY_SETTINGS_PATH);
	// game sub-dir
	m_rc_path += game::Manager::Instance().currentGame()->getType();
	m_rc_path += ".game/";
	Q_mkdir (m_rc_path.c_str());

	// then the ini file
	m_inipath = m_rc_path;
	m_inipath += "local.pref";
}

typedef std::list<PreferenceGroupCallback> PreferenceGroupCallbacks;

inline void PreferenceGroupCallbacks_constructGroup(const PreferenceGroupCallbacks& callbacks, PreferenceGroup& group)
{
  for(PreferenceGroupCallbacks::const_iterator i = callbacks.begin(); i != callbacks.end(); ++i)
  {
    (*i)(group);
  }
}


inline void PreferenceGroupCallbacks_pushBack(PreferenceGroupCallbacks& callbacks, const PreferenceGroupCallback& callback)
{
  callbacks.push_back(callback);
}

typedef std::list<PreferencesPageCallback> PreferencesPageCallbacks;

inline void PreferencesPageCallbacks_constructPage(const PreferencesPageCallbacks& callbacks, PrefPage* page)
{
  for(PreferencesPageCallbacks::const_iterator i = callbacks.begin(); i != callbacks.end(); ++i)
  {
    (*i)(page);
  }
}

inline void PreferencesPageCallbacks_pushBack(PreferencesPageCallbacks& callbacks, const PreferencesPageCallback& callback)
{
  callbacks.push_back(callback);
}

PreferencesPageCallbacks g_interfacePreferences;
void PreferencesDialog_addInterfacePreferences(const PreferencesPageCallback& callback)
{
  PreferencesPageCallbacks_pushBack(g_interfacePreferences, callback);
}
PreferenceGroupCallbacks g_interfaceCallbacks;
void PreferencesDialog_addInterfacePage(const PreferenceGroupCallback& callback)
{
  PreferenceGroupCallbacks_pushBack(g_interfaceCallbacks, callback);
}

PreferencesPageCallbacks g_displayPreferences;
void PreferencesDialog_addDisplayPreferences(const PreferencesPageCallback& callback)
{
  PreferencesPageCallbacks_pushBack(g_displayPreferences, callback);
}
PreferenceGroupCallbacks g_displayCallbacks;
void PreferencesDialog_addDisplayPage(const PreferenceGroupCallback& callback)
{
  PreferenceGroupCallbacks_pushBack(g_displayCallbacks, callback);
}

PreferencesPageCallbacks g_settingsPreferences;
void PreferencesDialog_addSettingsPreferences(const PreferencesPageCallback& callback)
{
  PreferencesPageCallbacks_pushBack(g_settingsPreferences, callback);
}
PreferenceGroupCallbacks g_settingsCallbacks;
void PreferencesDialog_addSettingsPage(const PreferenceGroupCallback& callback)
{
  PreferenceGroupCallbacks_pushBack(g_settingsCallbacks, callback);
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


inline GtkWidget* getVBox(GtkWidget* page)
{
  return gtk_bin_get_child(GTK_BIN(page));
}

GtkTreeIter PreferenceTree_appendPage(GtkTreeStore* store, GtkTreeIter* parent, const char* name, GtkWidget* page)
{
  GtkTreeIter group;
  gtk_tree_store_append(store, &group, parent);
  gtk_tree_store_set(store, &group, 0, name, 1, page, -1);
  return group;
}

PrefPage::PrefPage(
		const std::string& name, 
		const std::string& parentPath, 
		GtkWidget* notebook,
		gtkutil::RegistryConnector& connector) :
	_name(name),
	_path(parentPath),
	_notebook(notebook),
	_connector(connector)
{
	// If this is not the root item, add a leading slash
	_path += (!_path.empty()) ? "/" : "";
	_path += _name;
	
	// Create the overall vbox
	_pageWidget = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(_pageWidget), 12);
	
	// Create the label
	GtkWidget* label = gtkutil::LeftAlignedLabel(std::string("<b>") + _name + " Settings</b>");
	gtk_box_pack_start(GTK_BOX(_pageWidget), label, FALSE, FALSE, 0);
	
	// Create the VBOX for all the client widgets
	_vbox = gtk_vbox_new(FALSE, 6);
	
	// Create the alignment for the client vbox and pack it
	GtkWidget* alignment = gtkutil::LeftAlignment(_vbox, 18, 1.0);
	gtk_box_pack_start(GTK_BOX(_pageWidget), alignment, FALSE, FALSE, 0);
	
	// Append the whole vbox as new page to the notebook
	gtk_notebook_append_page(GTK_NOTEBOOK(_notebook), _pageWidget, NULL);
}

GtkWidget* PreferencePages_addPage(GtkWidget* notebook, const char* name)
{
  GtkWidget* preflabel = gtk_label_new(name);
  gtk_widget_show(preflabel);

  GtkWidget* pageframe = gtk_frame_new(name);
  gtk_container_set_border_width(GTK_CONTAINER(pageframe), 4);
  gtk_widget_show(pageframe);

  GtkWidget* vbox = gtk_vbox_new(FALSE, 4);
  gtk_widget_show(vbox);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
  gtk_container_add(GTK_CONTAINER(pageframe), vbox);

  // Add the page to the notebook
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pageframe, preflabel);

  return pageframe;
}

class PreferenceTreeGroup : public PreferenceGroup
{
  Dialog& m_dialog;
  GtkWidget* m_notebook;
  GtkTreeStore* m_store;
  GtkTreeIter m_group;
  
  typedef std::list<PrefPage> PrefPageList;
  PrefPageList _prefPages;
public:
  PreferenceTreeGroup(Dialog& dialog, GtkWidget* notebook, GtkTreeStore* store, GtkTreeIter group) :
    m_dialog(dialog),
    m_notebook(notebook),
    m_store(store),
    m_group(group)
  {
  }
  
  PrefPage* createPage(const std::string& treeName, const std::string& frameName)
  {
    /*GtkWidget* page = PreferencePages_addPage(m_notebook, frameName.c_str());
    PreferenceTree_appendPage(m_store, &m_group, treeName.c_str(), page);
    
    _prefPages.push_back(PrefPage(m_dialog, getVBox(page)));
    
    PrefPageList::iterator i = _prefPages.end();
    
    // Return the last item in the list, except the case there is none
    if (i != _prefPages.begin()) {
    	i--;
    	PrefPage* pagePtr = &(*i); 
    	return pagePtr; 
    }*/
    
    return NULL;
  }
};

PrefPagePtr PrefPage::createOrFindPage(const std::string& path) {
	// Split the path into parts
	StringVector parts;
	boost::algorithm::split(parts, path, boost::algorithm::is_any_of("/"));
	
	if (parts.size() == 0) {
		std::cout << "Warning: Could not resolve preference path: " << path << "\n";
		return PrefPagePtr();
	}
	
	PrefPagePtr child;
	
	// Try to lookup the page in the child list
	for (unsigned int i = 0; i < _children.size(); i++) {
		if (_children[i]->getName() == parts[0]) {
			child = _children[i];
			break;
		}
	}
	
	if (child == NULL) {
		// No child found, create a new page and add it to the list
		child = PrefPagePtr(new PrefPage(parts[0], _path, _notebook, _connector));
		_children.push_back(child);
	}
	
	// We now have a child with this name, do we have a leaf?
	if (parts.size() > 1) {
		// We have still more parts, split off the first part
		std::string subPath("");
		for (unsigned int i = 1; i < parts.size(); i++) {
			subPath += (subPath.empty()) ? "" : "/";
			subPath += parts[i];
		}
		// Pass the call to the child
		return child->createOrFindPage(subPath);
	}
	else {
		// We have found a leaf, return the child page		
		return child;
	}
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
	//gtk_window_set_transient_for(GTK_WINDOW(_dialog), MainFrame_getWindow());
	gtk_window_set_title(GTK_WINDOW(_dialog), "DarkRadiant Preferences");
	gtk_window_set_modal(GTK_WINDOW(_dialog), TRUE);
	gtk_window_set_position(GTK_WINDOW(_dialog), GTK_WIN_POS_CENTER);
		
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 8);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	
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

// Construct the GTK elements for the Preferences Dialog.

GtkWindow* BuildDialog()
{
	/*
    PreferencesDialog_addInterfacePreferences(FreeCaller1<PrefPage*, Interface_constructPreferences>());
    
    // Construct the main dialog window. Set a vertical default size as the
    // size_request is too small.
    GtkWindow* dialog = create_floating_window("DarkRadiant Preferences", m_parent);
    gtk_window_set_default_size(dialog, -1, 450);

  {
    GtkWidget* mainvbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(dialog), mainvbox);
    gtk_container_set_border_width(GTK_CONTAINER(mainvbox), 5);
    gtk_widget_show(mainvbox);
  
    {
      GtkWidget* hbox = gtk_hbox_new(FALSE, 5);
      gtk_widget_show(hbox);
      gtk_box_pack_end(GTK_BOX(mainvbox), hbox, FALSE, TRUE, 0);

      {
        GtkButton* button = create_dialog_button("OK", G_CALLBACK(dialog_button_ok), &m_modal);
        gtk_box_pack_end(GTK_BOX(hbox), GTK_WIDGET(button), FALSE, FALSE, 0);
      }
      {
        GtkButton* button = create_dialog_button("Cancel", G_CALLBACK(dialog_button_cancel), &m_modal);
        gtk_box_pack_end(GTK_BOX(hbox), GTK_WIDGET(button), FALSE, FALSE, 0);
      }
      {
        GtkButton* button = create_dialog_button("Clean", G_CALLBACK(OnButtonClean), this);
        gtk_box_pack_end(GTK_BOX(hbox), GTK_WIDGET(button), FALSE, FALSE, 0);
      }
    }
  
    {
      GtkWidget* hbox = gtk_hbox_new(FALSE, 5);
      gtk_box_pack_start(GTK_BOX(mainvbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show(hbox);
  
      {
        GtkWidget* sc_win = gtk_scrolled_window_new(0, 0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sc_win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_box_pack_start(GTK_BOX(hbox), sc_win, FALSE, FALSE, 0);
        gtk_widget_show(sc_win);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sc_win), GTK_SHADOW_IN);

        // hide the notebook tabs since its not supposed to look like a notebook
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_notebook), FALSE);
        gtk_box_pack_start(GTK_BOX(hbox), m_notebook, TRUE, TRUE, 0);
        gtk_widget_show(m_notebook);


        {
          GtkTreeStore* store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);

          GtkWidget* view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
          gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

          {
            GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
            GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes("Preferences", renderer, "text", 0, 0);
            gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
          }

          {
            GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
            g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(treeSelection), this);
          }

          gtk_widget_show(view);

          gtk_container_add(GTK_CONTAINER (sc_win), view);

          {*/
            /********************************************************************/
            /* Add preference tree options                                      */
            /********************************************************************/
            // Front page... 
            //GtkWidget* front =
 /*           PreferencePages_addPage(m_notebook, "Front Page");

            {
              GtkWidget* interfacePage = PreferencePages_addPage(m_notebook, "Interface Preferences");
              {
                PrefPage preferencesPage(*this, getVBox(interfacePage));
                PreferencesPageCallbacks_constructPage(g_interfacePreferences, &preferencesPage);
              }

              GtkTreeIter group = PreferenceTree_appendPage(store, 0, "Interface", interfacePage);
              PreferenceTreeGroup preferenceGroup(*this, m_notebook, store, group);

              PreferenceGroupCallbacks_constructGroup(g_interfaceCallbacks, preferenceGroup);
            }

            {
              GtkWidget* display = PreferencePages_addPage(m_notebook, "Display Preferences");
              {
                PrefPage preferencesPage(*this, getVBox(display));
                PreferencesPageCallbacks_constructPage(g_displayPreferences, &preferencesPage);
              }
              GtkTreeIter group = PreferenceTree_appendPage(store, 0, "Display", display);
              PreferenceTreeGroup preferenceGroup(*this, m_notebook, store, group);

              PreferenceGroupCallbacks_constructGroup(g_displayCallbacks, preferenceGroup);
            }

            {
              GtkWidget* settings = PreferencePages_addPage(m_notebook, "General Settings");
              {
                PrefPage preferencesPage(*this, getVBox(settings));
                PreferencesPageCallbacks_constructPage(g_settingsPreferences, &preferencesPage);
              }

              GtkTreeIter group = PreferenceTree_appendPage(store, 0, "Settings", settings);
              PreferenceTreeGroup preferenceGroup(*this, m_notebook, store, group);

				// greebo: Invoke the registered constructors to do their stuff
				callConstructors(preferenceGroup);
				
              PreferenceGroupCallbacks_constructGroup(g_settingsCallbacks, preferenceGroup);
            }
          }

          gtk_tree_view_expand_all(GTK_TREE_VIEW(view));
    
          g_object_unref(G_OBJECT(store));
        }
      }
    }
  }

  return dialog;*/return NULL;
}

preferences_globals_t g_preferences_globals;

void PreferencesDialog_constructWindow(GtkWindow* main_window)
{
  //PrefsDlg::Instance().m_parent = main_window;
  //PrefsDlg::Instance().Create();
}
void PreferencesDialog_destroyWindow()
{
  //PrefsDlg::Instance().Destroy();
}


PreferenceDictionary g_preferences;

PreferenceSystem& GetPreferenceSystem()
{
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

void Preferences_Load()
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
  if (g_preferences_globals.disable_ini)
    return;

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
}

std::vector<const char*> g_restart_required;

void PreferencesDialog_restartRequired(const char* staticName)
{
  g_restart_required.push_back(staticName);
}

void PreferencesDialog_showDialog()
{
	PrefsDlg::toggle();
  /*if(ConfirmModified("Edit Preferences") && PrefsDlg::Instance().DoModal() == eIDOK)
  {
    if(!g_restart_required.empty())
    {
      StringOutputStream message(256);
      message << "Preference changes require a restart:\n";
      for(std::vector<const char*>::iterator i = g_restart_required.begin(); i != g_restart_required.end(); ++i)
      {
        message << (*i) << '\n';
      }
      gtk_MessageBox(GTK_WIDGET(MainFrame_getWindow()), message.c_str());
      g_restart_required.clear();
    }
  }*/
}





void GameName_importString(const char* value)
{
  gamename_set(value);
}
typedef FreeCaller1<const char*, GameName_importString> GameNameImportStringCaller;
void GameName_exportString(const StringImportCallback& importer)
{
  importer(gamename_get());
}
typedef FreeCaller1<const StringImportCallback&, GameName_exportString> GameNameExportStringCaller;

void GameMode_importString(const char* value)
{
  gamemode_set(value);
}
typedef FreeCaller1<const char*, GameMode_importString> GameModeImportStringCaller;
void GameMode_exportString(const StringImportCallback& importer)
{
  importer(gamemode_get());
}
typedef FreeCaller1<const StringImportCallback&, GameMode_exportString> GameModeExportStringCaller;


void RegisterPreferences(PreferenceSystem& preferences)
{

#ifdef WIN32
  //preferences.registerPreference("NativeGUI", BoolImportStringCaller(g_FileChooser_nativeGUI), BoolExportStringCaller(g_FileChooser_nativeGUI));
#endif


#ifdef WIN32
  preferences.registerPreference("UseCustomShaderEditor", BoolImportStringCaller(g_TextEditor_useWin32Editor), BoolExportStringCaller(g_TextEditor_useWin32Editor));
#else
  preferences.registerPreference("UseCustomShaderEditor", BoolImportStringCaller(g_TextEditor_useCustomEditor), BoolExportStringCaller(g_TextEditor_useCustomEditor));
  preferences.registerPreference("CustomShaderEditorCommand", CopiedStringImportStringCaller(g_TextEditor_editorCommand), CopiedStringExportStringCaller(g_TextEditor_editorCommand));
#endif

  preferences.registerPreference("GameName", GameNameImportStringCaller(), GameNameExportStringCaller());
  preferences.registerPreference("GameMode", GameModeImportStringCaller(), GameModeExportStringCaller());
}

void Preferences_Init()
{
  RegisterPreferences(GetPreferenceSystem());
}
