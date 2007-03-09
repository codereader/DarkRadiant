#ifndef TRANSFORMDIALOG_H_
#define TRANSFORMDIALOG_H_

#include <string>
#include <map>
#include "iselection.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/RegistryConnector.h"
namespace gtkutil { class ControlButton; }
#include <boost/shared_ptr.hpp>

// Forward Declarations
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkEditable GtkEditable;
typedef struct _GtkTable GtkTable;

/* greebo: The dialog providing the Free Transform functionality.
 * 
 */
namespace ui {

class TransformDialog :
	public SelectionSystem::Observer
{
	// The actual dialog widget
	GtkWidget* _dialog;
	
	// The overall vbox (for global sensitivity toggle)
	GtkWidget* _dialogVBox;
	
	typedef boost::shared_ptr<gtkutil::ControlButton> ControlButtonPtr;
	
	// The entry fields
	struct EntryRow {
		GtkWidget* hbox;
		GtkWidget* label;
		GtkWidget* step;
		GtkWidget* stepLabel;
		ControlButtonPtr smaller;
		ControlButtonPtr larger;
	};
	typedef std::map<std::string, EntryRow> EntryRowMap;
	EntryRowMap _entries;
	
	GtkWidget* _rotateLabel;
	GtkWidget* _scaleLabel;
	
	// The reference to the SelectionInfo (number of patches, etc.)
	const SelectionInfo& _selectionInfo;
	
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;
	
	// The helper class that syncs the registry to/from widgets
	gtkutil::RegistryConnector _connector;

public:
	// Constructor
	TransformDialog();
	
	/** greebo: Contains the static instance of this dialog.
	 * Constructs the instance and calls toggle() when invoked.
	 */
	static TransformDialog& Instance();
	
	/** greebo: This toggles the visibility of the dialog.
	 * It is constructed only once and never destructed during runtime.
	 */
	void toggleDialog();
	
	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating of the
	 * widget sensitivity.
	 */
	void selectionChanged(scene::Instance& instance);

	/** greebo: Safely disconnects this dialog from all systems 
	 * 			(EventManager) also saves the window state to the registry.
	 */
	void shutdown();
	
	/** greebo: The command target compatible with FreeCaller<> to connect
	 * 			this method to the EventManager.
	 */
	static void toggle();
	
private:
	/** greebo: Updates the sensitivity of the widgets according to 
	 * 			the current selection.
	 */
	void update();

	/** greebo: Saves the step values to the registry
	 */
	void saveToRegistry();

	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	/** greebo: Helper method that creates a single row
	 */
	EntryRow createEntryRow(const std::string& label, GtkTable* table, int row); 
	
	// Callbacks to catch the scale/rotation button clicks
	static void onClickSmaller(GtkWidget* button, EntryRow* row);
	static void onClickLarger(GtkWidget* button, EntryRow* row);
	
	// The callback ensuring that the step changes are written to the registry
	static void onStepChanged(GtkEditable* editable, TransformDialog* self);
	
	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, TransformDialog* self);

}; // class TransformDialog

} // namespace ui


#endif /*TRANSFORMDIALOG_H_*/
