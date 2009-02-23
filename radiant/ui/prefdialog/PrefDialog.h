#ifndef PREFDIALOG_H_
#define PREFDIALOG_H_

#include "iradiant.h"
#include "icommandsystem.h"
#include "gtkutil/RegistryConnector.h"
#include "gtkutil/WindowPosition.h"
#include "PrefPage.h"

typedef struct _GtkTreeStore GtkTreeStore;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeSelection GtkTreeSelection;

namespace ui {

class PrefDialog;
typedef boost::shared_ptr<PrefDialog> PrefDialogPtr;

class PrefDialog :
	public RadiantEventListener
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
	PrefDialog();

	// Retrieve a reference to the static instance of this dialog
	static PrefDialog& Instance();

	/** greebo: Toggles the window visibility
	 */
	static void toggle(const cmd::ArgumentList& args);
	
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
	 * 			to the registry. (RadiantEventListener implementation)
	 */
	virtual void onRadiantShutdown();
	
	/** greebo: Returns TRUE if the dialog is visible.
	 */
	bool isVisible() const;
	
	/** greebo: Displays the page with the specified path.
	 * 
	 * @path: a string like "Settings/Patches"
	 */
	void showPage(const std::string& path);
	
private:
	// This is where the static shared_ptr of the singleton instance is held.
	static PrefDialogPtr& InstancePtr();

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
	static void onPrefPageSelect(GtkTreeSelection* treeselection, PrefDialog* self);
	static void onOK(GtkWidget* button, PrefDialog* self);
	static void onCancel(GtkWidget* button, PrefDialog* self);
	
	// Delete event (fired when the "X" close button is clicked)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, PrefDialog* self);
};

} // namespace ui

#endif /*PREFDIALOG_H_*/
