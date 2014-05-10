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
	// The dialog window - due to some weird effects which I couldn't figure out
	// this top level window will be destroyed and re-created each time the dialog
	// is shown. The contents are staying the same and will be moved over each time.
	wxutil::DialogBase* _dialog;

	wxutil::TreeModel* _prefTree;
	wxutil::TreeView* _treeView;
	
	wxTreebook* _notebook;

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

	wxPanel* _mainPanel;

	// The root page
	PrefPagePtr _root;

	// Stays false until the main window is created,
	// which happens in toggleWindow() first (the mainframe doesn't exist earlier)
	bool _packed;

	// True if the dialog is in modal mode
	bool _isModal;

	std::string _requestedPage;

protected:
	// Virtual pre-destroy callback
	virtual void _preShow();

public:
	PrefDialog();

	// Retrieve a reference to the static instance of this dialog
	static PrefDialog& Instance();

	int ShowModal();

	/** greebo: Runs the modal dialog
	 */
	static void ShowDialog(const cmd::ArgumentList& args);

	/** greebo: Makes sure that the dialog is visible.
	 * 			(does nothing if the dialog is already on screen)
	 */
	static void showModal(const std::string& path = "");

	/** greebo: The command target to show the Game settings preferences.
	 */
	static void showProjectSettings(const cmd::ArgumentList& args);

	/** greebo: Looks up the page for the path and creates it
	 * 			if necessary.
	 */
	PrefPagePtr createOrFindPage(const std::string& path);

	/** greebo: A safe shutdown request that saves the window information
	 * 			to the registry.
	 */
	void onRadiantShutdown();

	/** greebo: Displays the page with the specified path.
	 *
	 * @path: a string like "Settings/Patches"
	 */
	void showPage(const std::string& path);

protected:
	// Override the TransientWindow delete event
	// (Fired when the "X" close button is clicked)
	virtual void _onDeleteEvent();

private:
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

	/** greebo: Creates the widgets of this dialog
	 */
	void populateWindow();

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
