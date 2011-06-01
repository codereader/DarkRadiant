#include "GuiSelector.h"

#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "gui/GuiManager.h"
#include "gtkutil/dialog/MessageBox.h"

#include "ReadablePopulator.h"
#include "ReadableEditorDialog.h"

#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/notebook.h>
#include <gtkmm/label.h>
#include <gtkmm/box.h>
#include <gtkmm/treeview.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Choose a Gui Definition...");

	const int WINDOW_WIDTH = 400;
	const int WINDOW_HEIGHT = 500;

	const char* const GUI_ICON = "sr_icon_readable.png";
	const char* const FOLDER_ICON = "folder16.png";
}

GuiSelector::GuiSelector(bool twoSided, ReadableEditorDialog& editorDialog) :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), editorDialog.getRefPtr()),
	_editorDialog(editorDialog),
	_oneSidedStore(Gtk::TreeStore::create(_columns)),
	_twoSidedStore(Gtk::TreeStore::create(_columns)),
	_result(RESULT_CANCELLED)
{
	// Set the windowsize and default border width in accordance to the HIG
	set_default_size(WINDOW_WIDTH, WINDOW_HEIGHT);

	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	add(createInterface());

	// Set the current page and connect the switch-page signal afterwards.
	_notebook->set_current_page(twoSided ? 1 : 0);
	_notebook->signal_switch_page().connect(sigc::mem_fun(*this, &GuiSelector::onPageSwitch));

	// We start with an empty selection, so de-sensitise the OK button
	_okButton->set_sensitive(false);
}

void GuiSelector::_preShow()
{
	// Call the base class
	BlockingTransientWindow::_preShow();

	// Populate the treestores
	fillTrees();
}

std::string GuiSelector::run(bool twoSided, ReadableEditorDialog& editorDialog)
{
	GuiSelector dialog(twoSided, editorDialog);

	try
	{
		dialog.show();
	}
	catch (gtkutil::ModalProgressDialog::OperationAbortedException&)
	{
		return "";
	}

	return (dialog._result == RESULT_OK) ? "guis/" + dialog._name : "";
}

void GuiSelector::visit(const Glib::RefPtr<Gtk::TreeStore>& store,
						const Gtk::TreeModel::iterator& iter,
						const std::string& path,
						bool isExplicit)
{
	// Get the display name by stripping off everything before the last
	// slash
	std::string displayName = path.substr(path.rfind("/") + 1);
	displayName = displayName.substr(0,displayName.rfind("."));

	// Fill in the column values
	Gtk::TreeModel::Row row = *iter;

	row[_columns.name] = displayName;
	row[_columns.fullName] = path;
	row[_columns.icon] = GlobalUIManager().getLocalPixbuf(isExplicit ? GUI_ICON : FOLDER_ICON);
	row[_columns.isFolder] = !isExplicit;
}

void GuiSelector::fillTrees()
{
	gtkutil::VFSTreePopulator popOne(_oneSidedStore);
	gtkutil::VFSTreePopulator popTwo(_twoSidedStore);

	ReadablePopulator walker(popOne, popTwo, _editorDialog.getRefPtr());
	gui::GuiManager::Instance().foreachGui(walker);

	popOne.forEachNode(*this);
	popTwo.forEachNode(*this);
}

Gtk::Widget& GuiSelector::createInterface()
{
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	// Create the tabs
	_notebook = Gtk::manage(new Gtk::Notebook);

	// One-Sided Readables Tab
	Gtk::Label* labelOne = Gtk::manage(new Gtk::Label(_("One-Sided Readable Guis")));
	labelOne->show_all();

	Gtk::Widget &oneSidedTreeView = createOneSidedTreeView();
	_notebook->append_page(oneSidedTreeView, *labelOne);

	// Two-Sided Readables Tab
	Gtk::Label* labelTwo = Gtk::manage(new Gtk::Label(_("Two-Sided Readable Guis")));
	labelTwo->show_all();

	Gtk::Widget &twoSidedTreeView = createTwoSidedTreeView();
	_notebook->append_page(twoSidedTreeView, *labelTwo);

	// Make all the contents of _notebook show, so that set_current_page() works.
	oneSidedTreeView.show();
	twoSidedTreeView.show();

	// Packing
	vbox->pack_start(*_notebook, true, true, 0);
	vbox->pack_start(createButtons(), false, false, 0);

	return *vbox;
}

Gtk::Widget& GuiSelector::createButtons()
{
	_okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	_okButton->signal_clicked().connect(sigc::mem_fun(*this, &GuiSelector::onOk));

	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &GuiSelector::onCancel));

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	hbox->pack_start(*_okButton, false, false, 0);
	hbox->pack_start(*cancelButton, false, false, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbox));
}

Gtk::TreeView* GuiSelector::createTreeView(const Glib::RefPtr<Gtk::TreeStore>& store)
{
	// Create the treeview
	Gtk::TreeView* treeView = Gtk::manage(new Gtk::TreeView(store));

	treeView->set_headers_visible(false);

	// Add the selection and connect the signal
	Glib::RefPtr<Gtk::TreeSelection> selection = treeView->get_selection();
	selection->set_mode(Gtk::SELECTION_SINGLE);
	selection->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &GuiSelector::onSelectionChanged), treeView));

	// Single visible column, containing the directory/model name and the icon
	Gtk::TreeViewColumn* nameCol = Gtk::manage(new gtkutil::IconTextColumn(
		_("Gui Path"), _columns.name, _columns.icon
	));

	treeView->append_column(*nameCol);

	// Set the tree store's sort behaviour
	gtkutil::TreeModel::applyFoldersFirstSortFunc(store, _columns.name, _columns.isFolder);

	// Use the TreeModel's full string search function
	treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	return treeView;
}

Gtk::Widget& GuiSelector::createOneSidedTreeView()
{
	// Create the treeview
	Gtk::TreeView* treeViewOne = createTreeView(_oneSidedStore);

	Gtk::ScrolledWindow* scrolledFrame = Gtk::manage(new gtkutil::ScrolledFrame(*treeViewOne));
	scrolledFrame->set_border_width(12);

	return *scrolledFrame;
}

Gtk::Widget& GuiSelector::createTwoSidedTreeView()
{
	// Create the treeview
	Gtk::TreeView* treeViewTwo = createTreeView(_twoSidedStore);

	Gtk::ScrolledWindow* scrolledFrame = Gtk::manage(new gtkutil::ScrolledFrame(*treeViewTwo));
	scrolledFrame->set_border_width(12);

	return *scrolledFrame;
}

void GuiSelector::onCancel()
{
	destroy();
}

void GuiSelector::onOk()
{
	_result = RESULT_OK;

	// Delete the notebook to prevent it from switching pages when destroying the window
	delete(_notebook);

	// Everything done. Destroy the window!
	destroy();
}

void GuiSelector::onPageSwitch(GtkNotebookPage* page, guint page_num)
{
	if ((page_num == 0))
		_editorDialog.useOneSidedEditing();
	else
		_editorDialog.useTwoSidedEditing();
}

void GuiSelector::onSelectionChanged(Gtk::TreeView* view)
{
	Gtk::TreeModel::iterator iter = view->get_selection()->get_selected();

	if (iter && !(*iter)[_columns.isFolder])
	{
		_name = Glib::ustring((*iter)[_columns.fullName]);
		std::string guiPath = "guis/" + _name;

		_editorDialog.updateGuiView(getRefPtr(), guiPath);

		_okButton->set_sensitive(true);
	}
	else
	{
		_okButton->set_sensitive(false);
	}
}

} // namespace
