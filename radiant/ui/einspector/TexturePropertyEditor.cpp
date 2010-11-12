#include "TexturePropertyEditor.h"
#include "LightTextureChooser.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ientity.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/button.h>

namespace ui
{

// Main constructor
TexturePropertyEditor::TexturePropertyEditor(Entity* entity,
											 const std::string& name,
											 const std::string& options)
: PropertyEditor(entity),
  _prefixes(options),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 6);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	Gtk::VBox* outer = Gtk::manage(new Gtk::VBox(false, 0));
	Gtk::HBox* editBox = Gtk::manage(new Gtk::HBox(false, 3));

	// Create the browse button
	Gtk::Button* browseButton = Gtk::manage(new Gtk::Button(_("Choose texture...")));
	browseButton->set_image(*Gtk::manage(new Gtk::Image(
		PropertyEditorFactory::getPixbufFor("texture"))));

	browseButton->signal_clicked().connect(
		sigc::mem_fun(*this, &TexturePropertyEditor::_onBrowse));

	editBox->pack_start(*browseButton, true, false, 0);
	outer->pack_start(*editBox, true, false, 0);

	mainVBox->pack_start(*outer, true, true, 0);
}

// Browse button callback
void TexturePropertyEditor::_onBrowse()
{
	// Light texture chooser (self-destructs on close)
	LightTextureChooser chooser;
	std::string texture = chooser.chooseTexture();

	if (!texture.empty())
	{
		// Apply the keyvalue immediately
		setKeyValue(_key, texture);
	}
}

} // namespace ui
