#include "ModelPropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "ui/modelselector/ModelSelector.h"
#include "ui/particles/ParticleChooserDialog.h"

#include "i18n.h"
#include "ientity.h"
#include "scenelib.h"
#include "wxutil/dialog/MessageBox.h"

#include <wx/panel.h>
#include <wx/button.h>
#include "wxutil/Bitmap.h"
#include <wx/sizer.h>

#include "ui/common/SkinChooser.h"

namespace ui
{

// Main constructor
ModelPropertyEditor::ModelPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _key(key)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Browse button for models
	wxButton* browseButton = new wxButton(mainVBox, wxID_ANY, _("Choose model..."));
	browseButton->SetBitmap(PropertyEditorFactory::getBitmapFor("model"));
	browseButton->Bind(wxEVT_BUTTON, &ModelPropertyEditor::_onModelButton, this);

	wxButton* skinButton = new wxButton(mainVBox, wxID_ANY, _("Choose skin..."));
	skinButton->SetBitmap(PropertyEditorFactory::getBitmapFor("skin"));
	skinButton->Bind(wxEVT_BUTTON, &ModelPropertyEditor::_onSkinButton, this);

	// Browse button for particles
	wxButton* particleButton = new wxButton(mainVBox, wxID_ANY, _("Choose particle..."));
	particleButton->SetBitmap(wxutil::GetLocalBitmap("particle16.png"));
	particleButton->Bind(wxEVT_BUTTON, &ModelPropertyEditor::_onParticleButton, this);

	// The panel will use the entire height of the editor frame in the entity inspector
	// use vertical centering to position it in the middle
	mainVBox->GetSizer()->Add(browseButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
	mainVBox->GetSizer()->Add(skinButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
	mainVBox->GetSizer()->Add(particleButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
}

void ModelPropertyEditor::_onModelButton(wxCommandEvent& ev)
{
	// Use the ModelSelector to choose a model
	auto result = ModelSelector::chooseModel(
		_entities.getSharedKeyValue(_key->getFullKey(), true), false, false // pass the current model, don't show options or skins
	);

    if (result.objectKind != ModelSelector::Result::ObjectKind::Model)
    {
        return;
    }

    UndoableCommand cmd("setModelProperty");

    _entities.foreachEntity([&](const IEntityNodePtr& node)
    {
        auto& entity = node->getEntity();
        std::string prevModel = entity.getKeyValue(_key->getFullKey());
        std::string name = entity.getKeyValue("name");

        bool wasBrushBasedModel = prevModel == name;

        if (!result.name.empty())
        {
            bool willBeBrushBasedModel = result.name == name;

            // Check if any brushes should be removed, but inform the user about this
            if (!willBeBrushBasedModel && wasBrushBasedModel && hasChildPrimitives(node))
            {
                // Warn the user and proceed
                wxutil::Messagebox::Show(_("Warning"),
                    _("Changing this entity's model to the selected value will\nremove all child primitives from it:\n") + name,
                    IDialog::MessageType::MESSAGE_WARNING);

                scene::NodeRemover walker;
                node->traverseChildren(walker);
            }

            // Save the model key now
            entity.setKeyValue(_key->getFullKey(), result.name);

            signal_keyValueApplied().emit(_key->getFullKey(), result.name);
        }
    });
}

void ModelPropertyEditor::_onParticleButton(wxCommandEvent& ev)
{
	// Invoke ParticlesChooser
    std::string currentSelection = getKeyValueFromSelection(_key->getFullKey());
	std::string particle = ParticleChooserDialog::ChooseParticle(currentSelection);

	if (!particle.empty())
	{
        setKeyValueOnSelection(_key->getFullKey(), particle);
	}
}

void ModelPropertyEditor::_onSkinButton(wxCommandEvent& ev)
{
    // Check the key this model property editor is attached to first
    auto model = getKeyValueFromSelection(_key->getFullKey());

    // Fall back to "model" if nothing found
    if (model.empty())
    {
        model = getKeyValueFromSelection("model");
    }

    if (model.empty())
    {
        wxutil::Messagebox::ShowError(
            _("The model key values of the selection are ambiguous, cannot choose a skin."), getWidget());
        return;
    }

    // Target the "skin" property
    auto skinKey = _key->clone();
    skinKey->setAffectedKey("skin");

	std::string prevSkin = getKeyValueFromSelection(skinKey->getFullKey());
	std::string skin = SkinChooser::ChooseSkin(model, prevSkin);

	if (skin != prevSkin)
	{
		// Apply the key to the entity
        setKeyValueOnSelection(skinKey->getFullKey(), skin);
	}
}

}
