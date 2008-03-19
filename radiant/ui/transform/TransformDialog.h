#ifndef TRANSFORMDIALOG_H_
#define TRANSFORMDIALOG_H_

#include <string>
#include <map>
#include "iselection.h"
#include "iradiant.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/RegistryConnector.h"
namespace gtkutil { class ControlButton; }

// Forward Declarations
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkEditable GtkEditable;
typedef struct _GtkTable GtkTable;

/* greebo: The dialog providing the Free Transform functionality.
 * 	
 * The scale and rotation steps are loaded and written from/to the registry
 * using the RegistryConnector helper class.
 * 
 * The Dialog gets notified upon selection change and updates the widget
 * sensitivity accordingly.
 * 
 * If any entity is part of the selection, the scale widgets get disabled.
 */
namespace ui {

class TransformDialog;
typedef boost::shared_ptr<TransformDialog> TransformDialogPtr;

class TransformDialog 
: public gtkutil::PersistentTransientWindow,
  public SelectionSystem::Observer,
  public RadiantEventListener
{
	// The overall vbox (for global sensitivity toggle)
	GtkWidget* _dialogVBox;
	
	typedef boost::shared_ptr<gtkutil::ControlButton> ControlButtonPtr;
	
	// The entry fields
	struct EntryRow {
		bool isRotator;
		int axis; 
		int direction; // Direction (rotation only), is 1 by default
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
	GtkTable* _rotateTable;
	GtkWidget* _scaleLabel;
	GtkTable* _scaleTable;
	
	// The reference to the SelectionInfo (number of patches, etc.)
	const SelectionInfo& _selectionInfo;
	
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;
	
	// The helper class that syncs the registry to/from widgets
	gtkutil::RegistryConnector _connector;

private:
	
	// TransientWindow callbacks
	virtual void _preShow();
	virtual void _preHide();
	
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
	 * 
	 * @row: the row index of <table> where the widgets should be packed into.
	 * @isRotator: set to true if a rotator row is to be created.
	 * @axis: the axis this transformation is referring to.
	 */
	EntryRow createEntryRow(const std::string& label, GtkTable* table, 
							int row, bool isRotator, int axis);
	
	// Callbacks to catch the scale/rotation button clicks
	static void onClickSmaller(GtkWidget* button, EntryRow* row);
	static void onClickLarger(GtkWidget* button, EntryRow* row);
	
	// The callback ensuring that the step changes are written to the registry
	static void onStepChanged(GtkEditable* editable, TransformDialog* self);
	
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
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	/** greebo: Safely disconnects this dialog from all systems 
	 * 			(EventManager) also saves the window state to the registry.
	 */
	virtual void onRadiantShutdown();
	
	/** greebo: The command target compatible with FreeCaller<> to connect
	 * 			this method to the EventManager.
	 */
	static void toggle();

private:
	static TransformDialogPtr& InstancePtr();
	
}; // class TransformDialog

} // namespace ui


#endif /*TRANSFORMDIALOG_H_*/
