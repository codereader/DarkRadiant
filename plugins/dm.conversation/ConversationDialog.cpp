#include "ConversationDialog.h"

#include "i18n.h"
#include "iregistry.h"
#include "iundo.h"
#include "ieclass.h"
#include "imainframe.h"
#include "iscenegraph.h"
#include "string/string.h"

#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/dialog/MessageBox.h"

#include "RandomOrigin.h"
#include "ConversationEntityFinder.h"
#include "ConversationEditor.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treeview.h>

#include <gdk/gdkkeysyms.h>

#include <boost/format.hpp>
#include <iostream>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Conversation Editor");

	const std::string RKEY_ROOT = "user/ui/conversationDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

	const std::string CONVERSATION_ENTITY_CLASS = "atdm:conversation_info";
}

ConversationDialog::ConversationDialog() :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_convEntityList(Gtk::ListStore::create(_convEntityColumns)),
	_entityView(NULL),
	_convList(Gtk::ListStore::create(_convColumns))
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	signal_key_press_event().connect(sigc::mem_fun(*this, &ConversationDialog::onWindowKeyPress), false);

	// Create the widgets
	populateWindow();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();

	// Show the dialog, this enters the gtk main loop
	show();
}

void ConversationDialog::_preHide()
{
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
}

void ConversationDialog::_preShow()
{
	// Restore the position
	_windowPosition.applyPosition();

	populateWidgets();
}

void ConversationDialog::populateWindow()
{
	// Create the overall vbox
	_dialogVBox = Gtk::manage(new Gtk::VBox(false, 12));
	add(*_dialogVBox);

	_dialogVBox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Conversation entities") + "</b>")),
		false, false, 0);

	_dialogVBox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignment(createEntitiesPanel(), 18, 1.0)),
		false, false, 0);

	_dialogVBox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Conversations") + "</b>")),
		false, false, 0);

	_dialogVBox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignment(createConversationsPanel(), 18, 1.0)),
		true, true, 0);

	// Pack in dialog buttons
	_dialogVBox->pack_start(createButtons(), false, false, 0);
}

// Create the conversation entity panel
Gtk::Widget& ConversationDialog::createEntitiesPanel()
{
	// Hbox containing the entity list and the buttons vbox
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 6));

	// Tree view listing the conversation_info entities
	_entityView = Gtk::manage(new Gtk::TreeView(_convEntityList));
	_entityView->set_headers_visible(false);

	Glib::RefPtr<Gtk::TreeSelection> sel = _entityView->get_selection();
	sel->signal_changed().connect(sigc::mem_fun(*this, &ConversationDialog::onEntitySelectionChanged));

	// Entity Name column
	_entityView->append_column(*Gtk::manage(new gtkutil::TextColumn("", _convEntityColumns.displayName)));

	hbx->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_entityView)), true, true, 0);

	// Vbox for the buttons
	Gtk::VBox* buttonBox = Gtk::manage(new Gtk::VBox(false, 6));

	Gtk::Button* addButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD));
	addButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationDialog::onAddEntity));
	buttonBox->pack_start(*addButton, true, true, 0);

	_deleteEntityButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
	_deleteEntityButton->set_sensitive(false); // disabled at start
	_deleteEntityButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationDialog::onDeleteEntity));

	buttonBox->pack_start(*_deleteEntityButton, true, true, 0);

	hbx->pack_start(*buttonBox, false, false, 0);

	return *hbx;
}

// Create the main conversation editing widgets
Gtk::Widget& ConversationDialog::createConversationsPanel()
{
	// Tree view
	_convView = Gtk::manage(new Gtk::TreeView(_convList));
	_convView->set_headers_visible(true);

	Glib::RefPtr<Gtk::TreeSelection> sel = _convView->get_selection();
	sel->signal_changed().connect(sigc::mem_fun(*this, &ConversationDialog::onConversationSelectionChanged));

	// Key and value text columns
	_convView->append_column(*Gtk::manage(new gtkutil::TextColumn("#", _convColumns.index, false)));
	_convView->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Name"), _convColumns.name, false)));

	// Beside the list is an vbox containing add, edit, delete and clear buttons
	_convButtonPanel = Gtk::manage(new Gtk::VBox(false, 6));

    // Buttons panel box is disabled by default, enabled once an Entity is selected.
    _convButtonPanel->set_sensitive(false);

	Gtk::Button* addButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD));
	addButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationDialog::onAddConversation));

	_editConvButton = Gtk::manage(new Gtk::Button(Gtk::Stock::EDIT));
	_editConvButton->set_sensitive(false); // not enabled without selection
	_editConvButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationDialog::onEditConversation));

	_delConvButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
	_delConvButton->set_sensitive(false); // not enabled without selection
	_delConvButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationDialog::onDeleteConversation));

	_clearConvButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
	_clearConvButton->set_sensitive(false); // requires >0 conversations
	_clearConvButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationDialog::onClearConversations));

	_convButtonPanel->pack_start(*addButton, false, false, 0);
	_convButtonPanel->pack_start(*_editConvButton, false, false, 0);
	_convButtonPanel->pack_start(*_delConvButton, false, false, 0);
	_convButtonPanel->pack_start(*_clearConvButton, false, false, 0);

	// Pack the list and the buttons into an hbox
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 6));

	hbx->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_convView)), true, true, 0);
	hbx->pack_start(*_convButtonPanel, false, false, 0);

	return *hbx;
}

// Lower dialog buttons
Gtk::Widget& ConversationDialog::createButtons()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(true, 12));

	// Save button
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationDialog::onSave));
	buttonHBox->pack_end(*okButton, true, true, 0);

	// Close Button
	_closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	_closeButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationDialog::onClose));
	buttonHBox->pack_end(*_closeButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*buttonHBox));
}

void ConversationDialog::save()
{
	// Consistency check can go here

	// Scoped undo object
	UndoableCommand command("editConversations");

	// Save the working set to the entity
	for (conversation::ConversationEntityMap::iterator i = _entities.begin();
		 i != _entities.end();
		 ++i)
	{
		i->second->writeToEntity();
	}
}

void ConversationDialog::clear()
{
	// Clear internal data
	_entities.clear();
	_curEntity = _entities.end();

	// Clear the list boxes
	_convEntityList->clear();
	_convList->clear();
}

void ConversationDialog::refreshConversationList()
{
	// Clear and refresh the conversation list
	_convList->clear();
	_curEntity->second->populateListStore(_convList, _convColumns);

	// If there is at least one conversation, make the Clear button available
	_clearConvButton->set_sensitive(!_curEntity->second->isEmpty());
}

void ConversationDialog::populateWidgets()
{
	// First clear the data
	clear();

	// Use an ConversationEntityFinder to walk the map and add any conversation
	// entities to the liststore and entity map
	conversation::ConversationEntityFinder finder(
		_convEntityList,
		_convEntityColumns,
		_entities,
		CONVERSATION_ENTITY_CLASS
	);

	GlobalSceneGraph().root()->traverseChildren(finder);
}

void ConversationDialog::onSave()
{
	save();
	destroy();
}

void ConversationDialog::onClose()
{
	destroy();
}

bool ConversationDialog::onWindowKeyPress(GdkEventKey* ev)
{
	if (ev->keyval == GDK_Escape)
	{
		destroy();
		// Catch this keyevent, don't propagate
		return true;
	}

	// Propagate further
	return false;
}

// Static command target
void ConversationDialog::showDialog(const cmd::ArgumentList& args)
{
	// Construct a new instance, this enters the main loop
	ConversationDialog _editor;
}

// Callback for conversation entity selection changed in list box
void ConversationDialog::onEntitySelectionChanged()
{
	// Clear the conversations list
	_convList->clear();

	// Get the selection
	Gtk::TreeModel::iterator iter = _entityView->get_selection()->get_selected();

	if (iter)
    {
		// Get name of the entity and find the corresponding ConversationEntity in the map
		std::string name = Glib::ustring((*iter)[_convEntityColumns.entityName]);

		// Save the current selection and refresh the conversation list
		_curEntity = _entities.find(name);
		refreshConversationList();

		// Enable the delete button and conversation panel
		_deleteEntityButton->set_sensitive(true);
		_convButtonPanel->set_sensitive(true);
	}
	else
    {
		// No selection, disable the delete button and clear the conversation panel
		_deleteEntityButton->set_sensitive(false);

        // Disable all the Conversation edit buttons
		_convButtonPanel->set_sensitive(false);
	}
}

// Add a new conversations entity button
void ConversationDialog::onAddEntity()
{
	// Obtain the entity class object
	IEntityClassPtr eclass =
		GlobalEntityClassManager().findClass(CONVERSATION_ENTITY_CLASS);

    if (eclass)
    {
        // Construct a Node of this entity type
        IEntityNodePtr node(GlobalEntityCreator().createEntity(eclass));

        // Create a random offset
		node->getEntity().setKeyValue(
            "origin", RandomOrigin::generate(128)
        );

        // Insert the node into the scene graph
        assert(GlobalSceneGraph().root());
        GlobalSceneGraph().root()->addChildNode(node);

        // Refresh the widgets
        populateWidgets();
    }
    else
    {
        // conversation entityclass was not found
        gtkutil::MessageBox::ShowError(
			(boost::format(_("Unable to create conversation Entity: class '%s' not found."))
				% CONVERSATION_ENTITY_CLASS).str(),
            GlobalMainFrame().getTopLevelWindow()
        );
    }
}

// Delete entity button
void ConversationDialog::onDeleteEntity()
{
	// Get the Node* from the tree model and remove it from the scenegraph
	Gtk::TreeModel::iterator iter = _entityView->get_selection()->get_selected();

	if (iter)
	{
		// Get the name of the selected entity
		std::string name = Glib::ustring((*iter)[_convEntityColumns.entityName]);

		// Instruct the ConversationEntity to delete its world node, and then
		// remove it from the map
		_entities[name]->deleteWorldNode();
		_entities.erase(name);

		// Update the widgets to remove the selection from the list
		populateWidgets();
	}
}

// Callback for current conversation selection changed
void ConversationDialog::onConversationSelectionChanged()
{
	// Get the selection
	_currentConversation = _convView->get_selection()->get_selected();

	if (_currentConversation)
	{
		// Enable the edit and delete buttons
		_editConvButton->set_sensitive(true);
		_delConvButton->set_sensitive(true);
	}
	else
	{
		// Disable the edit and delete buttons
		_editConvButton->set_sensitive(false);
		_delConvButton->set_sensitive(false);
	}
}

void ConversationDialog::onAddConversation()
{
	// Add a new conversation to the ConversationEntity and refresh the list store
	_curEntity->second->addConversation();
	refreshConversationList();
}

void ConversationDialog::onEditConversation()
{
	// Retrieve the index of the current conversation
	int index = (*_currentConversation)[_convColumns.index];

	conversation::Conversation& conv = _curEntity->second->getConversation(index);

	// Display the edit dialog, blocks on construction
	ConversationEditor editor(getRefPtr(), conv);

	// Repopulate the conversation list
	refreshConversationList();
}

void ConversationDialog::onDeleteConversation()
{
	// Get the index of the current conversation
	int index = (*_currentConversation)[_convColumns.index];

	// Tell the ConversationEntity to delete this conversation
	_curEntity->second->deleteConversation(index);

	// Repopulate the conversation list
	refreshConversationList();
}

void ConversationDialog::onClearConversations()
{
	// Clear the entity and refresh the list
	_curEntity->second->clearConversations();
	refreshConversationList();
}

} // namespace ui
