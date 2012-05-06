#pragma once

#include "iradiant.h"
#include "ieclass.h"

#include "gtkutil/GladeWidgetHolder.h"
#include "gtkutil/preview/ModelPreview.h"
#include "gtkutil/window/BlockingTransientWindow.h"

#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>

#include <boost/scoped_ptr.hpp>

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
    private gtkutil::GladeWidgetHolder
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

	// Tree model holding the classnames
	Glib::RefPtr<Gtk::TreeStore> _treeStore;

    // Delegated object for loading entity classes in a separate thread
    class ThreadedEntityClassLoader;
    boost::scoped_ptr<ThreadedEntityClassLoader> _eclassLoader; // PIMPL idiom

	// GtkTreeSelection holding the currently-selected classname
	Glib::RefPtr<Gtk::TreeSelection> _selection;

	// Last selected classname
	std::string _selectedName;

    // Class we should select when the treemodel is populated
    std::string _classToHighlight;

	// Model preview widget
    gtkutil::ModelPreviewPtr _modelPreview;

	Result _result;

private:
	// Constructor. Creates the GTK widgets.
	EntityClassChooser();

    Gtk::TreeView* treeView();
    void setTreeViewModel();
    void getEntityClassesFromLoader();
	void loadEntityClasses();

	/* Widget construction helpers */
	void setupTreeView();

	// Update the usage panel with information from the provided entityclass
	void updateUsageInfo(const std::string& eclass);

	// Updates the member variables based on the current tree selection
	void updateSelection();

	// Button callbacks
	void callbackCancel();
	void callbackOK();

	// This is where the static shared_ptr of the singleton instance is held.
	static EntityClassChooserPtr& InstancePtr();

protected:
	// Override TransientWindow::_onDeleteEvent
	void _onDeleteEvent();

	// Override BlockingTransientWindow::_postShow()
	void _postShow();

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

	void onRadiantShutdown();
};

} // namespace ui
