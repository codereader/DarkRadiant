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

#include <time.h>
#include <gtk/gtktogglebutton.h>

#include "iregistry.h"
#include "environment.h"
#include "os/file.h"
#include "string/string.h"
#include "ui/prefdialog/PrefDialog.h"

#include <boost/algorithm/string/replace.hpp>

void resetPreferences() {
	// Get the filename
	const std::string userSettingsFile = 
		GlobalRegistry().get(RKEY_SETTINGS_PATH) + "user.xml";
	
	// If the file exists, remove it and tell the registry to 
	// skip saving the user.xml file on shutdown
	if (file_exists(userSettingsFile.c_str())) {
		time_t timeStamp;
		time(&timeStamp);
		std::tm* timeStruc = localtime(&timeStamp); 
		
		std::string suffix = "." + intToStr(timeStruc->tm_mon+1) + "-";
		suffix += intToStr(timeStruc->tm_mday) + "_";
		suffix += intToStr(timeStruc->tm_hour) + "-";
		suffix += intToStr(timeStruc->tm_min) + "-";
		suffix += intToStr(timeStruc->tm_sec) + ".xml";
		
		std::string moveTarget = userSettingsFile;
		boost::algorithm::replace_all(moveTarget, ".xml", suffix);
		
		file_move(userSettingsFile.c_str(), moveTarget.c_str());
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

	PreferenceSystemAPI() {
		m_preferencesystem = &GetPreferenceSystem();
	}
	PreferenceSystem* getTable() {
		return m_preferencesystem;
	}
};

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

typedef SingletonModule<PreferenceSystemAPI> PreferenceSystemModule;
typedef Static<PreferenceSystemModule> StaticPreferenceSystemModule;
StaticRegisterModule staticRegisterPreferenceSystem(StaticPreferenceSystemModule::instance());
