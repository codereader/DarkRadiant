#include "ColourSchemeEditor.h"
#include "ColourSchemeManager.h"
#include "iregistry.h"
#include "imainframe.h"
#include "ibrush.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "i18n.h"

#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treeview.h>
#include <gtkmm/frame.h>
#include <gtkmm/box.h>
#include <gtkmm/paned.h>
#include <gtkmm/colorbutton.h>

#include "gtkutil/TreeModel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/dialog/Dialog.h"
#include "gtkutil/dialog/MessageBox.h"

namespace ui {

	namespace
	{
		// Constants
    	const int COLOURS_PER_COLUMN = 10;

		const char* const EDITOR_WINDOW_TITLE = N_("Edit Colour Schemes");

		const unsigned int FULL_INTENSITY = 65535;
	}

ColourSchemeEditor::ColourSchemeEditor() :
	BlockingTransientWindow(_(EDITOR_WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_listStore(Gtk::ListStore::create(_columns))
{
	set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	add(constructWindow());

	// Load all the list items
  	populateTree();

	// Highlight the currently selected scheme
	selectActiveScheme();
	updateColourSelectors();

	// Connect the signal AFTER selecting the active scheme
	_treeView->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &ColourSchemeEditor::callbackSelChanged));
}

void ColourSchemeEditor::_onDeleteEvent()
{
	// Cancel action first
	doCancel();

	// Proceed with regular destruction
	BlockingTransientWindow::_onDeleteEvent();
}

/*	Loads all the scheme items into the list
 */
void ColourSchemeEditor::populateTree()
{
	ColourSchemeMap allSchemes = ColourSchemeManager::Instance().getSchemeList();

	for (ColourSchemeMap::iterator scheme = allSchemes.begin();
		 scheme != allSchemes.end(); ++scheme)
	{
		Gtk::TreeModel::Row row = *_listStore->append();

		row[_columns.name] = scheme->first;
	}
}

void ColourSchemeEditor::createTreeView()
{
	// Create the treeView
	_treeView = Gtk::manage(new Gtk::TreeView(_listStore));
	_treeView->set_size_request(200, -1);

	// Create a new column and set its parameters
	_treeView->append_column(*Gtk::manage(new gtkutil::TextColumn("Colour", _columns.name, false)));

   	_treeView->set_headers_visible(false);
}

Gtk::Widget& ColourSchemeEditor::constructButtons()
{
	// Create the buttons and put them into a horizontal box
	Gtk::HBox* buttonBox = Gtk::manage(new Gtk::HBox(true, 12));

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	buttonBox->pack_end(*okButton, true, true, 0);
	buttonBox->pack_end(*cancelButton, true, true, 0);

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &ColourSchemeEditor::callbackOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ColourSchemeEditor::callbackCancel));

	return *Gtk::manage(new gtkutil::RightAlignment(*buttonBox));
}

// Construct buttons underneath the list box
Gtk::Widget& ColourSchemeEditor::constructTreeviewButtons()
{
	Gtk::HBox* buttonBox = Gtk::manage(new Gtk::HBox(true, 6));

	_deleteButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
	Gtk::Button* copyButton = Gtk::manage(new Gtk::Button(Gtk::Stock::COPY));

	buttonBox->pack_start(*copyButton, true, true, 0);
	buttonBox->pack_start(*_deleteButton, true, true, 0);

	copyButton->signal_clicked().connect(sigc::mem_fun(*this, &ColourSchemeEditor::callbackCopy));
	_deleteButton->signal_clicked().connect(sigc::mem_fun(*this, &ColourSchemeEditor::callbackDelete));

	return *buttonBox;
}

Gtk::Widget& ColourSchemeEditor::constructWindow()
{
	// The vbox that separates the buttons and the upper part of the window
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 12));

	// Place the buttons at the bottom of the window
	vbox->pack_end(constructButtons(), false, false, 0);

	// VBox containing the tree view and copy/delete buttons underneath
	Gtk::VBox* treeAndButtons = Gtk::manage(new Gtk::VBox(false, 6));

	// Create the treeview and pack it into the treeViewFrame
	createTreeView();

	treeAndButtons->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_treeView)), true, true, 0);
	treeAndButtons->pack_end(constructTreeviewButtons(), false, false, 0);

	// The Box containing the Colour, pack it into the right half of the hbox
	_colourFrame = Gtk::manage(new Gtk::Frame);
	_colourBox = Gtk::manage(new Gtk::HBox(false, 5));

	_colourFrame->add(*_colourBox);

	// This is the divider for the treeview and the whole rest
	Gtk::HPaned* paned = Gtk::manage(new Gtk::HPaned);

	// Pack the treeViewFrame into the hbox
	paned->add1(*treeAndButtons);
	paned->add2(*_colourFrame);

	vbox->pack_start(*paned, true, true, 0);

	return *vbox;
}

void ColourSchemeEditor::selectActiveScheme()
{
	Gtk::TreeModel::Children children = _listStore->children();

	for (Gtk::TreeModel::Children::iterator i = children.begin(); i != children.end(); ++i)
	{
		// Get the name
		std::string name = Glib::ustring((*i)[_columns.name]);

		if (ColourSchemeManager::Instance().isActive(name))
		{
			_treeView->get_selection()->select(i);

			// Set the button sensitivity correctly for read-only schemes
			_deleteButton->set_sensitive(
				!ColourSchemeManager::Instance().getScheme(name).isReadOnly()
			);

			return;
		}
	}
}

void ColourSchemeEditor::deleteSchemeFromList()
{
	Gtk::TreeModel::iterator iter = _treeView->get_selection()->get_selected();

	if (iter)
	{
		_listStore->erase(iter);
	}

	// Select the first scheme
	_treeView->get_selection()->select(_listStore->children().begin());
}

std::string ColourSchemeEditor::getSelectedScheme()
{
	Gtk::TreeModel::iterator iter = _treeView->get_selection()->get_selected();

	return iter ? Glib::ustring((*iter)[_columns.name]) : "";
}

Gtk::Widget& ColourSchemeEditor::constructColourSelector(ColourItem& colour, const std::string& name)
{
	// Get the description of this colour item from the registry
	std::string descriptionPath = std::string("user/ui/colourschemes/descriptions/") + name;
	std::string description = GlobalRegistry().get(descriptionPath);

	// Give gettext a chance to translate the colour description
	description = _(description.c_str());

	// Create a new colour button
	Gdk::Color tempColour;
	Vector3 tempColourVector = colour;
	tempColour.set_red(static_cast<gushort>(FULL_INTENSITY * tempColourVector[0]));
	tempColour.set_green(static_cast<guint16>(FULL_INTENSITY * tempColourVector[1]));
	tempColour.set_blue(static_cast<guint16>(FULL_INTENSITY * tempColourVector[2]));

	// Create the colour button
	Gtk::ColorButton* button = Gtk::manage(new Gtk::ColorButton(tempColour));

	button->set_title(description);

	// Connect the signal, so that the ColourItem class is updated along with the colour button
	button->signal_color_set().connect(
		sigc::bind(sigc::mem_fun(*this, &ColourSchemeEditor::callbackColorChanged), button, &colour)
	);

	button->show();

	// Create the description label
	Gtk::Label* label = Gtk::manage(new Gtk::Label(description));

	// Create a new horizontal divider
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 10));

	hbox->pack_start(*button, false, false, 0);
	hbox->pack_start(*label, false, false, 0);

	return *hbox;
}

void ColourSchemeEditor::updateColourSelectors()
{
	// Destroy the current _colourBox instance
	_colourBox = NULL;
	_colourFrame->remove();

	// Create a new column container
	_colourBox = Gtk::manage(new Gtk::HBox(false, 12));
	_colourFrame->add(*_colourBox);

	// Get the selected scheme
	ColourScheme& scheme = ColourSchemeManager::Instance().getScheme(getSelectedScheme());

	// Retrieve the list with all the ColourItems of this scheme
	ColourItemMap& colourMap = scheme.getColourMap();

	// A temporary vbox for each column
	Gtk::VBox* curVbox = Gtk::manage(new Gtk::VBox(false, 5));

	ColourItemMap::iterator it;
	unsigned int i = 1;
	// Cycle through all the ColourItems and save them into the registry
	for (it = colourMap.begin(), i = 1;
		 it != colourMap.end();
		 it++, i++)
	{
		Gtk::Widget& colourSelector = constructColourSelector(it->second, it->first);
		curVbox->pack_start(colourSelector, false, false, 5);

		// Have we reached the maximum number of colours per column?
		if (i % COLOURS_PER_COLUMN == 0)
		{
			// yes, pack the current column into the _colourBox and create a new vbox
			_colourBox->pack_start(*curVbox, false, false, 5);
			curVbox = Gtk::manage(new Gtk::VBox(false, 5));
		}
	}

	// Pack the remaining items into the last column
	_colourBox->pack_start(*curVbox, false, false, 0);

	_colourBox->show_all();
}

void ColourSchemeEditor::updateWindows()
{
	// Call the update, so all colours can be previewed
	GlobalMainFrame().updateAllWindows();
	SceneChangeNotify();
}

void ColourSchemeEditor::selectionChanged()
{
	std::string activeScheme = getSelectedScheme();

	// Update the colour selectors to reflect the newly selected scheme
	updateColourSelectors();

	// Check, if the currently selected scheme is read-only
	ColourScheme& scheme = ColourSchemeManager::Instance().getScheme(activeScheme);
	_deleteButton->set_sensitive(!scheme.isReadOnly());

	// Set the active Scheme, so that the views are updated accordingly
	ColourSchemeManager::Instance().setActive(activeScheme);

	updateWindows();
}

void ColourSchemeEditor::deleteScheme()
{
	std::string name = getSelectedScheme();
	// Get the selected scheme
	ColourScheme& scheme = ColourSchemeManager::Instance().getScheme(name);

	if (!scheme.isReadOnly())
	{
		// Remove the actual scheme from the ColourSchemeManager
		ColourSchemeManager::Instance().deleteScheme(name);

		// Remove the selected item from the GtkListStore
		deleteSchemeFromList();
	}
}

std::string ColourSchemeEditor::inputDialog(const std::string& title, const std::string& label)
{
	gtkutil::Dialog dialog(title, getRefPtr());

	dialog.addLabel(label);
	IDialog::Handle entryHandle = dialog.addEntryBox("");

	if (dialog.run() == IDialog::RESULT_OK)
	{
		return dialog.getElementValue(entryHandle);
	}
	else
	{
		return "";
	}
}

void ColourSchemeEditor::copyScheme()
{
	std::string name = getSelectedScheme();
	std::string newName = inputDialog(_("Copy Colour Scheme"), _("Enter a name for the new scheme:"));

	if (newName.empty())
	{
		return; // empty name
	}

	// greebo: Check if the new name is already existing
	if (ColourSchemeManager::Instance().schemeExists(newName))
	{
		gtkutil::MessageBox::ShowError(_("A Scheme with that name already exists."), getRefPtr());
		return;
	}

	// Copy the scheme
	ColourSchemeManager::Instance().copyScheme(name, newName);
	ColourSchemeManager::Instance().setActive(newName);

	// Add the new list item to the ListStore
	Gtk::TreeModel::Row row = *_listStore->append();
	row[_columns.name] = newName;

	// Highlight the copied scheme
	selectActiveScheme();
}

void ColourSchemeEditor::callbackCopy()
{
	copyScheme();
}

void ColourSchemeEditor::callbackDelete()
{
	deleteScheme();
}

void ColourSchemeEditor::callbackColorChanged(Gtk::ColorButton* widget, ColourItem* colourItem)
{
	Gdk::Color colour = widget->get_color();

	// Update the colourItem class
	colourItem->set(static_cast<float>(colour.get_red_p()), 
					static_cast<float>(colour.get_green_p()), 
					static_cast<float>(colour.get_blue_p()));

	// Call the update, so all colours can be previewed
	updateWindows();
}

// This is called when the colourscheme selection is changed
void ColourSchemeEditor::callbackSelChanged()
{
	selectionChanged();
}

void ColourSchemeEditor::callbackOK()
{
	ColourSchemeManager::Instance().setActive(getSelectedScheme());
	ColourSchemeManager::Instance().saveColourSchemes();

	destroy();
}

// Destroy self function
void ColourSchemeEditor::doCancel()
{
	// Restore all the colour settings from the XMLRegistry, changes get lost
	ColourSchemeManager::Instance().restoreColourSchemes();

	// Call the update, so all restored colours are displayed
	updateWindows();
}

// Cancel button callback
void ColourSchemeEditor::callbackCancel()
{
	doCancel();
	destroy();
}

void ColourSchemeEditor::editColourSchemes(const cmd::ArgumentList& args)
{
	 ColourSchemeEditor editor;
	 editor.show(); // enter GTK main loop
}


} // namespace ui

