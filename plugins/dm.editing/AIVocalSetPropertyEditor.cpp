#include "AIVocalSetPropertyEditor.h"

#include "i18n.h"
#include "ieclass.h"
#include "iuimanager.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/artprov.h>
#include <wx/sizer.h>

#include "AIVocalSetChooserDialog.h"

namespace ui
{

AIVocalSetPropertyEditor::AIVocalSetPropertyEditor() :
	_widget(NULL),
	_entity(NULL)
{}

AIVocalSetPropertyEditor::AIVocalSetPropertyEditor(wxWindow* parent, Entity* entity, const std::string& key, const std::string& options) :
	_entity(entity)
{
	// Construct the main widget (will be managed by the base class)
	_widget = new wxPanel(parent, wxID_ANY);
	_widget->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Create the browse button
	wxButton* browseButton = new wxButton(_widget, wxID_ANY, _("Select Vocal Set..."));
	browseButton->SetBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "icon_sound.png"));
	browseButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AIVocalSetPropertyEditor::onChooseButton), NULL, this);

	_widget->GetSizer()->Add(browseButton, 0, wxALIGN_CENTER_VERTICAL);
}

AIVocalSetPropertyEditor::~AIVocalSetPropertyEditor()
{
	if (_widget != NULL)
	{
		_widget->Destroy();
	}
}

wxPanel* AIVocalSetPropertyEditor::getWidget()
{
	return _widget;
}

void AIVocalSetPropertyEditor::updateFromEntity()
{
	// Nothing to do
}

IPropertyEditorPtr AIVocalSetPropertyEditor::createNew(wxWindow* parent, Entity* entity,
	const std::string& key, const std::string& options)
{
	return IPropertyEditorPtr(new AIVocalSetPropertyEditor(parent, entity, key, options));
}

void AIVocalSetPropertyEditor::onChooseButton(wxCommandEvent& ev)
{
	// Construct a new vocal set chooser dialog
	AIVocalSetChooserDialog* dialog = new AIVocalSetChooserDialog;

	dialog->setSelectedVocalSet(_entity->getKeyValue(DEF_VOCAL_SET_KEY));

	// Show and block
	if (dialog->ShowModal() == wxID_OK)
	{
		_entity->setKeyValue(DEF_VOCAL_SET_KEY, dialog->getSelectedVocalSet());
	}

	dialog->Destroy();
}

std::string AIVocalSetPropertyEditor::runDialog(Entity* entity, const std::string& key)
{
	// Construct a new vocal set chooser dialog
	AIVocalSetChooserDialog* dialog = new AIVocalSetChooserDialog;

	std::string oldValue = entity->getKeyValue(DEF_VOCAL_SET_KEY);
	dialog->setSelectedVocalSet(oldValue);

	// Show and block
	std::string rv = oldValue;

	if (dialog->ShowModal() == wxID_OK)
	{
		rv = dialog->getSelectedVocalSet();
	}

	dialog->Destroy();

	return rv;
}

} // namespace ui
