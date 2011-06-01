#include "XDataSelector.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"

#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/dialog/MessageBox.h"

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>

#include "ReadableEditorDialog.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Choose an XData Definition...");
	const int WINDOW_WIDTH = 400;
	const int WINDOW_HEIGHT = 500;

	const char* const XDATA_ICON = "sr_icon_readable.png";
	const char* const FOLDER_ICON = "folder16.png";
}

XDataSelector::XDataSelector(const XData::StringVectorMap& files, ReadableEditorDialog& editorDialog) :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), editorDialog.getRefPtr()),
	_store(Gtk::TreeStore::create(_columns)),
	_okButton(NULL),
	_files(files),
	_editorDialog(editorDialog),
	_result(RESULT_CANCELLED)
{
	set_default_size(WINDOW_WIDTH, WINDOW_HEIGHT);

	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Add a vbox for the dialog elements
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	vbox->pack_start(createTreeView(), true, true, 0);
	vbox->pack_start(createButtons(), false, false, 0);

	add(*vbox);

	fillTree();

	_okButton->set_sensitive(false);
}

std::string XDataSelector::run(const XData::StringVectorMap& files, ReadableEditorDialog& editorDialog)
{
	XDataSelector dialog(files, editorDialog);
	dialog.show();

	return (dialog._result == RESULT_OK) ? dialog._selection : "";
}

void XDataSelector::visit(const Glib::RefPtr<Gtk::TreeStore>& store,
						  const Gtk::TreeModel::iterator& iter,
						  const std::string& path,
						  bool isExplicit)
{
	// Fill in the column values
	Gtk::TreeModel::Row row = *iter;

	row[_columns.name] = path.substr(path.rfind("/") + 1);
	row[_columns.fullName] = path;
	row[_columns.icon] = GlobalUIManager().getLocalPixbuf(isExplicit ? XDATA_ICON : FOLDER_ICON);
	row[_columns.isFolder] = !isExplicit;
}

void XDataSelector::fillTree()
{
	// Start adding to tree.
	gtkutil::VFSTreePopulator populator(_store);

	for (XData::StringVectorMap::const_iterator it = _files.begin(); it != _files.end(); ++it)
	{
		populator.addPath(it->first);
	}

	populator.forEachNode(*this);
}

Gtk::Widget& XDataSelector::createTreeView()
{
	// Create the treeview
	Gtk::TreeView* treeView = Gtk::manage(new Gtk::TreeView(_store));

	treeView->set_headers_visible(false);

	// Add the selection and connect the signal
	Glib::RefPtr<Gtk::TreeSelection> selection = treeView->get_selection();
	selection->set_mode(Gtk::SELECTION_SINGLE);
	selection->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &XDataSelector::onSelectionChanged), treeView));

	// Single visible column, containing the directory/model name and the icon
	Gtk::TreeViewColumn* nameCol = Gtk::manage(new gtkutil::IconTextColumn(
		_("Xdata Path"), _columns.name, _columns.icon
	));

	treeView->append_column(*nameCol);

	// Set the tree store's sort behaviour
	gtkutil::TreeModel::applyFoldersFirstSortFunc(_store, _columns.name, _columns.isFolder);

	// Use the TreeModel's full string search function
	treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	// Pack treeview into a scrolled window and frame, and return
	return *Gtk::manage(new gtkutil::ScrolledFrame(*treeView));
}

Gtk::Widget& XDataSelector::createButtons()
{
	_okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	_okButton->signal_clicked().connect(sigc::mem_fun(*this, &XDataSelector::onOk));

	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &XDataSelector::onCancel));

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	hbox->pack_start(*_okButton, false, false, 0);
	hbox->pack_start(*cancelButton, false, false, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbox));
}

void XDataSelector::onCancel()
{
	destroy();
}

void XDataSelector::onOk()
{
	_result = RESULT_OK;

	// Everything done. Destroy the window!
	destroy();
}

void XDataSelector::onSelectionChanged(Gtk::TreeView* view)
{
	Gtk::TreeModel::iterator iter = view->get_selection()->get_selected();

	if (iter && !(*iter)[_columns.isFolder])
	{
		_selection = Glib::ustring((*iter)[_columns.fullName]);
		_editorDialog.updateGuiView(getRefPtr(), "", _selection);

		_okButton->set_sensitive(true);
	}
	else
	{
		_okButton->set_sensitive(false);
	}
}

} // namespace ui
