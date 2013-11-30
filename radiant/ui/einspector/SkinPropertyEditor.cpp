#include "SkinPropertyEditor.h"
#include "SkinChooser.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include "ientity.h"

namespace ui
{

// Main constructor
SkinPropertyEditor::SkinPropertyEditor(Entity* entity,
									   const std::string& name,
									   const std::string& options)
: PropertyEditor(entity),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 6);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Horizontal box contains browse button
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 3));
	hbx->set_border_width(3);

	// Create the browse button
	Gtk::Button* browseButton = Gtk::manage(new Gtk::Button(_("Choose skin...")));
	browseButton->set_image(*Gtk::manage(new Gtk::Image(
		PropertyEditorFactory::getPixbufFor("skin"))));

	browseButton->signal_clicked().connect(
		sigc::mem_fun(*this, &SkinPropertyEditor::_onBrowseButton));

	hbx->pack_start(*browseButton, true, false, 0);

	// Pack hbox into vbox (to limit vertical size), then edit frame
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));
	vbx->pack_start(*hbx, true, false, 0);

	mainVBox->pack_start(*vbx, true, true, 0);
}

void SkinPropertyEditor::_onBrowseButton()
{
	// Display the SkinChooser to get a skin from the user
	std::string modelName = _entity->getKeyValue("model");
	std::string prevSkin = _entity->getKeyValue(_key);
	std::string skin = SkinChooser::chooseSkin(modelName, prevSkin);

	// Apply the key to the entity
	setKeyValue(_key, skin);
}

std::string SkinPropertyEditor::runDialog(Entity* entity, const std::string& key)
{
	std::string modelName = entity->getKeyValue("model");
	std::string prevSkin = entity->getKeyValue(key);
	std::string skin = SkinChooser::chooseSkin(modelName, prevSkin);

	// return the new value
	return skin;
}

} // namespace
