#include "CommandArgumentItem.h"

#include "ianimationchooser.h"
#include "idialogmanager.h"
#include "iresourcechooser.h"
#include "iuimanager.h"
#include "i18n.h"
#include "string/convert.h"
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>

#include "wxutil/ChoiceHelper.h"

#include <fmt/format.h>

#include "CommandEditor.h"
#include "ActorNodeFinder.h"

namespace ui
{

namespace
{
	const char* const FOLDER_ICON = "folder16.png";
}

CommandArgumentItem::CommandArgumentItem(CommandEditor& owner, wxWindow* parent,
		const conversation::ArgumentInfo& argInfo) :
	_owner(owner),
	_argInfo(argInfo)
{
	// Pack the label into an eventbox
	_labelBox = new wxStaticText(parent, wxID_ANY, _argInfo.title + ":");
	_labelBox->SetToolTip(argInfo.description);

	// Pack the description widget into an eventbox
	_descBox = new wxStaticText(parent, wxID_ANY, "?");
	_descBox->SetFont(_descBox->GetFont().Bold());
	_descBox->SetToolTip(argInfo.description);
}

// Retrieve the label widget
wxWindow* CommandArgumentItem::getLabelWidget()
{
	return _labelBox;
}

wxWindow* CommandArgumentItem::getHelpWidget()
{
	return _descBox;
}

// StringArgument
StringArgument::StringArgument(CommandEditor& owner, wxWindow* parent,
		const conversation::ArgumentInfo& argInfo) :
	CommandArgumentItem(owner, parent, argInfo)
{
	_entry = new wxTextCtrl(parent, wxID_ANY);
}

wxWindow* StringArgument::getEditWidget()
{
	return _entry;
}

std::string StringArgument::getValue()
{
	return _entry->GetValue().ToStdString();
}

void StringArgument::setValueFromString(const std::string& value)
{
	_entry->SetValue(value);
}

// Boolean argument
BooleanArgument::BooleanArgument(CommandEditor& owner, wxWindow* parent, const conversation::ArgumentInfo& argInfo) :
	 CommandArgumentItem(owner, parent, argInfo)
{
	_checkButton = new wxCheckBox(parent, wxID_ANY, argInfo.title);
}

wxWindow* BooleanArgument::getEditWidget()
{
	return _checkButton;
}

std::string BooleanArgument::getValue()
{
	return _checkButton->GetValue() ? "1" : "";
}

void BooleanArgument::setValueFromString(const std::string& value)
{
	_checkButton->SetValue(value == "1");
}

// Actor Argument
ActorArgument::ActorArgument(CommandEditor& owner, 
		wxWindow* parent,
		const conversation::ArgumentInfo& argInfo,
		const conversation::Conversation::ActorMap& actors) :
	CommandArgumentItem(owner, parent, argInfo)
{
	_comboBox = new wxChoice(parent, wxID_ANY);

	// Fill the actor list
	conversation::Conversation::ActorMap dummy = actors;
	for (conversation::Conversation::ActorMap::const_iterator i = dummy.begin();
		 i != dummy.end(); ++i)
	{
		std::string actorStr = fmt::format(_("Actor {0:d} ({1})"), i->first, i->second);

		// Store the actor ID into a client data object and pass it along
		_comboBox->Append(actorStr, new wxStringClientData(string::to_string(i->first)));
	}
}

std::string ActorArgument::getValue()
{
	return string::to_string(wxutil::ChoiceHelper::GetSelectionId(_comboBox));
}

void ActorArgument::setValueFromString(const std::string& value)
{
	wxutil::ChoiceHelper::SelectItemByStoredId(_comboBox, string::convert<int>(value, wxNOT_FOUND));
}

wxWindow* ActorArgument::getEditWidget()
{
	return _comboBox;
}

SoundShaderArgument::SoundShaderArgument(CommandEditor& owner, 
		wxWindow* parent, const conversation::ArgumentInfo& argInfo) :
	StringArgument(owner, parent, argInfo)
{
	_soundShaderPanel = new wxPanel(parent);

	wxBoxSizer* shaderHBox = new wxBoxSizer(wxHORIZONTAL);
	_soundShaderPanel->SetSizer(shaderHBox);

	_entry->SetMinSize(wxSize(100, -1));
	_entry->Reparent(_soundShaderPanel);

	shaderHBox->Add(_entry, 1, wxEXPAND);

	// Create the icon button to open the ShaderChooser
	wxButton* selectShaderButton = new wxBitmapButton(_soundShaderPanel, wxID_ANY,
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));

	selectShaderButton->SetToolTip(_("Browse Sound Shaders"));

	selectShaderButton->Bind(wxEVT_BUTTON, [&](wxCommandEvent& ev)
	{
		pickSoundShader();
	});

	shaderHBox->Add(selectShaderButton, 0, wxLEFT, 6);
}

wxWindow* SoundShaderArgument::getEditWidget()
{
	return _soundShaderPanel;
}

std::string SoundShaderArgument::getValue()
{
	return _entry->GetValue().ToStdString();
}

void SoundShaderArgument::setValueFromString(const std::string& value)
{
	_entry->SetValue(value);
}

void SoundShaderArgument::pickSoundShader()
{
	IResourceChooser* chooser = GlobalDialogManager().createSoundShaderChooser(wxGetTopLevelParent(_entry));

	std::string picked = chooser->chooseResource(getValue());

	if (!picked.empty())
	{
		setValueFromString(picked);
	}

	chooser->destroyDialog();
}

AnimationArgument::AnimationArgument(CommandEditor& owner, 
		wxWindow* parent, const conversation::ArgumentInfo& argInfo) :
	StringArgument(owner, parent, argInfo)
{
	_animPanel = new wxPanel(parent);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	_animPanel->SetSizer(hbox);

	_entry->SetMinSize(wxSize(100, -1));
	_entry->Reparent(_animPanel);

	hbox->Add(_entry, 1, wxEXPAND);

	// Create the icon button to open the
	wxButton* selectButton = new wxBitmapButton(_animPanel, wxID_ANY,
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));

	selectButton->SetToolTip(_("Browse Animations"));

	selectButton->Bind(wxEVT_BUTTON, [&](wxCommandEvent& ev)
	{
		pickAnimation();
	});

	hbox->Add(selectButton, 0, wxLEFT, 6);
}

wxWindow* AnimationArgument::getEditWidget()
{
	return _animPanel;
}

std::string AnimationArgument::getValue()
{
	return _entry->GetValue().ToStdString();
}

void AnimationArgument::setValueFromString(const std::string& value)
{
	_entry->SetValue(value);
}

void AnimationArgument::pickAnimation()
{
	// Find out which actor we're talking about
	int actorId = _owner.getCommand().actor;
	std::string preselectModel = std::string();

	if (actorId != -1 && _owner.getConversation().actors.find(actorId) != _owner.getConversation().actors.end())
	{
		std::string actorName = _owner.getConversation().actors.find(actorId)->second;

		// Try to find the entity in the current map
		scene::ActorNodeFinder finder(actorName);
		GlobalSceneGraph().root()->traverse(finder);

		if (finder.getFoundNode())
		{
			// Found the corresponding entity, get the model name
			Entity* entity = Node_getEntity(finder.getFoundNode());
			assert(entity != nullptr);

			preselectModel = entity->getKeyValue("model");
		}
	}

	IAnimationChooser* chooser = GlobalDialogManager().createAnimationChooser(wxGetTopLevelParent(_entry));

	IAnimationChooser::Result result = chooser->runDialog(preselectModel, getValue());

	if (!result.cancelled())
	{
		setValueFromString(result.anim);
	}

	chooser->destroyDialog();
}

} // namespace ui
