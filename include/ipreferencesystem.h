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

#include <list>
#include <vector>
#include "imodule.h"

// Forward declaration
typedef struct _GtkWidget GtkWidget;

// A list containing possible values for a combo box widgets
typedef std::list<std::string> ComboBoxValueList;
typedef std::vector<std::string> IconList;
typedef std::vector<std::string> IconDescriptionList;

/* greebo: This is the interface the preference page has to provide for adding
 * elements to the dialog page. */
class PreferencesPage
{
public:
	/** greebo: Allows to set a custom title of this page. The default title 
	 * 			upon construction is "guessed" by adding a " Settings" to 
	 * 			the page name, "Settings/Patch" gets assigned 
	 * 			a "Patch Settings" as default title.
	 * 			Use this method to change this to fit your needs. 
	 */
	virtual void setTitle(const std::string& title) = 0;

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
	
	/* greebo: Use this to add a series of radio buttons with icons and descriptions.
	 * The result will be stored under the given RegistryKey (with 0 referring to the first item) */		  
	virtual void appendRadioIcons(const std::string& name, const std::string& registryKey, 
								  const IconList& iconList, const IconDescriptionList& iconDescriptions) = 0;
	
	/* greebo: Appends an entry field with <name> as caption which is connected to the given registryKey
	 */
	virtual GtkWidget* appendEntry(const std::string& name, const std::string& registryKey) = 0;
	
	/* greebo: Appends an entry field with spinner buttons which retrieves its value from the given
	 * RegistryKey. The lower and upper values have to be passed as well.
	 */
	virtual GtkWidget* appendSpinner(const std::string& name, const std::string& registryKey, 
									 double lower, double upper, int fraction) = 0;
									
	// greebo: Adds a PathEntry to choose files or directories (depending on the given boolean)
	virtual GtkWidget* appendPathEntry(const std::string& name, 
									   const std::string& registryKey, 
									   bool browseDirectories) = 0;
									   
	// Appends a static label (to add some text to the preference page)
	virtual GtkWidget* appendLabel(const std::string& caption) = 0;
};
typedef boost::shared_ptr<PreferencesPage> PreferencesPagePtr;

const std::string MODULE_PREFERENCESYSTEM("PreferenceSystem");

class IPreferenceSystem :
	public RegisterableModule
{
public:
	/** greebo: Retrieves the page for the given path, for example:
	 * 
	 * 			"Settings/Patch Settings" 
	 * 			(spaces are ok, slashes are treated as delimiters, don't use them)
	 *  
	 * 			Use the PreferencesPage interface to add widgets 
	 * 			and connect them to the registry.
	 * 
	 * @path: The path to lookup
	 * 
	 * @returns: the PreferencesPage pointer.
	 */
	virtual PreferencesPagePtr getPage(const std::string& path) = 0; 
};

inline IPreferenceSystem& GlobalPreferenceSystem() {
	// Cache the reference locally
	static IPreferenceSystem& _prefSystem(
		*boost::static_pointer_cast<IPreferenceSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_PREFERENCESYSTEM)
		)
	);
	return _prefSystem;
}

#endif
