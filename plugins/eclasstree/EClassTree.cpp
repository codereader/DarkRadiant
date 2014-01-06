#include "EClassTree.h"

#include "ieclass.h"
#include "ientity.h"
#include "imainframe.h"
#include "iselection.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/MultiMonitor.h"

#include "EClassTreeBuilder.h"
#include "i18n.h"

#include <gtkmm/treeview.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/paned.h>

#include <boost/bind.hpp>

namespace ui {

	namespace
	{
		const char* const ECLASSTREE_TITLE = N_("Entity Class Tree");
	}

EClassTree::EClassTree() :
	gtkutil::BlockingTransientWindow(_(ECLASSTREE_TITLE), GlobalMainFrame().getTopLevelWindow())
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create a new tree store for the entityclasses
	_eclassStore = Gtk::TreeStore::create(_eclassColumns);

	// Construct an eclass visitor and traverse the entity classes
	EClassTreeBuilder builder(_eclassStore, _eclassColumns);

	// Construct the window's widgets
	populateWindow();

	// Enter main loop
	show();
}

void EClassTree::populateWindow()
{
	// Create the overall vbox
	Gtk::VBox* dialogVBox = Gtk::manage(new Gtk::VBox(false, 12));
	add(*dialogVBox);

	Gtk::HPaned* paned = Gtk::manage(new Gtk::HPaned);
	dialogVBox->pack_start(*paned, true, true, 0);

	// Pack tree view
	paned->add1(createEClassTreeView());

	// Pack spawnarg treeview
	paned->add2(createPropertyTreeView());

	// Pack in dialog buttons
	dialogVBox->pack_start(createButtons(), false, false, 0);

	// Set the default size of the window
	const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();
	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);

	set_default_size(
		static_cast<int>(rect.get_width() * 0.8f), static_cast<int>(rect.get_height() * 0.8f)
	);

	paned->set_position(static_cast<int>(rect.get_width() * 0.25f));
}

Gtk::Widget& EClassTree::createEClassTreeView()
{
	_eclassView = Gtk::manage(new Gtk::TreeView(_eclassStore));

	// Use the TreeModel's full string search function
	_eclassView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	// Tree selection
	_eclassSelection = _eclassView->get_selection();
	_eclassSelection->set_mode(Gtk::SELECTION_BROWSE);
	_eclassSelection->signal_changed().connect(sigc::mem_fun(*this, &EClassTree::onSelectionChanged));

	_eclassView->set_headers_visible(true);

	// Pack the columns
	// Single column with icon and name
	Gtk::TreeViewColumn* col = Gtk::manage(
		new gtkutil::IconTextColumn(_("Classname"), _eclassColumns.name, _eclassColumns.icon
	));
	col->set_sort_column(_eclassColumns.name);

	_eclassView->append_column(*col);

	return *Gtk::manage(new gtkutil::ScrolledFrame(*_eclassView));
}

Gtk::Widget& EClassTree::createPropertyTreeView()
{
	// Initialise the instance TreeStore
	_propertyStore = Gtk::ListStore::create(_propertyColumns);

    // Create the TreeView widget and link it to the model
	_propertyView = Gtk::manage(new Gtk::TreeView(_propertyStore));

    // Create the Property column
	Gtk::TreeViewColumn* nameCol = Gtk::manage(new Gtk::TreeViewColumn);
    nameCol->set_title(_("Property"));
	nameCol->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    nameCol->set_spacing(3);

	Gtk::CellRendererText* textRenderer = Gtk::manage(new Gtk::CellRendererText);
	nameCol->pack_start(*textRenderer, false);
	nameCol->add_attribute(textRenderer->property_text(), _propertyColumns.name);
	nameCol->add_attribute(textRenderer->property_foreground(), _propertyColumns.colour);

	nameCol->set_sort_column(_propertyColumns.name);
    _propertyView->append_column(*nameCol);

	// Create the value column
	Gtk::TreeViewColumn* valCol = Gtk::manage(new Gtk::TreeViewColumn);
    valCol->set_title(_("Value"));
	valCol->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);

	Gtk::CellRendererText* valRenderer = Gtk::manage(new Gtk::CellRendererText);
	valCol->pack_start(*valRenderer, true);
	valCol->add_attribute(valRenderer->property_text(), _propertyColumns.value);
	valCol->add_attribute(valRenderer->property_foreground(), _propertyColumns.colour);

	valCol->set_sort_column(_propertyColumns.value);
    _propertyView->append_column(*valCol);

	return *Gtk::manage(new gtkutil::ScrolledFrame(*_propertyView));
}

// Lower dialog buttons
Gtk::Widget& EClassTree::createButtons()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(true, 12));

	// Close Button
	Gtk::Button* closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));
	closeButton->signal_clicked().connect(sigc::mem_fun(*this, &EClassTree::onClose));
	buttonHBox->pack_end(*closeButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*buttonHBox));
}

void EClassTree::addToListStore(const EntityClassAttribute& attr)
{
    // Append the details to the treestore
    Gtk::TreeModel::Row row = *_propertyStore->append();

    row[_propertyColumns.name] = attr.getName();
    row[_propertyColumns.value] = attr.getValue();
    row[_propertyColumns.colour] = attr.inherited ? "#666666" : "black";
    row[_propertyColumns.inherited] = attr.inherited ? "1" : "0";
}

void EClassTree::updatePropertyView(const std::string& eclassName)
{
	// Clear the existing list
	_propertyStore->clear();

	IEntityClassPtr eclass = GlobalEntityClassManager().findClass(eclassName);
	if (eclass == NULL)
    {
		return;
	}

	eclass->forEachClassAttribute(
        boost::bind(&EClassTree::addToListStore, this, _1), true
    );
}

void EClassTree::_preShow()
{
	// Do we have anything selected
	if (GlobalSelectionSystem().countSelected() == 0) {
		return;
	}

	// Get the last selected node and check if it's an entity
	scene::INodePtr lastSelected = GlobalSelectionSystem().ultimateSelected();

	Entity* entity = Node_getEntity(lastSelected);

	if (entity != NULL)
	{
		// There is an entity selected, extract the classname
		std::string classname = entity->getKeyValue("classname");

		// Find the classname
		gtkutil::TreeModel::findAndSelectString(_eclassView, classname, _eclassColumns.name);
	}
}

// Static command target
void EClassTree::showWindow(const cmd::ArgumentList& args)
{
	// Construct a new instance, this enters the main loop
	EClassTree _tree;
}

void EClassTree::onClose()
{
	destroy();
}

void EClassTree::onSelectionChanged()
{
	// Prepare to check for a selection
	Gtk::TreeModel::iterator iter = _eclassSelection->get_selected();

	// Add button is enabled if there is a selection and it is not a folder.
	if (iter)
	{
		_propertyView->set_sensitive(true);

		// Set the panel text with the usage information
		updatePropertyView(Glib::ustring((*iter)[_eclassColumns.name]));
	}
	else
	{
		_propertyView->set_sensitive(false);
	}
}

} // namespace ui
