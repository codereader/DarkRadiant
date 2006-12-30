/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

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

#if !defined(INCLUDED_PREFERENCESYSTEM_H)
#define INCLUDED_PREFERENCESYSTEM_H

#include <string>
#include <list>

#include "generic/constant.h"

// ----- Begin: For deprecated methods ----------------------------------
#include "generic/callbackfwd.h"

typedef Callback1<const char*> StringImportCallback;
typedef Callback1<const StringImportCallback&> StringExportCallback;
// ----- End: For deprecated methods ------------------------------------

// Forward declaration
class PreferenceGroup;
//class PreferencesPage;
typedef struct _GtkWidget GtkWidget;

// A list containing possible values for a combo box widgets
typedef std::list<std::string> ComboBoxValueList;

/* greebo: This is the interface the preference page has to provide for adding
 * elements to the dialog page. */
class PreferencesPage
{
public:
	// greebo: Use this to add a checkbox to the preference dialog that is connected to a registry value
	virtual GtkWidget* appendCheckBox(const std::string& name, const std::string& flag, const std::string& registryKey) = 0;
	
	/* greebo: This adds a horizontal slider to the internally referenced VBox and connects 
	 * it to the given registryKey. */
	virtual void appendSlider(const std::string& name, const std::string& registryKey, bool drawValue, 
							  double value, double lower, double upper, 
							  double step_increment, double page_increment, double page_size) = 0;
	
	/* greebo: Use this to add a dropdown selection box with the given list of strings as captions. The value
	 * stored in the registryKey is used to determine the currently selected combobox item */		  
	virtual void appendCombo(const std::string& name, const std::string& registryKey, const ComboBoxValueList& valueList) = 0;
	
	/* greebo: Appends an entry field with <name> as caption which is connected to the given registryKey
	 */
	virtual GtkWidget* appendEntry(const std::string& name, const std::string& registryKey) = 0;
};

/* greebo: A PreferenceGroup consists of several PreferencePages and provides a method to add one of these. */
class PreferenceGroup
{
public:
	virtual PreferencesPage* createPage(const std::string& treeName, const std::string& frameName) = 0;
};

/* greebo: Derive from the PreferenceConstructor class if your module wants to add preferences
 * to the preference dialog. The method constructPreferencePage() gets invoked by the 
 * preference system when the dialog is to be constructed. */
class PreferenceConstructor
{
public:
	/* greebo: This gets called by the preference system and is responsible for adding the
	 * according pages and elements to the preference dialog.*/
	virtual void constructPreferencePage(PreferenceGroup& group) = 0;
};

class PreferenceSystem
{
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "preferences");

	/* greebo: Use this to connect a preference constructor that gets invoked when the dialog is constructed.
	 * 
	 * @arguments: <constructor> is pointing to an implementation of the IPreferenceConstructor class that is responsible
	 * for adding all the preference elements (pages, checkboxes, etc.) to the dialog.*/
	virtual void addConstructor(PreferenceConstructor* constructor) = 0;
	
	// greebo: Deprecated, don't use this in newly written code
	virtual void registerPreference(const char* name, const StringImportCallback& importer, const StringExportCallback& exporter) = 0;  
};

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<PreferenceSystem> GlobalPreferenceSystemModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<PreferenceSystem> GlobalPreferenceSystemModuleRef;

inline PreferenceSystem& GlobalPreferenceSystem() {
	return GlobalPreferenceSystemModule::getTable();
}

#endif
