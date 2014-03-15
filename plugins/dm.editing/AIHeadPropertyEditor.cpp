#include "AIHeadPropertyEditor.h"

#include "i18n.h"
#include "ieclass.h"
#include "iuimanager.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/artprov.h>

#include "AIHeadChooserDialog.h"

namespace ui
{

AIHeadPropertyEditor::AIHeadPropertyEditor() :
	_widget(NULL),
	_entity(NULL)
{}

AIHeadPropertyEditor::AIHeadPropertyEditor(wxWindow* parent, Entity* entity, const std::string& key, const std::string& options) :
	_entity(entity)
{
	// Construct the main widget (will be managed by the base class)
	_widget = new wxPanel(parent, wxID_ANY);

	// Create the browse button
	wxButton* browseButton = new wxButton(_widget, wxID_ANY, _("Choose AI head..."));
	browseButton->SetBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "icon_model.png"));
	browseButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AIHeadPropertyEditor::onChooseButton), NULL, this);
}

wxPanel* AIHeadPropertyEditor::getWidget()
{
	return _widget;
}

IPropertyEditorPtr AIHeadPropertyEditor::createNew(wxWindow* parent, Entity* entity,
	const std::string& key, const std::string& options)
{
	return IPropertyEditorPtr(new AIHeadPropertyEditor(parent, entity, key, options));
}

void AIHeadPropertyEditor::onChooseButton(wxCommandEvent& ev)
{
	// Construct a new head chooser dialog
	AIHeadChooserDialog dialog;

	dialog.setSelectedHead(_entity->getKeyValue(DEF_HEAD_KEY));

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIHeadChooserDialog::RESULT_OK)
	{
		_entity->setKeyValue(DEF_HEAD_KEY, dialog.getSelectedHead());
	}
}

std::string AIHeadPropertyEditor::runDialog(Entity* entity, const std::string& key)
{
	// Construct a new head chooser dialog
	AIHeadChooserDialog dialog;

	std::string prevHead = entity->getKeyValue(key);
	dialog.setSelectedHead(prevHead);

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIHeadChooserDialog::RESULT_OK)
	{
		return dialog.getSelectedHead();
	}
	else
	{
		return prevHead;
	}
}

} // namespace ui
