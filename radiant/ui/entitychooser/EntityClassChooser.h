#ifndef ENTITYCLASSCHOOSER_H_
#define ENTITYCLASSCHOOSER_H_

#include "iradiant.h"
#include "ieclass.h"
#include "imodelpreview.h"

#include "gtkutil/window/BlockingTransientWindow.h"

#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>

namespace Gtk
{
	class TreeView;
	class TextView;
	class Button;
}

namespace ui
{

class EntityClassChooser;
typedef boost::shared_ptr<EntityClassChooser> EntityClassChooserPtr;

/** 
 * Dialog window displaying a tree of Entity Classes, allowing the selection
 * of a class to create at the current location. 
 */
class EntityClassChooser :
	public gtkutil::BlockingTransientWindow,
	public RadiantEventListener,
	public IEntityClassManager::Observer
{
public:
	// Treemodel definition
	struct TreeColumns : 
		public Gtk::TreeModel::ColumnRecord
	{
		TreeColumns() { add(name); add(icon); add(isFolder); }

		Gtk::TreeModelColumn<std::string> name;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
		Gtk::TreeModelColumn<bool> isFolder;
	};

	enum Result
	{
		RESULT_CANCELLED,
		RESULT_OK
	};

private:
	TreeColumns _columns;

	// Tree model holding the classnames, and the corresponding treeview
	Glib::RefPtr<Gtk::TreeStore> _treeStore;
	Gtk::TreeView* _treeView;
	
	// GtkTreeSelection holding the currently-selected classname
	Glib::RefPtr<Gtk::TreeSelection> _selection;

	// Usage information textview
	Gtk::TextView* _usageTextView;

	// OK button. Needs to be a member since we enable/disable it in the
	// selectionchanged callback.
	Gtk::Button* _okButton;
	
	// Last selected classname
	std::string _selectedName;

	// Model preview widget
	IModelPreviewPtr _modelPreview;

	Result _result;

private:
	// Constructor. Creates the GTK widgets.
	EntityClassChooser();

	/* Widget construction helpers */
	
	Gtk::Widget& createTreeView();
	Gtk::Widget& createUsagePanel();
	Gtk::Widget& createButtonPanel();

	// Update the usage panel with information from the provided entityclass
	void updateUsageInfo(const std::string& eclass);

	// Updates the member variables based on the current tree selection
	void updateSelection();

	// Button callbacks
	void callbackCancel();
	void callbackOK();

	// Check when the selection changes, disable the add button if there
	// is nothing selected.
	void callbackSelectionChanged();

	// This is where the static shared_ptr of the singleton instance is held.
	static EntityClassChooserPtr& InstancePtr();

	// Loads or reloads the entity class tree
	void loadEntityClasses();

protected:
	// Override TransientWindow::_onDeleteEvent
	void _onDeleteEvent();

	// Override BlockingTransientWindow::_postShow()
	void _postShow();

	// Override BlockingTransientWindow::_postHide()
	void _postHide();

public:
	// Public accessor to the singleton instance
	static EntityClassChooser& Instance();

	// Returns the Result (OK, CANCELLED)
	Result getResult();

	// Sets the tree selection to the given entity class
	void setSelectedEntityClass(const std::string& eclass);

	// Sets the tree selection to the given entity class
	const std::string& getSelectedEntityClass() const;
	
	/** 
	 * Convenience function:
	 * Display the dialog and block awaiting the selection of an entity class,
	 * which is returned to the caller. If the dialog is cancelled or no
	 * selection is made, and empty string will be returned.
	 */
	static std::string chooseEntityClass();

	// RadiantEventListener implementation
	void onRadiantShutdown();

	// EntityClassManager::Observer impl.
	void onEClassReload();
};

} // namespace ui

#endif /*ENTITYCLASSCHOOSER_H_*/
