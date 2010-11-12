#ifndef COMPONENTSDIALOG_H_
#define COMPONENTSDIALOG_H_

#include "ce/ComponentEditor.h"
#include "DifficultyPanel.h"
#include "Objective.h"

#include <map>
#include <vector>
#include <string>

#include "gtkutil/Timer.h"
#include "gtkutil/window/BlockingTransientWindow.h"

#include <gtkmm/liststore.h>

namespace Gtk
{
	class Entry;
	class CheckButton;
	class ComboBox;
	class ComboBoxText;
	class Table;
	class Frame;
	class TreeView;
}

namespace objectives
{

/**
 * Dialog for displaying and editing the properties and components (conditions)
 * attached to a particular objective.
 */
class ComponentsDialog :
	public gtkutil::BlockingTransientWindow
{
private:
	// The objective we are editing
	Objective& _objective;

	struct ComponentListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ComponentListColumns() { add(index); add(description); }

		Gtk::TreeModelColumn<int> index;
		Gtk::TreeModelColumn<Glib::ustring> description;
	};

	// List store for the components
	ComponentListColumns _columns;
	Glib::RefPtr<Gtk::ListStore> _componentList;

	// Currently-active ComponentEditor (if any)
	ce::ComponentEditorPtr _componentEditor;

	// The widgets needed for editing the difficulty levels
	DifficultyPanel* _diffPanel;

	// Working set of components we're editing (get written to the objective
	// as soon as the "Save" button is pressed.
	Objective::ComponentMap _components;

	// TRUE while the widgets are populated to disable GTK callbacks
	bool _updateMutex;

	// A timer to periodically update the component list
	gtkutil::Timer _timer;

	Gtk::Entry* _objDescriptionEntry;
	Gtk::ComboBoxText* _objStateCombo;
	Gtk::Entry* _enablingObjs;
	Gtk::Entry* _successLogic;
	Gtk::Entry* _failureLogic;

	Gtk::Entry* _completionScript;
	Gtk::Entry* _failureScript;
	Gtk::Entry* _completionTarget;
	Gtk::Entry* _failureTarget;

	Gtk::CheckButton* _objMandatoryFlag;
	Gtk::CheckButton* _objIrreversibleFlag;
	Gtk::CheckButton* _objOngoingFlag;
	Gtk::CheckButton* _objVisibleFlag;

	Gtk::Table* _editPanel;
	Gtk::TreeView* _componentView;
	Gtk::ComboBox* _typeCombo;

	Gtk::CheckButton* _stateFlag;
	Gtk::CheckButton* _irreversibleFlag;
	Gtk::CheckButton* _invertedFlag;
	Gtk::CheckButton* _playerResponsibleFlag;

	Gtk::Frame* _compEditorPanel;

private:
	// Construction helpers
	Gtk::Widget& createObjectiveEditPanel();
	Gtk::Widget& createObjectiveFlagsTable();
	Gtk::Widget& createListView();
	Gtk::Widget& createEditPanel();
	Gtk::Widget& createComponentEditorPanel();
	Gtk::Widget& createButtons();

	// Populate the list of components from the Objective's component map
	void populateComponents();

	// Updates the list store contents without removing any components
	void updateComponents();

	// Populate the edit panel widgets with the specified component number
	void populateEditPanel(int index);

	// Populate the objective properties
	void populateObjectiveEditPanel();

	// Get the index of the selected Component, or -1 if there is no selection
	int getSelectedIndex();

    // Change the active ComponentEditor
    void changeComponentEditor(Component& compToEdit);

    // If there is a ComponentEditor, write its contents to the Component
    void checkWriteComponent();

	// Writes the data from the widgets to the data structures
	void save();

	// gtkmm callbacks
	void _onOK();
	void _onCancel();
	void _onDelete();
	void _onSelectionChanged();

	void _onCompToggleChanged(Gtk::CheckButton* button); // button is manually bound

	void _onAddComponent();
	void _onDeleteComponent();

	void _onTypeChanged();
    void _onApplyComponentChanges();

	static gboolean _onIntervalReached(gpointer data);

public:

	/**
	 * Constructor creates widgets.
	 *
	 * @param parent
	 * The parent window for which this dialog should be a transient.
	 *
	 * @param objective
	 * The Objective object for which conditions should be displayed and edited.
	 */
	ComponentsDialog(const Glib::RefPtr<Gtk::Window>& parent, Objective& objective);

	// Destructor performs cleanup
	~ComponentsDialog();
};

} // namespace

#endif /*COMPONENTSDIALOG_H_*/
