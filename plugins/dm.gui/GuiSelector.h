#ifndef _GUI_SELECTOR_H_
#define _GUI_SELECTOR_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/VFSTreePopulator.h"

#include <gtkmm/treestore.h>

namespace Gtk
{
	class Notebook;
	class Button;
	class TreeView;
}

namespace ui
{

// Forward decl.
class ReadableEditorDialog;

///////////////////////////// XDataSelector:
// Selector-Dialog for TwoSided and OneSided readable guis. Switching the pages of the notebook
// also toggles the according editing mode on the ReadableEditorDialog (TwoSided or OneSided).
// Selecting a gui definition updates the guiView for previewing.
class GuiSelector :
	public gtkutil::BlockingTransientWindow,
	public gtkutil::VFSTreePopulator::Visitor
{
public:
	// Treestore enum
	struct GuiTreeModelColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		GuiTreeModelColumns()
		{
			add(name);
			add(fullName);
			add(icon);
			add(isFolder);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> fullName;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
		Gtk::TreeModelColumn<bool> isFolder;
	};

private:
	// Reference to the calling ReadableEditorDialog
	ReadableEditorDialog& _editorDialog;

	// The name that was picked.
	std::string _name;

	// The notebook holding the tabs for one-sided and two-sided readables.
	Gtk::Notebook* _notebook;

	GuiTreeModelColumns _columns;
	Glib::RefPtr<Gtk::TreeStore> _oneSidedStore;
	Glib::RefPtr<Gtk::TreeStore> _twoSidedStore;

	Gtk::Button* _okButton;

	enum Result
	{
		RESULT_OK,
		RESULT_CANCELLED,
	};

	Result _result;

public:
	// Starts the GuiSelector and returns the name of the selected GUI or an empty string if the user canceled.
	// The dialog shows the twoSided treeview if twoSided is true.
	static std::string run(bool twoSided, ReadableEditorDialog& editorDialog);

	void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
			   const Gtk::TreeModel::iterator& iter,
			   const std::string& path,
			   bool isExplicit);

protected:
	void _preShow();

private:
	GuiSelector(bool twoSided, ReadableEditorDialog& editorDialog);

	void fillTrees();

	Gtk::Widget& createInterface();
	Gtk::Widget& createButtons();

	Gtk::TreeView* createTreeView(const Glib::RefPtr<Gtk::TreeStore>& store);

	Gtk::Widget& createOneSidedTreeView();
	Gtk::Widget& createTwoSidedTreeView();

	void onCancel();
	void onOk();
	void onSelectionChanged(Gtk::TreeView* view); // view is manually bound
	void onPageSwitch(GtkNotebookPage* page, guint page_num);
};

} // namespace

#endif
