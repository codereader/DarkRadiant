#include "AIVocalSetPropertyEditor.h"

#include "i18n.h"
#include "ieclass.h"
#include "iuimanager.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/artprov.h>

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

	// Create the browse button
	wxButton* browseButton = new wxButton(_widget, wxID_ANY, _("Select Vocal Set..."));
	browseButton->SetBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "icon_sound.png"));
	browseButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AIVocalSetPropertyEditor::onChooseButton), NULL, this);
}

wxPanel* AIVocalSetPropertyEditor::getWidget()
{
	return _widget;
}

IPropertyEditorPtr AIVocalSetPropertyEditor::createNew(wxWindow* parent, Entity* entity,
	const std::string& key, const std::string& options)
{
	return IPropertyEditorPtr(new AIVocalSetPropertyEditor(parent, entity, key, options));
}

void AIVocalSetPropertyEditor::onChooseButton(wxCommandEvent& ev)
{
	// Construct a new vocal set chooser dialog
	AIVocalSetChooserDialog dialog;

	dialog.setSelectedVocalSet(_entity->getKeyValue(DEF_VOCAL_SET_KEY));

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIVocalSetChooserDialog::RESULT_OK)
	{
		_entity->setKeyValue(DEF_VOCAL_SET_KEY, dialog.getSelectedVocalSet());
	}
}

std::string AIVocalSetPropertyEditor::runDialog(Entity* entity, const std::string& key)
{
	// Construct a new vocal set chooser dialog
	AIVocalSetChooserDialog dialog;

	std::string oldValue = entity->getKeyValue(DEF_VOCAL_SET_KEY);
	dialog.setSelectedVocalSet(oldValue);

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIVocalSetChooserDialog::RESULT_OK)
	{
		return dialog.getSelectedVocalSet();
	}

	return oldValue;
}

} // namespace ui
