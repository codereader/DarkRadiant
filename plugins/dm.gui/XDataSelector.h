#ifndef _XDATA_SELECTOR_H_
#define _XDATA_SELECTOR_H_

#include "XDataLoader.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/VFSTreePopulator.h"
#include <gtkmm/treestore.h>

namespace Gtk
{
	class Button;
	class TreeView;
}

namespace ui
{

class ReadableEditorDialog;

///////////////////////////// XDataSelector:
// Runs a dialog for choosing XData definitions, which updates the guiView of the calling
// ReadableEditorDialog for previewing.
class XDataSelector :
	public gtkutil::BlockingTransientWindow,
	public gtkutil::VFSTreePopulator::Visitor
{
private:
	// Treestore enum
	struct XdataTreeModelColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		XdataTreeModelColumns()
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

	// The tree
	XdataTreeModelColumns _columns;
	Glib::RefPtr<Gtk::TreeStore> _store;

	Gtk::Button* _okButton;

	// A Map of XData files. Basically just the keyvalues are needed.
	XData::StringVectorMap _files;

	// The name of the chosen definition
	std::string _selection;

	// Pointer to the ReadableEditorDialog for updating the guiView.
	ReadableEditorDialog& _editorDialog;

	enum Result
	{
		RESULT_OK,
		RESULT_CANCELLED,
	};

	Result _result;

public:
	// Runs the dialog and returns the name of the chosen definition.
	static std::string run(const XData::StringVectorMap& files, ReadableEditorDialog& editorDialog);

	void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
			   const Gtk::TreeModel::iterator& iter,
			   const std::string& path,
			   bool isExplicit);

private:
	//private contructor called by the run method.
	XDataSelector(const XData::StringVectorMap& files, ReadableEditorDialog& editorDialog);

	void fillTree();

	// Helper functions to create GUI components
	Gtk::Widget& createTreeView();
	Gtk::Widget& createButtons();

	void onCancel();
	void onOk();
	void onSelectionChanged(Gtk::TreeView* view); // view is manually bound
};

} //namespace ui

#endif
