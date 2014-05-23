#include "CommandArgumentItem.h"

#include "i18n.h"
#include "string/convert.h"
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "gtkutil/ChoiceHelper.h"

#include <boost/format.hpp>

namespace ui
{

CommandArgumentItem::CommandArgumentItem(wxWindow* parent, 
		const conversation::ArgumentInfo& argInfo) :
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
StringArgument::StringArgument(wxWindow* parent, 
		const conversation::ArgumentInfo& argInfo) :
	CommandArgumentItem(parent, argInfo)
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
BooleanArgument::BooleanArgument(wxWindow* parent, const conversation::ArgumentInfo& argInfo) :
	 CommandArgumentItem(parent, argInfo)
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
ActorArgument::ActorArgument(wxWindow* parent, 
		const conversation::ArgumentInfo& argInfo,
		const conversation::Conversation::ActorMap& actors) :
	CommandArgumentItem(parent, argInfo)
{
	_comboBox = new wxChoice(parent, wxID_ANY);

	// Fill the actor list
	conversation::Conversation::ActorMap dummy = actors;
	for (conversation::Conversation::ActorMap::const_iterator i = dummy.begin();
		 i != dummy.end(); ++i)
	{
		std::string actorStr = (boost::format(_("Actor %d (%s)")) % i->first % i->second).str();

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

} // namespace ui
