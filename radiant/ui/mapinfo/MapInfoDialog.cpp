#include "MapInfoDialog.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "iuimanager.h"

#include <gtkmm/box.h>
#include <gtkmm/notebook.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

namespace ui {

	namespace {
		const int MAPINFO_DEFAULT_SIZE_X = 600;
	    const int MAPINFO_DEFAULT_SIZE_Y = 550;
		const char* const MAPINFO_WINDOW_TITLE = N_("Map Info");
	}

MapInfoDialog::MapInfoDialog() :
	BlockingTransientWindow(_(MAPINFO_WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow())
{
	set_default_size(MAPINFO_DEFAULT_SIZE_X, MAPINFO_DEFAULT_SIZE_Y);
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create all the widgets
	populateWindow();

	// Propagate shortcuts to the main window
	GlobalEventManager().connectDialogWindow(this);
}

void MapInfoDialog::shutdown()
{
	// Stop propagating shortcuts to the main window
	GlobalEventManager().disconnectDialogWindow(this);
}

void MapInfoDialog::populateWindow()
{
	// Create the vbox containing the notebook and the buttons
	Gtk::VBox* dialogVBox = Gtk::manage(new Gtk::VBox(false, 6));

	// Create the tabs
	_notebook = Gtk::manage(new Gtk::Notebook);

	// Entity Info
	_notebook->append_page(
		_entityInfo.getWidget(),
		createTabLabel(_entityInfo.getLabel(), _entityInfo.getIconName())
	);

	// Shader Info
	_notebook->append_page(
		_shaderInfo.getWidget(),
		createTabLabel(_shaderInfo.getLabel(), _shaderInfo.getIconName())
	);

	// Model Info
	_notebook->append_page(
		_modelInfo.getWidget(),
		createTabLabel(_modelInfo.getLabel(), _modelInfo.getIconName())
	);

	// Add notebook plus buttons to vbox
	dialogVBox->pack_start(*_notebook, true, true, 0);
	dialogVBox->pack_start(createButtons(), false, false, 0);

	// Add vbox to dialog window
	add(*dialogVBox);
}

Gtk::Widget& MapInfoDialog::createTabLabel(const std::string& label, const std::string& iconName)
{
	// The tab label items (icon + label)
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 3));

	hbox->pack_start(
		*Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbufWithMask(iconName))),
    	false, false, 3
    );
	hbox->pack_start(*Gtk::manage(new Gtk::Label(label)), false, false, 3);

	// Show the widgets before using them as label, they won't appear otherwise
	hbox->show_all();

	return *hbox;
}

Gtk::Widget& MapInfoDialog::createButtons()
{
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	Gtk::Button* closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));
	closeButton->signal_clicked().connect(sigc::mem_fun(*this, &MapInfoDialog::onClose));

	hbox->pack_end(*closeButton, false, false, 0);

	return *hbox;
}

void MapInfoDialog::onClose()
{
	// Call the destroy method which exits the main loop
	shutdown();
	destroy();
}

void MapInfoDialog::showDialog(const cmd::ArgumentList& args)
{
	MapInfoDialog dialog;
	dialog.show(); // blocks
}

} // namespace ui
