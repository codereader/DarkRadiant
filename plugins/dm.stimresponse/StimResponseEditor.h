#ifndef SREDITOR_H_
#define SREDITOR_H_

#include "ientity.h"
#include "gtkutil/WindowPosition.h"

#include "StimTypes.h"
#include "SREntity.h"
#include "StimEditor.h"
#include "ResponseEditor.h"

// Forward declarations
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkToggleButton GtkToggleButton;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GtkComboBox GtkComboBox;
typedef struct _GtkEditable GtkEditable;
typedef struct _GtkNotebook GtkNotebook;
typedef struct _GtkCellRendererText GtkCellRendererText;

namespace ui {

class StimResponseEditor
{
	// The window widget
	GtkWidget* _dialog;
	
	// The overall dialog vbox (used to quickly disable the whole dialog)
	GtkWidget* _dialogVBox;
	
	GtkNotebook* _notebook;
	int _stimPageNum;
	int _responsePageNum;
	
	// The close button to toggle the view
	GtkWidget* _closeButton;
	
	// The treeview with the entity's stims/responses
	GtkWidget* _entitySRView;
	GtkTreeSelection* _entitySRSelection;
	
	struct SRPropertyWidgets {
		GtkWidget* vbox;
		GtkWidget* typeList;
		GtkWidget* stimButton;
		GtkWidget* respButton;
		GtkWidget* active;
		GtkWidget* useBounds;
		GtkWidget* radiusToggle;
		GtkWidget* radiusEntry;
		GtkWidget* timeIntToggle;
		GtkWidget* timeIntEntry;
		GtkWidget* timeUnitLabel;
		GtkWidget* magnToggle;
		GtkWidget* magnEntry;
		GtkWidget* falloffToggle;
		GtkWidget* falloffEntry;
		GtkWidget* timerTypeToggle;
		GtkWidget* addMenuItem;
		GtkWidget* deleteMenuItem;
	} _srWidgets;
	
	struct EffectWidgets {
		GtkWidget* view;
		GtkTreeSelection* selection;
		GtkWidget* deleteMenuItem;
		GtkWidget* addMenuItem;
		GtkWidget* upMenuItem;
		GtkWidget* downMenuItem;
	} _effectWidgets;
	
	// Context menus for list views
	GtkWidget* _stimListContextMenu;
	GtkWidget* _effectListContextMenu;
	
	// The "extended" entity object managing the stims
	SREntityPtr _srEntity;
	
	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;
	
	// The entity we're editing
	Entity* _entity;
	
	// The helper class managing the stims
	StimTypes _stimTypes;
	
	// To allow updating the widgets without firing callbacks
	bool _updatesDisabled;
	
	// The helper classes for editing the stims/responses
	StimEditor _stimEditor;
	ResponseEditor _responseEditor;

public:
	StimResponseEditor();
	
	/** greebo: Contains the static instance of this dialog.
	 */
	static StimResponseEditor& Instance();
	
	// Command target to toggle the dialog
	static void toggle();
	
	/** greebo: Some sort of "soft" destructor that de-registers
	 * this class from the SelectionSystem, saves the window state, etc.
	 */
	void shutdown();
	
	/** greebo: This updates the widget sensitivity and loads
	 * 			the data into them.
	 */
	void update();
	
private:

	/** greebo: Selects the effect with the given id in the treeview
	 */
	void selectEffectIndex(const unsigned int index);

	/** greebo: Updates the context menu item sensitivity
	 */
	void updateEffectContextMenu();

	/** greebo: Edits the currently selected effect 
	 */
	void editEffect();
	
	/** greebo: Saves the current working set to the entity
	 */
	void save();

	/** greebo: Adds a new response effect to the list.
	 */
	void addEffect();

	/** greebo: Removes the currently selected response effect
	 */
	void removeEffect();
	
	/** greebo: Moves the selected effect up or down (i.e. increasing
	 * 			or decreasing its index).
	 * 
	 * @direction: +1 for moving it down (increasing the index)
	 * 			   -1 for moving it up (decreasing the index)
	 */
	void moveEffect(int direction);

	/** greebo: Removes the currently selected stim/response object
	 */
	void removeStimResponse();

	/** greebo: Adds a new StimResponse object, the index and the internal
	 * 			id are auto-incremented. The ListStore is refreshed. 
	 */
	void addStimResponse();

	/** greebo: Returns the ID of the currently selected stim
	 * 		
	 * @returns: the id (number) of the selected stim or -1 on failure 
	 */
	int getIdFromSelection();
	
	/** greebo: Returns the ID of the currently selected response effect
	 * 		
	 * @returns: the index of the selected effect or -1 on failure 
	 */
	int getEffectIdFromSelection();

	/** greebo: Tries to set the <key> of the currently selected 
	 * 			StimResponse to <value>
	 * 			The request is refused for inherited StimResponses.
	 */
	void setProperty(const std::string& key, const std::string& value);

	/** greebo: Updates the SR widget group according to the list selection.
	 */
	void updateSRWidgets();

	/* WIDGET POPULATION */
	void populateWindow(); 			// Main window
	GtkWidget* createSRWidgets(); 	// Stim/Response widget group
	GtkWidget* createEffectWidgets(); // Response effect list
	GtkWidget* createButtons(); 	// Dialog buttons
	void createContextMenus();		// Popup menus
	
	/** greebo: Updates the sensitivity of the "Add yyy" buttons
	 */
	void updateAddButton();

	/** greebo: Checks the selection for a single entity.
	 */
	void rescanSelection();

	/** greebo: This toggles the visibility of the surface dialog.
	 * The dialog is constructed only once and never destructed 
	 * during runtime.
	 */
	void toggleWindow();
	
	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, StimResponseEditor* self);

	// Callback for Stim/Response and effect selection changes
	static void onSRSelectionChange(GtkTreeSelection* treeView, StimResponseEditor* self);
	static void onEffectSelectionChange(GtkTreeSelection* treeView, StimResponseEditor* self);
	
	static void onClassChange(GtkToggleButton* toggleButton, StimResponseEditor* self);
	static void onTypeSelect(GtkComboBox* widget, StimResponseEditor* self);
	
	// StimType selection change
	static void onStimTypeChange(GtkComboBox* widget, StimResponseEditor* self);
	
	// Toggle buttons
	static void onActiveToggle(GtkToggleButton* toggleButton, StimResponseEditor* self);
	static void onBoundsToggle(GtkToggleButton* toggleButton, StimResponseEditor* self);
	static void onRadiusToggle(GtkToggleButton* toggleButton, StimResponseEditor* self);
	static void onMagnitudeToggle(GtkToggleButton* toggleButton, StimResponseEditor* self);
	static void onFalloffToggle(GtkToggleButton* toggleButton, StimResponseEditor* self);
	static void onTimeIntervalToggle(GtkToggleButton* toggleButton, StimResponseEditor* self);
	static void onTimerTypeToggle(GtkToggleButton* toggleButton, StimResponseEditor* self);

	// Entry text changes
	static void onMagnitudeChanged(GtkEditable* editable, StimResponseEditor* self);
	static void onFalloffChanged(GtkEditable* editable, StimResponseEditor* self);
	static void onTimeIntervalChanged(GtkEditable* editable, StimResponseEditor* self);
	static void onRadiusChanged(GtkEditable* editable, StimResponseEditor* self);

	// Button callbacks
	static void onSave(GtkWidget* button, StimResponseEditor* self);
	static void onClose(GtkWidget* button, StimResponseEditor* self);
	static void onRevert(GtkWidget* button, StimResponseEditor* self);

	// The keypress handler for catching the keys in the treeview
	static gboolean onTreeViewKeyPress(
		GtkTreeView* view,GdkEventKey* event, StimResponseEditor* self);
	static gboolean onWindowKeyPress(
		GtkWidget* dialog, GdkEventKey* event, StimResponseEditor* self);
	
	static gboolean onTreeViewButtonPress(
		GtkTreeView* view, GdkEventButton* ev, StimResponseEditor* self);
	
	// Tree view button click events (for popup menus)
	static gboolean onTreeViewButtonRelease(
		GtkTreeView* view, GdkEventButton* ev, StimResponseEditor* self);
	static void _onContextMenuAdd(GtkWidget*, StimResponseEditor*);
	static void _onContextMenuDelete(GtkWidget*, StimResponseEditor*);
	static void _onContextMenuEffectUp(GtkWidget*, StimResponseEditor*);
	static void _onContextMenuEffectDown(GtkWidget*, StimResponseEditor*);

}; // class StimResponseEditor

} // namespace ui

#endif /*SREDITOR_H_*/
