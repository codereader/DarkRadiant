#include "ModelPropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "ui/modelselector/ModelSelector.h"
#include "ui/particles/ParticlesChooser.h"

#include "ientity.h"
#include "iradiant.h"

#include <gtk/gtk.h>

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
	_widget = gtk_vbox_new(FALSE, 6);
	// Horizontal box contains the browse button
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);
	
	// Browse button for models
	GtkWidget* browseButton = gtk_button_new_with_label("Choose model...");
	gtk_button_set_image(
		GTK_BUTTON(browseButton),
		gtk_image_new_from_pixbuf(
			PropertyEditorFactory::getPixbufFor("model")
		)
	);
	g_signal_connect(G_OBJECT(browseButton), 
					 "clicked", 
					 G_CALLBACK(_onModelButton), 
					 this);
	gtk_box_pack_start(GTK_BOX(hbx), browseButton, TRUE, FALSE, 0);
	
			
	// Browse button for particles
	GtkWidget* particleButton = gtk_button_new_with_label("Choose particle...");
	gtk_button_set_image(
		GTK_BUTTON(particleButton),
		gtk_image_new_from_pixbuf(
			GlobalRadiant().getLocalPixbuf("particle16.png")
		)
	);
	g_signal_connect(G_OBJECT(particleButton), 
					 "clicked", 
					 G_CALLBACK(_onParticleButton), 
					 this);
	gtk_box_pack_start(GTK_BOX(hbx), particleButton, TRUE, FALSE, 0);
	
	// Pack hbox into vbox (to limit vertical size), then edit frame
	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), vbx, TRUE, TRUE, 0);
}

/* GTK CALLBACKS */

void ModelPropertyEditor::_onModelButton(GtkWidget* w, 
										  ModelPropertyEditor* self)
{
	// Use the ModelSelector to choose a model
	ModelSelectorResult result = ModelSelector::chooseModel(
		self->_entity->getKeyValue(self->_key), false, false // pass the current model, don't show options or skins
	);

	if (!result.model.empty()) {
		self->setKeyValue(self->_key, result.model);
	}
}

void ModelPropertyEditor::_onParticleButton(GtkWidget* w,
											ModelPropertyEditor* self)
{
	// Invoke ParticlesChooser
    std::string currentSelection = self->_entity->getKeyValue(self->_key);
	std::string particle = ParticlesChooser::chooseParticle(currentSelection);
	
	if (!particle.empty()) {
		self->setKeyValue(self->_key, particle);
	}
}


}
