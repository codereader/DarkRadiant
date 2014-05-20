#include "ConversationDialog.h"

#include "i18n.h"
#include "iregistry.h"
#include "iundo.h"
#include "ieclass.h"
#include "imainframe.h"
#include "iscenegraph.h"
#include "string/string.h"

#include "gtkutil/TreeModel.h"
#include "gtkutil/dialog/MessageBox.h"

#include "RandomOrigin.h"
#include "ConversationEntityFinder.h"
#include "ConversationEditor.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include <boost/format.hpp>
#include <iostream>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Conversation Editor");
	const std::string CONVERSATION_ENTITY_CLASS = "atdm:conversation_info";
}

ConversationDialog::ConversationDialog() :
	DialogBase(_(WINDOW_TITLE)),
	_entityList(new wxutil::TreeModel(_convEntityColumns, true)),
	_entityView(NULL),
	_convList(new wxutil::TreeModel(_convColumns, true)),
	_convView(NULL)
{
	// Create the widgets
	populateWindow();

	FitToScreen(0.3f, 0.5f);
}

void ConversationDialog::populateWindow()
{
	wxPanel* mainPanel = loadNamedPanel(this, "ConvDialogMainPanel");

	wxPanel* entityPanel = findNamedObject<wxPanel>(this, "ConvDialogEntityPanel");

	// Entity Tree View
	_entityView = wxutil::TreeView::CreateWithModel(entityPanel, _entityList, wxDV_NO_HEADER);
	entityPanel->GetSizer()->Add(_entityView, 1, wxEXPAND);

	_entityView->AppendTextColumn("", _convEntityColumns.displayName.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_entityView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(ConversationDialog::onEntitySelectionChanged), NULL, this);

	// Wire up button signals
	_addEntityButton = findNamedObject<wxButton>(this, "ConvDialogAddEntityButton");
	_addEntityButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationDialog::onAddEntity), NULL, this);

	_deleteEntityButton = findNamedObject<wxButton>(this, "ConvDialogDeleteEntityButton");
	_deleteEntityButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationDialog::onDeleteEntity), NULL, this);
	_deleteEntityButton->Enable(false);

	wxPanel* convPanel = findNamedObject<wxPanel>(this, "ConvDialogConversationPanel");

	_convView = wxutil::TreeView::CreateWithModel(convPanel, _convList);

	// Key and value text columns
	_convView->AppendTextColumn("#", _convColumns.index.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
	_convView->AppendTextColumn(_("Name"), _convColumns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_convView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(ConversationDialog::onConversationSelectionChanged), NULL, this);

	convPanel->GetSizer()->Add(_convView, 1, wxEXPAND);

	convPanel->Enable(false);

	// Wire up button signals
	_addConvButton = findNamedObject<wxButton>(this, "ConvDialogAddConvButton");
	_addConvButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationDialog::onAddConversation), NULL, this);
	_addConvButton->Enable(false); // not enabled without selection

	_editConvButton = findNamedObject<wxButton>(this, "ConvDialogEditConvButton");
	_editConvButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationDialog::onEditConversation), NULL, this);
	_editConvButton->Enable(false); // not enabled without selection

	_deleteConvButton =	findNamedObject<wxButton>(this, "ConvDialogDeleteConvButton");
	_deleteConvButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationDialog::onDeleteConversation), NULL, this);
	_deleteConvButton->Enable(false); // not enabled without selection

	_clearConvButton = findNamedObject<wxButton>(this, "ConvDialogClearConvButton");
	_clearConvButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationDialog::onClearConversations), NULL, this);
	_clearConvButton->Enable(false); // requires >0 conversations

	// Boldify a few labels
	makeLabelBold(this, "ConvDialogEntityLabel");
	makeLabelBold(this, "ConvDialogConvLabel");

	// Connect dialog buttons
	findNamedObject<wxButton>(this, "ConvDialogCancelButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ConversationDialog::onCancel), NULL, this);
	findNamedObject<wxButton>(this, "ConvDialogOkButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ConversationDialog::onOK), NULL, this);
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
	_entityList->Clear();
	_convList->Clear();
}

void ConversationDialog::refreshConversationList()
{
	// Clear and refresh the conversation list
	_convList->Clear();
	_curEntity->second->populateListStore(_convList, _convColumns);

	// If there is at least one conversation, make the Clear button available
	_clearConvButton->Enable(!_curEntity->second->isEmpty());
}

void ConversationDialog::populateWidgets()
{
	// First clear the data
	clear();

	// Use an ConversationEntityFinder to walk the map and add any conversation
	// entities to the liststore and entity map
	conversation::ConversationEntityFinder finder(
		_entityList,
		_convEntityColumns,
		_entities,
		CONVERSATION_ENTITY_CLASS
	);

	GlobalSceneGraph().root()->traverseChildren(finder);

	updateConversationPanelSensitivity();
}

int ConversationDialog::ShowModal()
{
	int returnCode = DialogBase::ShowModal();

	if (returnCode == wxID_OK)
	{
		save();
	}
	
	return returnCode;
}

// Static command target
void ConversationDialog::ShowDialog(const cmd::ArgumentList& args)
{
	// Construct a new instance, this enters the main loop
	ConversationDialog* editor = new ConversationDialog;

	editor->ShowModal();
	editor->Destroy();
}

void ConversationDialog::updateConversationPanelSensitivity()
{
	// Clear the conversations list
	_convList->Clear();

	wxDataViewItem item = _entityView->GetSelection();

	if (item.IsOk())
    {
		// Get name of the entity and find the corresponding ConversationEntity in the map
		wxutil::TreeModel::Row row(item, *_entityList);

		std::string name = row[_convEntityColumns.entityName];

		// Save the current selection and refresh the conversation list
		_curEntity = _entities.find(name);
		refreshConversationList();

		// Enable the delete button and conversation panel
		_deleteEntityButton->Enable(true);

		findNamedObject<wxPanel>(this, "ConvDialogConversationPanel")->Enable(true);
		_addConvButton->Enable(true);
	}
	else
    {
		// No selection, disable the delete button and clear the conversation panel
		_deleteEntityButton->Enable(false);

        // Disable all the Conversation buttons
		findNamedObject<wxPanel>(this, "ConvDialogConversationPanel")->Enable(false);
		_addConvButton->Enable(false);
		_editConvButton->Enable(false);
		_deleteConvButton->Enable(false);
		_clearConvButton->Enable(false);
	}
}

// Callback for conversation entity selection changed in list box
void ConversationDialog::onEntitySelectionChanged(wxDataViewEvent& ev)
{
	updateConversationPanelSensitivity();
}

void ConversationDialog::onOK(wxCommandEvent& ev)
{
	EndModal(wxID_OK);
}

void ConversationDialog::onCancel(wxCommandEvent& ev)
{
	EndModal(wxID_CANCEL);
}

// Add a new conversations entity button
void ConversationDialog::onAddEntity(wxCommandEvent& ev)
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
        wxutil::Messagebox::ShowError(
			(boost::format(_("Unable to create conversation Entity: class '%s' not found."))
				% CONVERSATION_ENTITY_CLASS).str()
        );
    }
}

// Delete entity button
void ConversationDialog::onDeleteEntity(wxCommandEvent& ev)
{
	// Get the Node* from the tree model and remove it from the scenegraph
	wxDataViewItem item = _entityView->GetSelection();

	if (item)
	{
		// Get the name of the selected entity
		wxutil::TreeModel::Row row(item, *_entityList);
		std::string name = row[_convEntityColumns.entityName];

		// Instruct the ConversationEntity to delete its world node, and then
		// remove it from the map
		_entities[name]->deleteWorldNode();
		_entities.erase(name);

		// Update the widgets to remove the selection from the list
		populateWidgets();
	}
}

// Callback for current conversation selection changed
void ConversationDialog::onConversationSelectionChanged(wxDataViewEvent& ev)
{
	// Get the selection
	_currentConversation = _convView->GetSelection();

	if (_currentConversation.IsOk())
	{
		// Enable the edit and delete buttons
		_editConvButton->Enable(true);
		_deleteConvButton->Enable(true);
	}
	else
	{
		// Disable the edit and delete buttons
		_editConvButton->Enable(false);
		_deleteConvButton->Enable(false);
	}
}

void ConversationDialog::onAddConversation(wxCommandEvent& ev)
{
	// Add a new conversation to the ConversationEntity and refresh the list store
	_curEntity->second->addConversation();
	refreshConversationList();
}

void ConversationDialog::onEditConversation(wxCommandEvent& ev)
{
	// Retrieve the index of the current conversation
	wxutil::TreeModel::Row row(_currentConversation, *_convList);
	int index = row[_convColumns.index].getInteger();

	conversation::Conversation& conv = _curEntity->second->getConversation(index);

	// Display the edit dialog, blocks on construction
	ConversationEditor editor(this, conv);

	// Repopulate the conversation list
	refreshConversationList();
}

void ConversationDialog::onDeleteConversation(wxCommandEvent& ev)
{
	// Get the index of the current conversation
	wxutil::TreeModel::Row row(_currentConversation, *_convList);
	int index = row[_convColumns.index].getInteger();

	// Tell the ConversationEntity to delete this conversation
	_curEntity->second->deleteConversation(index);

	// Repopulate the conversation list
	refreshConversationList();
}

void ConversationDialog::onClearConversations(wxCommandEvent& ev)
{
	// Clear the entity and refresh the list
	_curEntity->second->clearConversations();
	refreshConversationList();
}

} // namespace ui
