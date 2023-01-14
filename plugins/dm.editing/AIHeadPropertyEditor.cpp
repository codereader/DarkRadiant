#include "AIHeadPropertyEditor.h"

#include "i18n.h"
#include "ieclass.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/button.h>
#include "wxutil/Bitmap.h"
#include <wx/sizer.h>

#include "AIHeadChooserDialog.h"

namespace ui
{

AIHeadPropertyEditor::AIHeadPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key) :
    _entities(entities),
    _key(key)
{
	// Construct the main widget (will be managed by the base class)
	_widget = new wxPanel(parent, wxID_ANY);
	_widget->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Create the browse button
	wxButton* browseButton = new wxButton(_widget, wxID_ANY, _("Choose AI head..."));
	browseButton->SetBitmap(wxutil::GetLocalBitmap("icon_model.png"));
	browseButton->Bind(wxEVT_BUTTON, &AIHeadPropertyEditor::onChooseButton, this);

	_widget->GetSizer()->Add(browseButton, 0, wxALIGN_CENTER_VERTICAL);
}

AIHeadPropertyEditor::~AIHeadPropertyEditor()
{
	if (_widget != nullptr)
	{
		_widget->Destroy();
	}
}

wxPanel* AIHeadPropertyEditor::getWidget()
{
	return _widget;
}

void AIHeadPropertyEditor::updateFromEntities()
{
	// nothing to do
}

sigc::signal<void(const std::string&, const std::string&)>& AIHeadPropertyEditor::signal_keyValueApplied()
{
    return _sigKeyValueApplied;
}

IPropertyEditor::Ptr AIHeadPropertyEditor::CreateNew(wxWindow* parent, IEntitySelection& entities,
    const ITargetKey::Ptr& key)
{
	return std::make_shared<AIHeadPropertyEditor>(parent, entities, key);
}

void AIHeadPropertyEditor::onChooseButton(wxCommandEvent& ev)
{
	// Construct a new head chooser dialog
	AIHeadChooserDialog* dialog = new AIHeadChooserDialog;

	dialog->setSelectedHead(_entities.getSharedKeyValue(DEF_HEAD_KEY, true));

	// Show and block
	if (dialog->ShowModal() == wxID_OK)
	{
        auto selectedHead = dialog->getSelectedHead();

        _entities.foreachEntity([&](const IEntityNodePtr& entity)
        {
            entity->getEntity().setKeyValue(DEF_HEAD_KEY, selectedHead);
        });

        signal_keyValueApplied().emit(DEF_HEAD_KEY, selectedHead);
	}

	dialog->Destroy();
}

std::string AIHeadEditorDialogWrapper::runDialog(Entity* entity, const std::string& key)
{
    // Construct a new head chooser dialog
    AIHeadChooserDialog* dialog = new AIHeadChooserDialog;

    std::string prevHead = entity->getKeyValue(key);
    dialog->setSelectedHead(prevHead);

    // Show and block
    std::string selected = prevHead;

    if (dialog->ShowModal() == wxID_OK)
    {
        selected = dialog->getSelectedHead();
    }

    dialog->Destroy();

    return selected;
}

} // namespace ui
