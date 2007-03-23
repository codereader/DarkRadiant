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

#include "stringio.h"

#include "ui/prefdialog/PrefDialog.h"

//const char* const PREFERENCES_VERSION = "1.0";

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

class PreferenceDictionary : 
	public PreferenceSystem
{
public:
	// Looks up a page for the given path and returns it to the client
	PreferencesPagePtr getPage(const std::string& path) {
		return ui::PrefDialog::Instance().createOrFindPage(path);
	}
};

PreferenceSystem& GetPreferenceSystem() {
	static PreferenceDictionary _preferenceSystem;
	return _preferenceSystem;
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
}*/
