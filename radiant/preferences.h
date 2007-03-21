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

/*
The following source code is licensed by Id Software and subject to the terms of 
its LIMITED USE SOFTWARE LICENSE AGREEMENT, a copy of which is included with 
GtkRadiant. If you did not receive a LIMITED USE SOFTWARE LICENSE AGREEMENT, 
please contact Id Software immediately at info@idsoftware.com.
*/

#if !defined(INCLUDED_PREFERENCES_H)
#define INCLUDED_PREFERENCES_H

#include "preferencesystem.h"

#include "xmlutil/Document.h"

#include "gtkutil/RegistryConnector.h"
#include "gtkutil/WindowPosition.h"
#include "libxml/parser.h"
#include "dialog.h"
#include <list>
#include <map>
#include <boost/shared_ptr.hpp>

void Widget_connectToggleDependency(GtkWidget* self, GtkWidget* toggleButton);

class PrefPage;
typedef boost::shared_ptr<PrefPage> PrefPagePtr;

class PrefPage : 
	public PreferencesPage
{
public:
	class Visitor 
	{
	public:
		virtual void visit(PrefPagePtr prefPage) = 0;
	};

private:
	// The vbox this page is adding the widgets to
	GtkWidget* _vbox;
  
  	// The list of child pages
	std::vector<PrefPagePtr> _children;
	
	// The name (caption) of this page
	std::string _name;
	
	// The full path of this object
	std::string _path;
	
	// The notebook this page is packed into
	GtkWidget* _notebook;
	
	// The actual page that gets attached to the notebook
	GtkWidget* _pageWidget;
	
	// The reference to the dialog's connector object
	gtkutil::RegistryConnector& _connector;

public:
	/** greebo: Constructor
	 * 
	 * @name: The display caption of this prefpage
	 * @parentPath: the path to the parent of this page
	 * @notebook: The GtkNotebook widget this page is child of.
	 */
	PrefPage(const std::string& name,
	         const std::string& parentPath,
	         GtkWidget* notebook,
	         gtkutil::RegistryConnector& connector);
	
	/** greebo: Returns the full path to this PrefPage
	 */
	std::string getPath() const;
	
	/** greebo: Returns the name (caption) of this Page (e.g. "Settings")
	 */
	std::string getName() const;
	
	/** greebo: Returns the widget that can be used to determine
	 * 			the notebook page number.
	 */
	GtkWidget* getWidget();
	
	void foreachPage(Visitor& visitor);
	
	GtkWidget* appendCheckBox(const char* name, const char* flag, bool& data);
	GtkWidget* appendCheckBox(const char* name, const char* flag, const BoolImportCallback& importCallback, const BoolExportCallback& exportCallback);
	
	/* greebo: This adds a checkbox and connects it to an XMLRegistry key.
	 * @returns: the pointer to the created GtkWidget */
	GtkWidget* appendCheckBox(const std::string& name, const std::string& flag, const std::string& registryKey);
	
	/* greebo: This adds a horizontal slider to the internally referenced VBox and connects
	 * it to the given registryKey. */
	void appendSlider(const std::string& name, const std::string& registryKey, bool drawValue,
	                  double value, double lower, double upper, double step_increment, double page_increment, double page_size) ;
	
	/* greebo: Use this to add a dropdown selection box with the given list of strings as captions. The value
	 * stored in the registryKey is used to determine the currently selected combobox item */
	void appendCombo(const std::string& name, const std::string& registryKey, const ComboBoxValueList& valueList);
	
	/* greebo: Appends an entry field with <name> as caption which is connected to the given registryKey
	 */
	GtkWidget* appendEntry(const std::string& name, const std::string& registryKey);
	
	// greebo: Adds a PathEntry to choose files or directories (depending on the given boolean)
	GtkWidget* appendPathEntry(const std::string& name, const std::string& registryKey, bool browseDirectories);
	
	/* greebo: Appends an entry field with spinner buttons which retrieves its value from the given
	 * RegistryKey. The lower and upper values have to be passed as well.
	 */
	GtkWidget* appendSpinner(const std::string& name, const std::string& registryKey,
	                         double lower, double upper, int fraction);
	
	void appendCombo(const char* name, StringArrayRange values, const IntImportCallback& importCallback, const IntExportCallback& exportCallback);
	void appendCombo(const char* name, int& data, StringArrayRange values);
	void appendSlider(const char* name, int& data, gboolean draw_value, const char* low, const char* high, double value, double lower, double upper, double step_increment, double page_increment, double page_size);
	void appendRadio(const char* name, StringArrayRange names, const IntImportCallback& importCallback, const IntExportCallback& exportCallback);
	void appendRadio(const char* name, int& data, StringArrayRange names);
	void appendRadioIcons(const char* name, StringArrayRange icons, const IntImportCallback& importCallback, const IntExportCallback& exportCallback);
	void appendRadioIcons(const char* name, int& data, StringArrayRange icons);
	GtkWidget* appendEntry(const char* name, const IntImportCallback& importCallback, const IntExportCallback& exportCallback);
	GtkWidget* appendEntry(const char* name, int& data);
	GtkWidget* appendEntry(const char* name, const SizeImportCallback& importCallback, const SizeExportCallback& exportCallback);
	GtkWidget* appendEntry(const char* name, std::size_t& data);
	GtkWidget* appendEntry(const char* name, const FloatImportCallback& importCallback, const FloatExportCallback& exportCallback);
	GtkWidget* appendEntry(const char* name, float& data);
	GtkWidget* appendPathEntry(const char* name, bool browse_directory, const StringImportCallback& importCallback, const StringExportCallback& exportCallback);
	GtkWidget* appendPathEntry(const char* name, CopiedString& data, bool directory);
	GtkWidget* appendSpinner(const char* name, int& data, double value, double lower, double upper);
	GtkWidget* appendSpinner(const char* name, double value, double lower, double upper, const IntImportCallback& importCallback, const IntExportCallback& exportCallback);
	GtkWidget* appendSpinner(const char* name, double value, double lower, double upper, const FloatImportCallback& importCallback, const FloatExportCallback& exportCallback);
	
	/** greebo: Performs a recursive lookup of the given path
	 * 			and creates any items that do not exist.
	 * 
	 * @returns: the shared_ptr to the PrefPage, can be empty on error.
	 */
	PrefPagePtr createOrFindPage(const std::string& path);
};


typedef Callback1<PrefPage*> PreferencesPageCallback;

typedef Callback1<PreferenceGroup&> PreferenceGroupCallback;

void PreferencesDialog_addInterfacePreferences(const PreferencesPageCallback& callback);
void PreferencesDialog_addInterfacePage(const PreferenceGroupCallback& callback);
void PreferencesDialog_addDisplayPreferences(const PreferencesPageCallback& callback);
void PreferencesDialog_addDisplayPage(const PreferenceGroupCallback& callback);
void PreferencesDialog_addSettingsPreferences(const PreferencesPageCallback& callback);
void PreferencesDialog_addSettingsPage(const PreferenceGroupCallback& callback);

void PreferencesDialog_restartRequired(const char* staticName);

template<typename Value>
class LatchedValue
{
public:
  Value m_value;
  Value m_latched;
  const char* m_description;

  LatchedValue(Value value, const char* description) : m_latched(value), m_description(description)
  {
  }
  void useLatched()
  {
    m_value = m_latched;
  }
  void import(Value value)
  {
    m_latched = value;
    if(m_latched != m_value)
    {
      PreferencesDialog_restartRequired(m_description);
    }
  }
};

typedef LatchedValue<bool> LatchedBool;
typedef MemberCaller1<LatchedBool, bool, &LatchedBool::import> LatchedBoolImportCaller;

typedef LatchedValue<int> LatchedInt;
typedef MemberCaller1<LatchedInt, int, &LatchedInt::import> LatchedIntImportCaller;

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTreeStore GtkTreeStore;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeSelection GtkTreeSelection;
class PrefPage;
class StringOutputStream;
class PreferenceTreeGroup;

class PrefsDlg 
{
	// The dialo window
	GtkWidget* _dialog;
	
	// The dialog outermost vbox
	GtkWidget* _overallVBox;
	
	GtkTreeStore* _prefTree;
	GtkTreeView* _treeView;
	GtkTreeSelection* _selection;
	GtkWidget* _notebook;
	
	PrefPagePtr _root;
	
	// Helper class to pump/extract values to/from the Registry
	gtkutil::RegistryConnector _registryConnector;
	
	// The helper class memorising the size/position
	gtkutil::WindowPosition _windowPosition;
	
	// Stays false until the main window is created, 
	// which happens in toggleWindow() first (the mainframe doesn't exist earlier)
	bool _packed;
	
	// True if the dialog is in modal mode
	bool _isModal;
	
	std::string _requestedPage;
	
public:
	PrefsDlg();

	// Retrieve a reference to the static instance of this dialog
	static PrefsDlg& Instance();

	/** greebo: Toggles the window visibility
	 */
	static void toggle();
	
	/** greebo: Makes sure that the dialog is visible.
	 * 			(does nothing if the dialog is already on screen)
	 */
	static void showModal(const std::string& path = "");

	GtkWidget *m_notebook;

	std::string _globalPrefPath;

	/*!
	path to per-game settings
	used for various game dependant storage
	win32: GameToolsPath
	linux: ~/.radiant/[version]/[gamename]/
	*/
	std::string m_rc_path;

	/*!
	holds per-game settings
	m_rc_path+"local.pref"
	\todo FIXME at some point this should become XML property bag code too
	*/
	std::string m_inipath;

	// initialize the above paths
	void Init();
	
	/** greebo: Looks up the page for the path and creates it
	 * 			if necessary.
	 */
	PrefPagePtr createOrFindPage(const std::string& path);
	
	/** greebo: A safe shutdown request that saves the window information
	 * 			to the registry.
	 */
	void shutdown();
	
	/** greebo: Returns TRUE if the dialog is visible.
	 */
	bool isVisible() const;
	
	/** greebo: Displays the page with the specified path.
	 * 
	 * @path: a string like "Settings/Patches"
	 */
	void showPage(const std::string& path);
	
private:
	/** greebo: This creates the actual window widget (all the other
	 * 			are created by populateWindow() during construction).
	 * 			This is done at a later point, because the MainFrame
	 * 			is usually not yet existing at construction time.
	 */
	void initDialog();

	/** greebo: Saves the preferences and hides the dialog 
	 */
	void save();
	
	/** greebo: Closes the dialog without writing the settings to the Registry.
	 */
	void cancel();
	
	/** greebo: Helper function that selects the current notebook page
	 * 			by using the GtkTreeSelection* object 
	 */
	void selectPage();

	/** greebo: Updates the tree store according to the PrefPage structure
	 */
	void updateTreeStore();

	/** greebo: Creates the widgets of this dialog
	 */
	void populateWindow();

	/** greebo: Toggles the visibility of this instance.
	 * 
	 * @modal: set this to TRUE to create a modal window
	 */
	void toggleWindow(bool modal = false);

	// greebo: calls the constructors to add the preference elements
	void callConstructors(PreferenceTreeGroup& preferenceGroup);
	
	// Gets called on page selection
	static void onPrefPageSelect(GtkTreeSelection* treeselection, PrefsDlg* self);
	static void onSave(GtkWidget* button, PrefsDlg* self);
	static void onCancel(GtkWidget* button, PrefsDlg* self); 
};

struct preferences_globals_t
{
    // disabled all INI / registry read write .. used when shutting down after registry cleanup
  bool disable_ini;
  preferences_globals_t() : disable_ini(false)
  {
  }
};
extern preferences_globals_t g_preferences_globals;

PreferenceSystem& GetPreferenceSystem();

typedef struct _GtkWindow GtkWindow;
void PreferencesDialog_constructWindow(GtkWindow* main_window);
void PreferencesDialog_destroyWindow();

void PreferencesDialog_showDialog();

void Preferences_Init();

void Preferences_Load();
void Preferences_Save();
class PreferenceDictionary;
bool Preferences_Load(PreferenceDictionary& preferences, const char* filename);
bool Preferences_Save(PreferenceDictionary& preferences, const char* filename);
bool Preferences_Save_Safe(PreferenceDictionary& preferences, const char* filename);

void Preferences_Reset();


#endif
