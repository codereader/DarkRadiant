#ifndef CLASSEDITOR_H_
#define CLASSEDITOR_H_

#include "SREntity.h"
#include "StimTypes.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GdkEventKey GdkEventKey;
typedef struct _GdkEventButton GdkEventButton;
typedef struct _GtkEditable GtkEditable;
typedef struct _GtkToggleButton GtkToggleButton;
typedef struct _GtkComboBox GtkComboBox;

namespace ui {

class ClassEditor
{
protected:
	typedef std::map<GtkEditable*, std::string> EntryMap;
	EntryMap _entryWidgets; 

	GtkWidget* _pageVBox;
	
	GtkWidget* _list;
	GtkTreeSelection* _selection;
	
	// The entity object we're editing
	SREntityPtr _entity;

	// Helper class (owned by StimResponseEditor)
	StimTypes& _stimTypes;
	
	// TRUE if the GTK callbacks should be disabled
	bool _updatesDisabled;
	
	// The combo box to select the stim/response type
	typedef struct TypeSelectorWidgets {
		GtkWidget* hbox;	// The
		GtkWidget* list;	// the combo box
		GtkWidget* label;	// The "Type:" label
	} TypeSelectorWidgets;
	
	TypeSelectorWidgets _type;
	TypeSelectorWidgets _addType;

public:
	/** greebo: Constructs the shared widgets, but does not pack them
	 */
	ClassEditor(StimTypes& stimTypes);
	
	/** greebo: Operator cast to widget to pack this page into
	 * 			a notebook tab or other parent widget.
	 */
	virtual operator GtkWidget*();
	
	/** greebo: Sets the new entity (is called by the subclasses)
	 */
	virtual void setEntity(SREntityPtr entity);

	/** greebo: Sets the given <key> of the current entity to <value>
	 */
	virtual void setProperty(const std::string& key, const std::string& value);
	
	/** greebo: Updates the widgets (must be implemented by the child classes) 
	 */
	virtual void update() = 0;

protected:
	/** greebo: Returns the name of the selected stim in the given combo box.
	 * 			The model behind that combo box has to be according to the
	 * 			one created by the StimTypes helper class.
	 */
	virtual std::string getStimTypeIdFromSelector(GtkComboBox* widget);

	/** greebo: Adds/removes a S/R from the main list
	 */
	virtual void addSR() = 0; 
	virtual void removeSR(GtkTreeView* view);

	/** greebo: Duplicates the currently selected S/R object
	 */
	void duplicateStimResponse();

	/** greebo: Returns the fabricated Stim Selector widget structure 
	 */
	TypeSelectorWidgets createStimTypeSelector();

	/** greebo: Gets called when a check box is toggled, this should
	 * 			update the contents of possible associated entry fields. 
	 */
	virtual void checkBoxToggled(GtkToggleButton* toggleButton) {
	}

	/** greebo: Gets called when an entry box changes, this can be
	 * 			overriden by the subclasses, if this is needed
	 */
	virtual void entryChanged(GtkEditable* editable);

	/** greebo: Returns the ID of the currently selected stim/response
	 * 		
	 * @returns: the id (number) of the selected stim or -1 on failure 
	 */
	int getIdFromSelection();

	/** greebo: Selects the given ID in the S/R list
	 */
	void selectId(int id);

	/** greebo: Gets called when the list selection changes
	 */
	virtual void selectionChanged() = 0;

	/** greebo: Opens the context menu. The treeview widget this event
	 * 			has been happening on gets passed so that the correct
	 * 			menu can be displayed (in the case of multiple possible treeviews).
	 */
	virtual void openContextMenu(GtkTreeView* view) = 0;

	// GTK Callback for Stim/Response selection changes
	static void onSRSelectionChange(GtkTreeSelection* treeView, ClassEditor* self);
	// The keypress handler for catching the keys in the treeview
	static gboolean onTreeViewKeyPress(GtkTreeView* view,GdkEventKey* event, ClassEditor* self);
	// Release-event opens the context menu for right clicks
	static gboolean onTreeViewButtonRelease(GtkTreeView* view, GdkEventButton* ev, ClassEditor* self);
	
	// Gets called if any of the entry widget contents get changed
	static void onEntryChanged(GtkEditable* editable, ClassEditor* self);
	// Gets called on check box toggles
	static void onCheckboxToggle(GtkToggleButton* toggleButton, ClassEditor* self);
	
	// Gets called on stim type selection change
	static void onStimTypeSelect(GtkComboBox* widget, ClassEditor* self);
	
	static void onAddSR(GtkWidget* button, ClassEditor* self);
	static void onRemoveSR(GtkWidget* button, ClassEditor* self);
	
	// Override/disable override menu items
	static void onContextMenuEnable(GtkWidget* w, ClassEditor* self);
	static void onContextMenuDisable(GtkWidget* w, ClassEditor* self);
	static void onContextMenuDuplicate(GtkWidget* w, ClassEditor* self);
};

} // namespace ui

#endif /*CLASSEDITOR_H_*/
