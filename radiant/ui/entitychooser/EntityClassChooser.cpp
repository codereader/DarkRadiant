#include "EntityClassChooser.h"
#include "EntityClassTreePopulator.h"

#include "i18n.h"
#include "iregistry.h"
#include "imainframe.h"
#include "iuimanager.h"

#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/MultiMonitor.h"
#include "string/string.h"

namespace ui
{

	namespace
	{
		const char* const ECLASS_CHOOSER_TITLE = N_("Create entity");
	}

// Display the singleton instance
std::string EntityClassChooser::chooseEntityClass()
{
	return Instance().showAndBlock();
}

EntityClassChooser& EntityClassChooser::Instance()
{
	EntityClassChooserPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new EntityClassChooser);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(instancePtr);
	}


	return *instancePtr;
}

EntityClassChooserPtr& EntityClassChooser::InstancePtr()
{
	static EntityClassChooserPtr _instancePtr;
	return _instancePtr;
}

void EntityClassChooser::onRadiantShutdown()
{
	globalOutputStream() << "EntityClassChooser shutting down." << std::endl;

	GlobalEntityClassManager().removeObserver(this);

	_modelPreview = IModelPreviewPtr();

	// Final step at shutdown, release the shared ptr
	InstancePtr().reset();
}

void EntityClassChooser::onEClassReload()
{
	// Reload the class tree
	loadEntityClasses();
}

std::string EntityClassChooser::showAndBlock()
{
	// Show and enter main recursion
	show(); 

	// Release the models after showing
	_modelPreview->clear();
	
	// Return the last selection (may be "" if dialog was cancelled)
	return _selectedName;
}

// Constructor. Creates GTK widgets.

EntityClassChooser::EntityClassChooser()
: gtkutil::BlockingTransientWindow(_(ECLASS_CHOOSER_TITLE), GlobalMainFrame().getTopLevelWindow()),
  _treeStore(Gtk::TreeStore::create(_columns)),
  _treeView(NULL),
  _selection(NULL),
  _okButton(NULL),
  _selectedName(""),
  _modelPreview(GlobalUIManager().createModelPreview())
{
	set_border_width(12);

	// Set the default size of the window
	const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();	
	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	set_default_size(
		static_cast<int>(rect.get_width() * 0.7f), static_cast<int>(rect.get_height() * 0.6f)
	);

	_modelPreview->setSize(static_cast<int>(rect.get_width() * 0.3f));

	// Create GUI elements and pack into main VBox
	
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 6));
	vbx->pack_start(createTreeView(), true, true, 0);
	vbx->pack_start(createUsagePanel(), false, false, 0);
	vbx->pack_start(createButtonPanel(), false, false, 0);
	
	hbox->pack_start(*vbx, true, true, 0);
	hbox->pack_start(*Glib::wrap(_modelPreview->getWidget(), true), false, false, 0);

	add(*hbox);

	// Register to the eclass manager
	GlobalEntityClassManager().addObserver(this);

	// Populate the model
	loadEntityClasses();
}

void EntityClassChooser::_onDeleteEvent()
{
	// greebo: Clear the selected name on hide, we don't want to create another entity when 
	// the user clicks on the X in the upper right corner.
	_selectedName.clear();

	hide(); // just hide, don't call base class which might delete this dialog
}

void EntityClassChooser::_postShow()
{
	// Initialise the GL widget after the widgets have been shown
	_modelPreview->initialisePreview();

	// Update the member variables
	updateSelection();

	// Focus on the treeview
	_treeView->grab_focus();

	// Call the base class, will enter main loop
	BlockingTransientWindow::_postShow();
}

void EntityClassChooser::loadEntityClasses()
{
	// Clear the tree store first
	_treeStore->clear();

	// Populate it with the list of entity
	// classes by using a visitor class.
	EntityClassTreePopulator visitor(_treeStore, _columns);
	GlobalEntityClassManager().forEach(visitor);

	// insert the data, using the same walker class as Visitor
	visitor.forEachNode(visitor);
}

// Create the tree view

Gtk::Widget& EntityClassChooser::createTreeView()
{
	// Construct the tree view widget with the now-populated model
	_treeView = Gtk::manage(new Gtk::TreeView(_treeStore));
	_treeView->set_headers_visible(true);

	// Use the TreeModel's full string search function
	_treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContainsmm));

	_selection = _treeView->get_selection();
	_selection->set_mode(Gtk::SELECTION_BROWSE);
	_selection->signal_changed().connect(sigc::mem_fun(*this, &EntityClassChooser::callbackSelectionChanged));

	// Single column with icon and name
	Gtk::TreeViewColumn* col = Gtk::manage(
		new gtkutil::IconTextColumnmm(_("Classname"), _columns.name, _columns.icon)
	);
	col->set_sort_column_id(_columns.name);

	_treeView->append_column(*col);

	_treeStore->set_sort_column_id(_columns.name, Gtk::SORT_ASCENDING);

	// Pack treeview into a scrolled frame and return
	return *Gtk::manage(new gtkutil::ScrolledFramemm(*_treeView));
}

// Create the entity usage information panel
Gtk::Widget& EntityClassChooser::createUsagePanel()
{
	// Create a GtkTextView
	_usageTextView = Gtk::manage(new Gtk::TextView);
	_usageTextView->set_wrap_mode(Gtk::WRAP_WORD);
	_usageTextView->set_editable(false);

	return *Gtk::manage(new gtkutil::ScrolledFramemm(*_usageTextView));
}

// Create the button panel
Gtk::Widget& EntityClassChooser::createButtonPanel()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));
	
	_okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	_okButton->signal_clicked().connect(sigc::mem_fun(*this, &EntityClassChooser::callbackOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &EntityClassChooser::callbackCancel));
	
	hbx->pack_end(*_okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);
					   
	return *Gtk::manage(new gtkutil::RightAlignmentmm(*hbx));
}

// Update the usage information
void EntityClassChooser::updateUsageInfo(const std::string& eclass)
{
	// Lookup the IEntityClass instance
	IEntityClassPtr e = GlobalEntityClassManager().findOrInsert(eclass, true);	

	// Set the usage panel to the IEntityClass' usage information string
	Glib::RefPtr<Gtk::TextBuffer> buf = _usageTextView->get_buffer();
	
	// Create the concatenated usage string
	std::string usage = "";
	EntityClassAttributeList usageAttrs = e->getAttributeList("editor_usage");

	for (EntityClassAttributeList::const_iterator i = usageAttrs.begin();
		 i != usageAttrs.end();
		 ++i)
	{
		// Add only explicit (non-inherited) usage strings
		if (!i->inherited)
		{
			if (!usage.empty())
				usage += std::string("\n") + i->value;
			else
				usage += i->value;
		}
	}
	
	buf->set_text(usage);
}

void EntityClassChooser::updateSelection()
{
	Gtk::TreeModel::iterator iter = _selection->get_selected();

	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;

		if (!row[_columns.isFolder])
		{
			// Make the OK button active 
			_okButton->set_sensitive(true);

			// Set the panel text with the usage information
			std::string selName = Glib::ustring(row[_columns.name]);
			updateUsageInfo(selName);

			// Lookup the IEntityClass instance
			IEntityClassPtr eclass = GlobalEntityClassManager().findClass(selName);	

			if (eclass != NULL)
			{
				_modelPreview->setModel(eclass->getAttribute("model").value);
				_modelPreview->setSkin(eclass->getAttribute("skin").value);
			}

			// Update the _selectionName field
			_selectedName = selName;

			return; // success
		}
	}

	// Nothing selected
	_modelPreview->setModel("");
	_modelPreview->setSkin("");

	_okButton->set_sensitive(false);
}

void EntityClassChooser::callbackCancel() 
{
	_selectedName.clear();
	
	hide(); // breaks main loop
}

void EntityClassChooser::callbackOK() 
{
	hide(); // breaks main loop
}

void EntityClassChooser::callbackSelectionChanged() 
{
	updateSelection();
}

} // namespace ui
