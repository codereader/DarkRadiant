#include "ModelPropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "ui/modelselector/ModelSelector.h"
#include "ui/particles/ParticlesChooser.h"

#include "i18n.h"
#include "ientity.h"
#include "iuimanager.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

namespace ui
{

ModelPropertyEditor::ModelPropertyEditor()
{}

// Main constructor
ModelPropertyEditor::ModelPropertyEditor(Entity* entity,
									     const std::string& name,
									     const std::string& options)
: PropertyEditor(entity),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 6);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Horizontal box contains the browse button
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 3));
	hbx->set_border_width(3);

	// Browse button for models
	Gtk::Button* browseButton = Gtk::manage(new Gtk::Button(_("Choose model...")));
	browseButton->set_image(*Gtk::manage(new Gtk::Image(
		PropertyEditorFactory::getPixbufFor("model"))));

	browseButton->signal_clicked().connect(sigc::mem_fun(*this, &ModelPropertyEditor::_onModelButton));

	// Browse button for particles
	Gtk::Button* particleButton = Gtk::manage(new Gtk::Button(_("Choose particle...")));
	particleButton->set_image(*Gtk::manage(new Gtk::Image(
		GlobalUIManager().getLocalPixbuf("particle16.png"))));

	particleButton->signal_clicked().connect(sigc::mem_fun(*this, &ModelPropertyEditor::_onParticleButton));

	hbx->pack_start(*browseButton, true, false, 0);
	hbx->pack_start(*particleButton, true, false, 0);

	// Pack hbox into vbox (to limit vertical size), then edit frame
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));
	vbx->pack_start(*hbx, true, false, 0);

	mainVBox->pack_start(*vbx, true, true, 0);
}

void ModelPropertyEditor::_onModelButton()
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

void ModelPropertyEditor::_onParticleButton()
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
