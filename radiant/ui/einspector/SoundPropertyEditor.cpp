#include "SoundPropertyEditor.h"
#include "PropertyEditorFactory.h"
#include "ui/common/SoundChooser.h"

#include "i18n.h"
#include "ientity.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

namespace ui
{

// Main constructor
SoundPropertyEditor::SoundPropertyEditor(Entity* entity,
									     const std::string& name,
									     const std::string& options)
: PropertyEditor(entity),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 0);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Horizontal box contains browse button
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 3));
	hbx->set_border_width(3);

	// Create the browse button
	Gtk::Button* browseButton = Gtk::manage(new Gtk::Button(_("Choose sound...")));
	browseButton->set_image(*Gtk::manage(new Gtk::Image(
		PropertyEditorFactory::getPixbufFor("sound"))));

	browseButton->signal_clicked().connect(
		sigc::mem_fun(*this, &SoundPropertyEditor::_onBrowseButton));

	hbx->pack_start(*browseButton, true, false, 0);

	// Pack hbox into vbox (to limit vertical size), then edit frame
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));
	vbx->pack_start(*hbx, true, false, 0);

	mainVBox->pack_start(*vbx, true, true, 0);
}

void SoundPropertyEditor::_onBrowseButton()
{
	// Use a SoundChooser dialog to get a selection from the user
	SoundChooser chooser;
	chooser.setSelectedShader(getKeyValue(_key));

	chooser.show(); // blocks

	const std::string& selection = chooser.getSelectedShader();

	// Selection will be empy if user clicked cancel or X
	if (!selection.empty())
	{
		// Apply the change to the entity
		setKeyValue(_key, selection);
	}
}

} // namespace ui
