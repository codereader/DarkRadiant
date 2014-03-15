#include "ModelPropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "ui/modelselector/ModelSelector.h"
#include "ui/particles/ParticlesChooser.h"

#include "i18n.h"
#include "ientity.h"
#include "iuimanager.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/artprov.h>
#include <wx/sizer.h>

namespace ui
{

ModelPropertyEditor::ModelPropertyEditor()
{}

// Main constructor
ModelPropertyEditor::ModelPropertyEditor(wxWindow* parent, Entity* entity,
									     const std::string& name,
									     const std::string& options)
: PropertyEditor(entity),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Browse button for models
	wxButton* browseButton = new wxButton(mainVBox, wxID_ANY, _("Choose model..."));
	browseButton->SetBitmap(PropertyEditorFactory::getBitmapFor("model"));
	browseButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ModelPropertyEditor::_onModelButton), NULL, this);

	// Browse button for particles
	wxButton* particleButton = new wxButton(mainVBox, wxID_ANY, _("Choose particle..."));
	particleButton->SetBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "particle16"));
	particleButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ModelPropertyEditor::_onParticleButton), NULL, this);

	mainVBox->GetSizer()->Add(browseButton);
	mainVBox->GetSizer()->Add(particleButton);
}

void ModelPropertyEditor::_onModelButton(wxCommandEvent& ev)
{
	// Use the ModelSelector to choose a model
	ModelSelectorResult result = ModelSelector::chooseModel(
		_entity->getKeyValue(_key), false, false // pass the current model, don't show options or skins
	);

	if (!result.model.empty())
	{
		setKeyValue(_key, result.model);
	}
}

void ModelPropertyEditor::_onParticleButton(wxCommandEvent& ev)
{
	// Invoke ParticlesChooser
    std::string currentSelection = _entity->getKeyValue(_key);
	std::string particle = ParticlesChooser::chooseParticle(currentSelection);

	if (!particle.empty())
	{
		setKeyValue(_key, particle);
	}
}


}
