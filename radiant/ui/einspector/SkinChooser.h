#pragma once

#include "modelskin.h"

#include "iradiant.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/preview/ModelPreview.h"
#include <string>

#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>

namespace Gtk
{
	class TreeView;
}

namespace ui
{

class SkinChooser;
typedef boost::shared_ptr<SkinChooser> SkinChooserPtr;

/** Dialog to allow selection of skins for a model entity. Skins are grouped
 * into two toplevel categories - matching skins which are associated with the
 * model, and all skins available.
 */
class SkinChooser :
	public gtkutil::BlockingTransientWindow
{
public:
	// Treemodel definition
	struct TreeColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		TreeColumns()
		{
			add(displayName);
			add(fullName);
			add(icon);
			add(isFolder);
		}

		Gtk::TreeModelColumn<Glib::ustring> displayName;
		Gtk::TreeModelColumn<Glib::ustring> fullName;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
		Gtk::TreeModelColumn<bool> isFolder;
	};

private:
	TreeColumns _columns;

	// Tree store, view and selection
	Glib::RefPtr<Gtk::TreeStore> _treeStore;
	Gtk::TreeView* _treeView;
	Glib::RefPtr<Gtk::TreeSelection> _selection;

	// The model name to use for skin matching
	std::string _model;

	// The last skin selected, and the original (previous) skin
	std::string _lastSkin;
	std::string _prevSkin;

	// Model preview widget
    gtkutil::ModelPreviewPtr _preview;

private:
	// Constructor creates GTK widgets
	SkinChooser();

	// Widget creation functions
	Gtk::Widget& createTreeView(int width);
	Gtk::Widget& createPreview(int size);
	Gtk::Widget& createButtons();

	// Show the dialog and block until selection is made
	std::string showAndBlock(const std::string& model, const std::string& prev);

	// Populate the tree with skins
	void populateSkins();

	// GTK callbacks
	void _onOK();
	void _onCancel();
	void _onSelChanged();

	// Contains the static instance
	static SkinChooser& Instance();

	// This is where the static shared_ptr of the singleton instance is held.
	static SkinChooserPtr& InstancePtr();

	// Retrieve the currently selected skin
	std::string getSelectedSkin();

protected:
	// Override TransientWindow::_onDeleteEvent
	void _onDeleteEvent();

	// Override BlockingTransientWindow::_postShow()
	void _postShow();

public:

	void onRadiantShutdown();

	/** Display the dialog and return the skin chosen by the user, or an empty
	 * string if no selection was made. This static method enters are recursive
	 * GTK main loop during skin selection.
	 *
	 * @param model
	 * The full VFS path of the model for which matching skins should be found.
	 *
	 * @param prevSkin
	 * The current skin set on the model, so that the original can be returned
	 * if the dialog is cancelled.
	 */
	static std::string chooseSkin(const std::string& model,
								  const std::string& prevSkin);
};

}
