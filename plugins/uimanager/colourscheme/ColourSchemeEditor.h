#ifndef COLOURSCHEMEEDITOR_H_
#define COLOURSCHEMEEDITOR_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include <string>
#include "icommandsystem.h"
#include "ColourScheme.h"

#include <gtkmm/liststore.h>
#include <gdk/gdkevents.h>

namespace Gtk
{
	class TreeView;
	class HBox;
	class Button;
	class Frame;
	class ColorButton;
}

namespace ui
{

class ColourSchemeEditor :
	public gtkutil::BlockingTransientWindow
{
private:
	// The treeview and its selection pointer
	Gtk::TreeView* _treeView;

	struct Columns :
		public Gtk::TreeModel::ColumnRecord
	{
		Columns() { add(name); }

		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	// The list store containing the list of ColourSchemes
	Columns _columns;
	Glib::RefPtr<Gtk::ListStore> _listStore;

	// The vbox containing the colour buttons and its frame
	Gtk::HBox* _colourBox;
	Gtk::Frame* _colourFrame;

	// The "delete scheme" button
	Gtk::Button* _deleteButton;

public:
	// Constructor
	ColourSchemeEditor();

	// Command target
	static void editColourSchemes(const cmd::ArgumentList& args);

protected:
	// Override TransientWindow's delete event
	virtual void _onDeleteEvent();

private:
	// private helper functions
	void 		populateTree();
    void 		createTreeView();
	Gtk::Widget& constructWindow();
	Gtk::Widget& constructButtons();
	Gtk::Widget& constructTreeviewButtons();
	Gtk::Widget& constructColourSelector(ColourItem& colour, const std::string& name);
	void 		updateColourSelectors();

	// Queries the user for a string and returns it
	// Returns "" if the user aborts or nothing is entered
	std::string inputDialog(const std::string& title, const std::string& label);

	// Puts the cursor on the currently active scheme
	void 		selectActiveScheme();

	// Updates the colour selectors after a selection change
	void 		selectionChanged();

	// Returns the name of the currently selected scheme
	std::string	getSelectedScheme();

	// Deletes or copies a scheme
	void 		deleteScheme();
	void 		copyScheme();

	// Deletes a scheme from the list store (called from deleteScheme())
	void 		deleteSchemeFromList();

	// gtkmm Callbacks
	void callbackSelChanged();
	void callbackOK();
	void callbackCancel();
	void callbackColorChanged(Gtk::ColorButton* widget, ColourItem* colour);
	void callbackDelete();
	void callbackCopy();

	// Destroy window and delete self, called by both Cancel and window
	// delete callbacks
	void doCancel();

	// Updates the windows after a colour change
	static void updateWindows();
};

} // namespace ui

#endif /*COLOURSCHEMEEDITOR_H_*/
