#include "OverlayDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "iscenegraph.h"
#include "iregistry.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/SerialisableWidgets.h"
#include "gtkutil/MultiMonitor.h"

#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/filechooserbutton.h>
#include <gtkmm/scale.h>

#include "OverlayRegistryKeys.h"

namespace ui
{

/* CONSTANTS */
namespace
{
	const char* DIALOG_TITLE = N_("Background image");
}

// Create GTK stuff in c-tor
OverlayDialog::OverlayDialog() :
	PersistentTransientWindow(_(DIALOG_TITLE), GlobalMainFrame().getTopLevelWindow(), true),
	_callbackActive(false)
{
	set_border_width(12);

	// Set default size
	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());
	set_default_size(static_cast<int>(rect.get_width()/3), -1);

	// Pack in widgets
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 12));

	vbox->pack_start(createWidgets(), true, true, 0);
	vbox->pack_end(createButtons(), false, false, 0);
	
	add(*vbox);
	
	// Connect the widgets to the registry
	connectWidgets();	
}

// Construct main widgets
Gtk::Widget& OverlayDialog::createWidgets()
{
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 12));
	
	// "Use image" checkbox
	Gtk::CheckButton* useImage = Gtk::manage(new Gtk::CheckButton(_("Use background image"))); 
	_subWidgets["useImage"] = useImage;

	useImage->signal_toggled().connect(sigc::mem_fun(*this, &OverlayDialog::_onChange));
		
	vbox->pack_start(*useImage, false, false, 0);
	
	// Other widgets are in a table, which is indented with respect to the
	// Use Image checkbox, and becomes enabled/disabled with it.
	Gtk::Table* table = Gtk::manage(new Gtk::Table(8, 2, false));
	table->set_row_spacings(12);
	table->set_col_spacings(12);

	_subWidgets["subTable"] = table;
	
	// Image file
	Gtk::Label* imageLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Image file") + "</b>"));
	table->attach(*imageLabel, 0, 1, 0, 1, Gtk::FILL, Gtk::FILL);
	
	Gtk::FileChooserButton* fileButton = Gtk::manage(
		new Gtk::FileChooserButton(_("Choose image"), Gtk::FILE_CHOOSER_ACTION_OPEN));

	fileButton->signal_selection_changed().connect(sigc::mem_fun(*this, &OverlayDialog::_onFileSelection));
	_subWidgets["fileChooser"] = fileButton;
	
	table->attach(*fileButton, 1, 2, 0, 1);
	
	// Transparency slider
	Gtk::Label* transpLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Transparency") + "</b>"));
	table->attach(*transpLabel, 0, 1, 1, 2, Gtk::FILL, Gtk::FILL);
	
	Gtk::HScale* transSlider = Gtk::manage(new Gtk::HScale(0, 1, 0.01));
	transSlider->signal_value_changed().connect(sigc::mem_fun(*this, &OverlayDialog::_onScrollChange));
	_subWidgets["transparency"] = transSlider;
							  		
	table->attach(*transSlider, 1, 2, 1, 2);
	
	// Image size slider
	Gtk::Label* sizeLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Image scale") + "</b>"));
	table->attach(*sizeLabel, 0, 1, 2, 3, Gtk::FILL, Gtk::FILL);

	Gtk::HScale* scale = Gtk::manage(new Gtk::HScale(0, 20, 0.01));
	scale->signal_value_changed().connect(sigc::mem_fun(*this, &OverlayDialog::_onScrollChange));
	_subWidgets["scale"] = scale;
							  				
	table->attach(*scale, 1, 2, 2, 3);
	
	// Translation X slider
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(
					std::string("<b>") + _("Horizontal offset") + "</b>")),
				  0, 1, 3, 4, Gtk::FILL, Gtk::FILL);

	Gtk::HScale* transx = Gtk::manage(new Gtk::HScale(-20, 20, 0.01));
	transx->signal_value_changed().connect(sigc::mem_fun(*this, &OverlayDialog::_onScrollChange));
	_subWidgets["translateX"] = transx;
			
	table->attach(*transx, 1, 2, 3, 4);
	
	// Translation Y slider
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(
					std::string("<b>") + _("Vertical offset") + "</b>")),
				  0, 1, 4, 5, Gtk::FILL, Gtk::FILL);

	Gtk::HScale* transy = Gtk::manage(new Gtk::HScale(-20, 20, 0.01));
	transy->signal_value_changed().connect(sigc::mem_fun(*this, &OverlayDialog::_onScrollChange));
	_subWidgets["translateY"] = transy;
	
	table->attach(*transy, 1, 2, 4, 5);
	
	// Options list
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(
					std::string("<b>") + _("Options") + "</b>")),
				  0, 1, 5, 6, Gtk::FILL, Gtk::FILL);
	
	Gtk::CheckButton* keepAspect = Gtk::manage(new Gtk::CheckButton(_("Keep aspect ratio"))); 
	keepAspect->signal_toggled().connect(sigc::mem_fun(*this, &OverlayDialog::_onChange));
	_subWidgets["keepAspect"] = keepAspect;

	table->attach(*keepAspect, 1, 2, 5, 6);
	
	Gtk::CheckButton* scaleWithViewport = Gtk::manage(new Gtk::CheckButton(_("Zoom image with viewport"))); 
	scaleWithViewport->signal_toggled().connect(sigc::mem_fun(*this, &OverlayDialog::_onChange));
	_subWidgets["scaleImage"] = scaleWithViewport;	

	table->attach(*scaleWithViewport, 1, 2, 6, 7);
	
	Gtk::CheckButton* panWithViewport = Gtk::manage(new Gtk::CheckButton(_("Pan image with viewport"))); 
	panWithViewport->signal_toggled().connect(sigc::mem_fun(*this, &OverlayDialog::_onChange));
	_subWidgets["panImage"] = panWithViewport;	

	table->attach(*panWithViewport, 1, 2, 7, 8);
	
	// Pack table into vbox and return
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*table, 18, 1.0)), true, true, 0);
	
	return *vbox;
}

// Create button panel
Gtk::Widget& OverlayDialog::createButtons()
{
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));
	
	Gtk::Button* closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));
	closeButton->signal_clicked().connect(sigc::mem_fun(*this, &OverlayDialog::_onClose));
	
	hbox->pack_end(*closeButton, false, false, 0);

	return *hbox;
}

// Static show method
void OverlayDialog::display(const cmd::ArgumentList& args)
{
	// Maintain a static dialog instance and display it on demand
	OverlayDialogPtr& instance = InstancePtr();

	if (instance == NULL)
	{
		instance.reset(new OverlayDialog);
	}
	
	// Update the dialog state from the registry, and show it
	instance->getStateFromRegistry();
	instance->show_all();
}

OverlayDialogPtr& OverlayDialog::InstancePtr()
{
	static OverlayDialogPtr _instancePtr;
	return _instancePtr;
}

void OverlayDialog::destroy()
{
	InstancePtr().reset();
}

void OverlayDialog::connectWidgets()
{
   using namespace gtkutil;
	_connector.addObject(
        RKEY_OVERLAY_VISIBLE,
        StringSerialisablePtr(
			new SerialisableToggleButtonWrapper(static_cast<Gtk::ToggleButton*>(_subWidgets["useImage"]))
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_TRANSPARENCY,
        StringSerialisablePtr(
            new SerialisableScaleWidgetWrapper(static_cast<Gtk::HScale*>(_subWidgets["transparency"]))
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_SCALE,
        StringSerialisablePtr(
            new SerialisableScaleWidgetWrapper(static_cast<Gtk::HScale*>(_subWidgets["scale"]))
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_PROPORTIONAL,
        StringSerialisablePtr(
            new SerialisableToggleButtonWrapper(static_cast<Gtk::ToggleButton*>(_subWidgets["keepAspect"]))
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_SCALE_WITH_XY,
        StringSerialisablePtr(
            new SerialisableToggleButtonWrapper(static_cast<Gtk::ToggleButton*>(_subWidgets["scaleImage"]))
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_PAN_WITH_XY,
        StringSerialisablePtr(
            new SerialisableToggleButtonWrapper(static_cast<Gtk::ToggleButton*>(_subWidgets["panImage"]))
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_TRANSLATIONX,
        StringSerialisablePtr(
            new SerialisableScaleWidgetWrapper(static_cast<Gtk::HScale*>(_subWidgets["translateX"]))
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_TRANSLATIONY,
        StringSerialisablePtr(
            new SerialisableScaleWidgetWrapper(static_cast<Gtk::HScale*>(_subWidgets["translateY"]))
        )
    );
}

// Get the dialog state from the registry
void OverlayDialog::getStateFromRegistry()
{
	// Load the values into the widgets
	_callbackActive = true;

	_connector.importValues();
	updateSensitivity();
		
	// Image filename
	static_cast<Gtk::FileChooserButton*>(_subWidgets["fileChooser"])->set_filename(
		GlobalRegistry().get(RKEY_OVERLAY_IMAGE)
	);
	
	_callbackActive = false;
}

void OverlayDialog::updateSensitivity()
{
	// If the "Use image" toggle is disabled, desensitise all the other widgets
	_subWidgets["subTable"]->set_sensitive(GlobalRegistry().get(RKEY_OVERLAY_VISIBLE) == "1");
}

// Close button
void OverlayDialog::_onClose()
{
	hide();
}

// Generalised callback that triggers a value export
void OverlayDialog::_onChange()
{
	if (_callbackActive) return;

	// Export all the widget values to the registry
	_connector.exportValues();
	updateSensitivity();

	// Refresh
	GlobalSceneGraph().sceneChanged();
}

// File selection
void OverlayDialog::_onFileSelection()
{
	if (_callbackActive) return;
	
	// Set registry key
	GlobalRegistry().set(
		RKEY_OVERLAY_IMAGE, 
		static_cast<Gtk::FileChooserButton*>(_subWidgets["fileChooser"])->get_filename()
	);
}

// Scroll changes (triggers an update)
void OverlayDialog::_onScrollChange()
{
	if (_callbackActive) return;
	
	// Export all the widget values to the registry
	_connector.exportValues();

	// Refresh display
	GlobalSceneGraph().sceneChanged();
}

} // namespace ui
