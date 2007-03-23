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
#include "dialog.h"
#include <list>
#include <map>
#include "settings/PrefPage.h"

	namespace {
		const std::string RKEY_SKIP_REGISTRY_SAVE = "user/skipRegistrySaveOnShutdown";
	}

void Widget_connectToggleDependency(GtkWidget* self, GtkWidget* toggleButton);

typedef struct _GtkTreeStore GtkTreeStore;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeSelection GtkTreeSelection;

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

	/** greebo: The command target to show the Game settings preferences.
	 */
	static void showProjectSettings();

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

	// Gets called on page selection
	static void onPrefPageSelect(GtkTreeSelection* treeselection, PrefsDlg* self);
	static void onSave(GtkWidget* button, PrefsDlg* self);
	static void onCancel(GtkWidget* button, PrefsDlg* self);
	
	// Delete event (fired when the "X" close button is clicked)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, PrefsDlg* self);
};

PreferenceSystem& GetPreferenceSystem();

/** greebo: Deletes the user.xml file from the settings folder
 */
void resetPreferences();

#endif
