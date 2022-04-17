#include "EffectEditor.h"

#include "iscenegraph.h"
#include "iregistry.h"
#include "entitylib.h"
#include "wxutil/dataview/TreeModel.h"
#include "ResponseEditor.h"
#include <iostream>
#include "i18n.h"
#include "gamelib.h"

#include "wxutil/ChoiceHelper.h"
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Edit Response Effect");
	// The name of the _SELF entity as parsed by the response scripts
	const char* const GKEY_ENTITY_SELF = "/stimResponseSystem/selfEntity";
}

EffectEditor::EffectEditor(wxWindow* parent,
						   StimResponse& response,
						   const unsigned int effectIndex,
						   StimTypes& stimTypes,
						   ui::ResponseEditor& editor) :
	DialogBase(_(WINDOW_TITLE), parent),
	_argTable(NULL),
	_response(response),
	_effectIndex(effectIndex),
	_backup(_response.getResponseEffect(_effectIndex)),
	_editor(editor),
	_stimTypes(stimTypes)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Create the widgets
	populateWindow();

	// Search the scenegraph for entities
	populateEntityListStore();

	// Initialise the widgets with the current data
	ResponseEffect& effect = _response.getResponseEffect(_effectIndex);

	// Setup the selectionfinder to search for the name string
	wxutil::ChoiceHelper::SelectItemByStoredString(_effectTypeCombo, effect.getName());

	_stateToggle->SetValue(effect.isActive());

	// Parse the argument types from the effect and create the widgets
	createArgumentWidgets(effect);

	Layout();
	Fit();
}

int EffectEditor::ShowModal()
{
	int returnCode = DialogBase::ShowModal();

	if (returnCode == wxID_OK)
	{
		save();
	}
	else
	{
		revert();
	}

	return returnCode;
}

void EffectEditor::populateWindow()
{
	// Create the overall vbox
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

	// Pack everything into the main window
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxALL, 12);

	_effectTypeCombo = new wxChoice(this, wxID_ANY);

	// Connect the signal to get notified of further changes
	_effectTypeCombo->Connect(wxEVT_CHOICE, wxCommandEventHandler(EffectEditor::onEffectTypeChange), NULL, this);

	// Retrieve the map from the ResponseEffectTypes object
	ResponseEffectTypeMap& effectTypes =
		ResponseEffectTypes::Instance().getMap();

	// Now populate the list store with all the possible effect types
	for (ResponseEffectTypeMap::iterator i = effectTypes.begin();
		  i != effectTypes.end(); ++i)
	{
		// Store the effect name as client data
		_effectTypeCombo->Append(i->second->getAttributeValue("editor_caption"), 
			new wxStringClientData(i->first));
	}

	wxBoxSizer* effectHBox = new wxBoxSizer(wxHORIZONTAL);
		
	wxStaticText* effectLabel = new wxStaticText(this, wxID_ANY, _("Effect:"));

	effectHBox->Add(effectLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	effectHBox->Add(_effectTypeCombo);

	vbox->Add(effectHBox, 0, wxBOTTOM | wxEXPAND, 6);

	_stateToggle = new wxCheckBox(this, wxID_ANY, _("Active"));
	_stateToggle->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(EffectEditor::onStateToggle), NULL, this);

	vbox->Add(_stateToggle, 0, wxBOTTOM, 6);

	wxStaticText* argLabel = new wxStaticText(this, wxID_ANY, _("Arguments"));
	argLabel->SetFont(argLabel->GetFont().Bold());
	vbox->Add(argLabel, 0, wxBOTTOM, 6);

	_argTable = new wxFlexGridSizer(3, 6, 12);
	vbox->Add(_argTable, 0, wxEXPAND | wxLEFT, 12);
}

void EffectEditor::effectTypeChanged()
{
	std::string newEffectName("");

	// Get the currently selected effect name from the combo box
	if (_effectTypeCombo->GetSelection() != wxNOT_FOUND)
	{
		wxStringClientData* data = dynamic_cast<wxStringClientData*>(
			_effectTypeCombo->GetClientObject(_effectTypeCombo->GetSelection()));
		assert(data != NULL);

		newEffectName = data->GetData().ToStdString();
	}

	// Get the effect
	ResponseEffect& effect = _response.getResponseEffect(_effectIndex);

	// Set the name of the effect and trigger an argument list update
	effect.setName(newEffectName);
	effect.clearArgumentList();
	effect.buildArgumentList();

	// Rebuild the argument list basing on this new effect
	createArgumentWidgets(effect);
}

void EffectEditor::createArgumentWidgets(ResponseEffect& effect)
{
	ResponseEffect::ArgumentList& list = effect.getArguments();

	// Remove all possible previous items from the list
	_argumentItems.clear();

	// Clear the old table
	_argTable->DeleteWindows();

	for (ResponseEffect::ArgumentList::iterator i = list.begin();
		 i != list.end(); ++i)
	{
		//int index = i->first;
		ResponseEffect::Argument& arg = i->second;
		ArgumentItemPtr item;

		if (arg.type == "s")
		{
			// Create a new string argument item
			item = ArgumentItemPtr(new StringArgument(this, arg));
		}
		else if (arg.type == "f")
		{
			// Create a new string argument item
			item = ArgumentItemPtr(new FloatArgument(this, arg));
		}
		else if (arg.type == "v")
		{
			// Create a new vector argument item
			item = ArgumentItemPtr(new VectorArgument(this, arg));
		}
		else if (arg.type == "e") 
		{
			// Create a new string argument item
			item = ArgumentItemPtr(new EntityArgument(this, arg, _entityChoices));
		}
		else if (arg.type == "b")
		{
			// Create a new bool item
			item = ArgumentItemPtr(new BooleanArgument(this, arg));
		}
		else if (arg.type == "t")
		{
			// Create a new stim type item
			item = ArgumentItemPtr(new StimTypeArgument(this, arg, _stimTypes));
		}

		if (item != NULL)
		{
			_argumentItems.push_back(item);

			if (arg.type != "b")
			{
				// The label
				_argTable->Add(item->getLabelWidget(), 0, wxALIGN_CENTER_VERTICAL);

				// The edit widgets
				_argTable->Add(item->getEditWidget(), 1, wxEXPAND);
			}
			else 
			{
				// The label
				_argTable->Add(new wxStaticText(this, wxID_ANY, ""), 0, wxALIGN_CENTER_VERTICAL);

				// The edit widgets
				_argTable->Add(item->getEditWidget(), 1, wxEXPAND);
			}

			// The help widgets
			_argTable->Add(item->getHelpWidget(), 0, wxALIGN_CENTER_VERTICAL);
		}
	}

	_argTable->Layout();
	Layout();
	Fit();
}

void EffectEditor::save()
{
	// Request each argument item to save the content into the argument
	for (std::size_t i = 0; i < _argumentItems.size(); ++i)
	{
		_argumentItems[i]->save();
	}

	// Call the update routine of the parent editor
	_editor.update();
}

// Traverse the scenegraph to populate the tree model
void EffectEditor::populateEntityListStore()
{
	_entityChoices.Clear();

	std::string selfEntity = game::current::getValue<std::string>(GKEY_ENTITY_SELF);

	// Append the name to the list store
	_entityChoices.Add(selfEntity);

	// Create a scenegraph walker to traverse the graph
	class EntityFinder :
		public scene::NodeVisitor
	{
		// List to add names to
		wxArrayString& _list;
	public:
		// Constructor
		EntityFinder(wxArrayString& list) :
			_list(list)
		{}

		// Visit function
		bool pre(const scene::INodePtr& node)
		{
			Entity* entity = Node_getEntity(node);

			if (entity != NULL)
			{
				// Get the entity name and append it to the list store
				_list.Add(entity->getKeyValue("name"));

				return false; // don't traverse children if entity found
	        }

	        return true; // traverse children otherwise
		}
	} finder(_entityChoices);

    GlobalSceneGraph().root()->traverse(finder);
}

void EffectEditor::revert()
{
	_response.getResponseEffect(_effectIndex) = _backup;
}

void EffectEditor::onStateToggle(wxCommandEvent& ev)
{
	_response.getResponseEffect(_effectIndex).setActive(_stateToggle->GetValue());
}

void EffectEditor::onEffectTypeChange(wxCommandEvent& ev)
{
	effectTypeChanged();
}

} // namespace ui
