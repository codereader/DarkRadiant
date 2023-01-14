#include "ConversationDialog.h"

#include "i18n.h"
#include "iregistry.h"
#include "iundo.h"
#include "ieclass.h"
#include "ui/imainframe.h"
#include "iscenegraph.h"
#include "string/string.h"

#include "wxutil/dataview/TreeModel.h"
#include "wxutil/dialog/MessageBox.h"

#include "RandomOrigin.h"
#include "ConversationEntityFinder.h"
#include "ConversationEditor.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include <fmt/format.h>
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
	_entityView(nullptr),
	_convList(new wxutil::TreeModel(_convColumns, true)),
	_convView(nullptr),
	_addConvButton(nullptr),
	_editConvButton(nullptr),
	_deleteConvButton(nullptr),
	_moveUpConvButton(nullptr),
	_moveDownConvButton(nullptr),
	_clearConvButton(nullptr),
	_addEntityButton(nullptr),
	_deleteEntityButton(nullptr)
{
	// Create the widgets
	populateWindow();

	FitToScreen(0.3f, 0.5f);
}

void ConversationDialog::populateWindow()
{
	loadNamedPanel(this, "ConvDialogMainPanel");

	wxPanel* entityPanel = findNamedObject<wxPanel>(this, "ConvDialogEntityPanel");

	// Entity Tree View
	_entityView = wxutil::TreeView::CreateWithModel(entityPanel, _entityList.get(), wxDV_NO_HEADER);
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

	_convView = wxutil::TreeView::CreateWithModel(convPanel, _convList.get());

	// Key and value text columns
	_convView->AppendTextColumn("#", _convColumns.index.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
	_convView->AppendTextColumn(_("Name"), _convColumns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_convView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ConversationDialog::onConversationSelectionChanged, this);
    _convView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [this](auto&) { editSelectedConversation(); });

	convPanel->GetSizer()->Add(_convView, 1, wxEXPAND);

	convPanel->Enable(false);

	// Wire up button signals
	_addConvButton = findNamedObject<wxButton>(this, "ConvDialogAddConvButton");
	_addConvButton->Bind(wxEVT_BUTTON, &ConversationDialog::onAddConversation, this);
	_addConvButton->Enable(false); // not enabled without selection

	_editConvButton = findNamedObject<wxButton>(this, "ConvDialogEditConvButton");
	_editConvButton->Bind(wxEVT_BUTTON, &ConversationDialog::onEditConversation, this);
	_editConvButton->Enable(false); // not enabled without selection

	_deleteConvButton = findNamedObject<wxButton>(this, "ConvDialogDeleteConvButton");
	_deleteConvButton->Bind(wxEVT_BUTTON, &ConversationDialog::onDeleteConversation, this);
	_deleteConvButton->Enable(false); // not enabled without selection

	_moveUpConvButton = findNamedObject<wxButton>(this, "ConvDialogMoveUpConvButton");
	_moveUpConvButton->Bind(wxEVT_BUTTON, &ConversationDialog::onMoveConversationUpOrDown, this);
	_moveUpConvButton->Enable(false); // not enabled without selection

	_moveDownConvButton = findNamedObject<wxButton>(this, "ConvDialogMoveDownConvButton");
	_moveDownConvButton->Bind(wxEVT_BUTTON, &ConversationDialog::onMoveConversationUpOrDown, this);
	_moveDownConvButton->Enable(false); // not enabled without selection

	_clearConvButton = findNamedObject<wxButton>(this, "ConvDialogClearConvButton");
	_clearConvButton->Bind(wxEVT_BUTTON, &ConversationDialog::onClearConversations, this);
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
	_curEntity->second->populateListStore(*_convList, _convColumns);

	// If there is at least one conversation, make the Clear button available
	_clearConvButton->Enable(!_curEntity->second->isEmpty());

	handleConversationSelectionChange();
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
	populateWidgets();

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
		_moveUpConvButton->Enable(false);
		_moveDownConvButton->Enable(false);
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
        IEntityNodePtr node(GlobalEntityModule().createEntity(eclass));

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
			fmt::format(_("Unable to create conversation Entity: class '{0}' not found."),
				CONVERSATION_ENTITY_CLASS)
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
	handleConversationSelectionChange();
}

void ConversationDialog::handleConversationSelectionChange()
{
	// Get the selection
	_currentConversation = _convView->GetSelection();
	int index = getSelectedConvIndex();

	if (_currentConversation.IsOk())
	{
		// Enable the edit and delete buttons
		_editConvButton->Enable(true);
		_deleteConvButton->Enable(true);

		_moveUpConvButton->Enable(index > 1);
		_moveDownConvButton->Enable(index < _curEntity->second->getHighestIndex());
	}
	else
	{
		// Disable all manipulation buttons
		_editConvButton->Enable(false);
		_deleteConvButton->Enable(false);
		_moveUpConvButton->Enable(false);
		_moveDownConvButton->Enable(false);
	}
}

void ConversationDialog::onAddConversation(wxCommandEvent& ev)
{
	// Add a new conversation to the ConversationEntity and refresh the list store
	int newIndex = _curEntity->second->addConversation();

	refreshConversationList();

	selectConvByIndex(newIndex);
}

int ConversationDialog::getSelectedConvIndex()
{
	if (!_currentConversation.IsOk())
	{
		return -1;
	}

	// Retrieve the index of the current conversation
	wxutil::TreeModel::Row row(_currentConversation, *_convList);
	return row[_convColumns.index].getInteger();
}

void ConversationDialog::selectConvByIndex(int index)
{
	auto item = _convList->FindInteger(index, _convColumns.index);
	_convView->Select(item);

	handleConversationSelectionChange();
}

void ConversationDialog::editSelectedConversation()
{
    int index = getSelectedConvIndex();

    if (index == -1) return;

    auto& conv = _curEntity->second->getConversation(index);

    // Display the edit dialog, blocks on construction
    auto editor = new ConversationEditor(this, conv);

    editor->ShowModal();
    editor->Destroy();

    // Repopulate the conversation list
    refreshConversationList();
}

void ConversationDialog::onEditConversation(wxCommandEvent& ev)
{
    editSelectedConversation();
}

void ConversationDialog::onDeleteConversation(wxCommandEvent& ev)
{
	int index = getSelectedConvIndex();

	// Tell the ConversationEntity to delete this conversation
	_curEntity->second->deleteConversation(index);

	// Repopulate the conversation list
	refreshConversationList();
}

void ConversationDialog::onMoveConversationUpOrDown(wxCommandEvent& ev)
{
	bool moveUp = ev.GetEventObject() == _moveUpConvButton;
	int index = getSelectedConvIndex();

	int newIndex = _curEntity->second->moveConversation(index, moveUp);

	// Repopulate the conversation list
	refreshConversationList();

	// Try to select the moved item
	selectConvByIndex(newIndex);
}

void ConversationDialog::onClearConversations(wxCommandEvent& ev)
{
	// Clear the entity and refresh the list
	_curEntity->second->clearConversations();
	refreshConversationList();
}

} // namespace ui
