#pragma once

#include "iradiant.h"
#include "icommandsystem.h"
#include "gtkutil/dialog/DialogBase.h"
#include "PrefPage.h"

#include "gtkutil/TreeModel.h"
#include "gtkutil/TreeView.h"

class wxTreebook;

namespace ui
{

class PrefDialog;
typedef boost::shared_ptr<PrefDialog> PrefDialogPtr;

class PrefDialog
{
private:
	// The actual dialog instance
	wxutil::DialogBase* _dialog;

	/*struct PrefColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		PrefColumns() { add(name); add(pageWidget); }

		// The column with the caption (for lookups)
		Gtk::TreeModelColumn<Glib::ustring> name;

		// The pointer to the preference page
		Gtk::TreeModelColumn<Gtk::Widget*> pageWidget;
	};

	PrefColumns _treeColumns;*/

	wxTreebook* _notebook;

	// The root page
	PrefPagePtr _root;

public:
	PrefDialog();

	// Retrieve a reference to the static instance of this dialog
	static PrefDialog& Instance();

	/** greebo: Runs the modal dialog
	 */
	static void ShowDialog(const cmd::ArgumentList& args);

	/** greebo: Makes sure that the dialog is visible.
	 * 			(does nothing if the dialog is already on screen)
	 */
	static void ShowModal(const std::string& path = "");

	/** greebo: The command target to show the Game settings preferences.
	 */
	static void ShowProjectSettings(const cmd::ArgumentList& args);

	/** greebo: Looks up the page for the path and creates it
	 * 			if necessary.
	 */
	PrefPagePtr createOrFindPage(const std::string& path);

	// Reparent the preference dialog on startup
	void onRadiantStartup();

	/** greebo: A safe shutdown request that saves the window information
	 * 			to the registry.
	 */
	void onRadiantShutdown();
	
	/** greebo: Displays the page with the specified path.
	 *
	 * @path: a string like "Settings/Patches"
	 */
	void showPage(const std::string& path);

private:
	int doShowModal(const std::string& requestedPage);

	// This is where the static shared_ptr of the singleton instance is held.
	static PrefDialogPtr& InstancePtr();

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

	void createDialog(wxWindow* parent);

	/** greebo: Toggles the visibility of this instance.
	 *
	 * @modal: set this to TRUE to create a modal window
	 */
	void toggleWindow(bool modal = false);

	// Gets called on page selection
	void onPrefPageSelect();
	void onOK();
	void onCancel();
};

} // namespace ui
