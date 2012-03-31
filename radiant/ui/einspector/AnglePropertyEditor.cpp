#include "AnglePropertyEditor.h"

#include "iuimanager.h"
#include "ientity.h"
#include "string/convert.h"

#include "gtkutil/IconTextButton.h"

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/table.h>

namespace ui
{

// Constructor
AnglePropertyEditor::AnglePropertyEditor(Entity* entity, const std::string& key)
: PropertyEditor(entity),
  _key(key)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 0);

    // Construct a 3x3 table to contain the directional buttons
	Gtk::Table* table = Gtk::manage(new Gtk::Table(3, 3, true));

    // Create the buttons
    constructButtons();

    // Add buttons
    table->attach(*_nButton, 1, 2, 0, 1);
    table->attach(*_sButton, 1, 2, 2, 3);
    table->attach(*_eButton, 2, 3, 1, 2);
    table->attach(*_wButton, 0, 1, 1, 2);
    table->attach(*_neButton, 2, 3, 0, 1);
    table->attach(*_seButton, 2, 3, 2, 3);
    table->attach(*_swButton, 0, 1, 2, 3);
    table->attach(*_nwButton, 0, 1, 0, 1);

    // Pack table into an hbox/vbox and set as the widget
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 0));
	hbx->pack_start(*table, true, false, 0);

    mainVBox->pack_start(*hbx, false, false, 0);

	// Register the main widget in the base class
	setMainWidget(mainVBox);
}

Gtk::Button* AnglePropertyEditor::constructAngleButton(
	const std::string& icon, int angleValue)
{
	Gtk::Button* w = Gtk::manage(new gtkutil::IconTextButton(
        "", GlobalUIManager().getLocalPixbuf(icon)
    ));

	w->signal_clicked().connect(
		sigc::bind(sigc::mem_fun(*this, &AnglePropertyEditor::_onButtonClick), angleValue)
	);

	return w;
}

// Construct the buttons
void AnglePropertyEditor::constructButtons()
{
    _nButton = constructAngleButton("arrow_n24.png", 90);
    _neButton = constructAngleButton("arrow_ne24.png", 45);
	_eButton = constructAngleButton("arrow_e24.png", 0);
	_seButton = constructAngleButton("arrow_se24.png", 315);
	_sButton = constructAngleButton("arrow_s24.png", 270);
	_swButton = constructAngleButton("arrow_sw24.png", 225);
	_wButton = constructAngleButton("arrow_w24.png", 180);
	_nwButton = constructAngleButton("arrow_nw24.png", 135);
}

void AnglePropertyEditor::_onButtonClick(int angleValue)
{
    setKeyValue(_key, string::to_string(angleValue));
}

} // namespace ui
