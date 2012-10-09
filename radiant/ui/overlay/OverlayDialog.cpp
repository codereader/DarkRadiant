#include "OverlayDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "iscenegraph.h"
#include "registry/bind.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/SerialisableWidgets.h"
#include "gtkutil/MultiMonitor.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/checkbutton.h>
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
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Set default size
	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());
	set_default_size(static_cast<int>(rect.get_width()/3), -1);

	// Pack in widgets
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 12));

	vbox->pack_start(createWidgets(), true, true, 0);
	vbox->pack_end(createButtons(), false, false, 0);

	add(*vbox);
}

// Construct main widgets
Gtk::Widget& OverlayDialog::createWidgets()
{
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 12));

	// "Use image" checkbox
    _useImageBtn = Gtk::manage(new Gtk::CheckButton(_("Use background image")));
    _useImageBtn->set_active(registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));
	_useImageBtn->signal_toggled().connect(
        sigc::mem_fun(*this, &OverlayDialog::toggleUseImage)
    );
	vbox->pack_start(*_useImageBtn, false, false, 0);

	// Other widgets are in a table, which is indented with respect to the
	// Use Image checkbox, and becomes enabled/disabled with it.
	_subTable = Gtk::manage(new Gtk::Table(8, 2, false));
	_subTable->set_row_spacings(12);
	_subTable->set_col_spacings(12);

	// Image file
	Gtk::Label* imageLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Image file") + "</b>"));
	_subTable->attach(*imageLabel, 0, 1, 0, 1, Gtk::FILL, Gtk::FILL);

    // File chooser
	_fileChooserBtn = Gtk::manage(
        new Gtk::FileChooserButton(_("Choose image"), Gtk::FILE_CHOOSER_ACTION_OPEN)
    );
	_fileChooserBtn->signal_selection_changed().connect(
        sigc::mem_fun(*this, &OverlayDialog::_onFileSelection)
    );
	_subTable->attach(*_fileChooserBtn, 1, 2, 0, 1);

	// Transparency slider
	Gtk::Label* transpLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Transparency") + "</b>"));
	_subTable->attach(*transpLabel, 0, 1, 1, 2, Gtk::FILL, Gtk::FILL);

	Gtk::HScale* transSlider = Gtk::manage(new Gtk::HScale(0, 1, 0.01));
    registry::bindPropertyToKey(transSlider->get_adjustment()->property_value(), 
                                RKEY_OVERLAY_TRANSPARENCY);
	transSlider->signal_value_changed().connect(sigc::mem_fun(*this, &OverlayDialog::_onScrollChange));

	_subTable->attach(*transSlider, 1, 2, 1, 2);

	// Image size slider
	Gtk::Label* sizeLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Image scale") + "</b>"));
	_subTable->attach(*sizeLabel, 0, 1, 2, 3, Gtk::FILL, Gtk::FILL);

	Gtk::HScale* scale = Gtk::manage(new Gtk::HScale(0, 20, 0.01));
    registry::bindPropertyToKey(scale->get_adjustment()->property_value(),
                                RKEY_OVERLAY_SCALE);
	scale->signal_value_changed().connect(sigc::mem_fun(*this, &OverlayDialog::_onScrollChange));

	_subTable->attach(*scale, 1, 2, 2, 3);

	// Translation X slider
	_subTable->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(
					std::string("<b>") + _("Horizontal offset") + "</b>")),
				  0, 1, 3, 4, Gtk::FILL, Gtk::FILL);

	Gtk::HScale* transx = Gtk::manage(new Gtk::HScale(-20, 20, 0.01));
    registry::bindPropertyToKey(transx->get_adjustment()->property_value(),
                                RKEY_OVERLAY_TRANSLATIONX);
	transx->signal_value_changed().connect(sigc::mem_fun(*this, &OverlayDialog::_onScrollChange));

	_subTable->attach(*transx, 1, 2, 3, 4);

	// Translation Y slider
	_subTable->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(
					std::string("<b>") + _("Vertical offset") + "</b>")),
				  0, 1, 4, 5, Gtk::FILL, Gtk::FILL);

	Gtk::HScale* transy = Gtk::manage(new Gtk::HScale(-20, 20, 0.01));
    registry::bindPropertyToKey(transy->get_adjustment()->property_value(),
                                RKEY_OVERLAY_TRANSLATIONY);
	transy->signal_value_changed().connect(sigc::mem_fun(*this, &OverlayDialog::_onScrollChange));

	_subTable->attach(*transy, 1, 2, 4, 5);

	// Options list
	_subTable->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(
					std::string("<b>") + _("Options") + "</b>")),
				  0, 1, 5, 6, Gtk::FILL, Gtk::FILL);

	Gtk::CheckButton* keepAspect = Gtk::manage(new Gtk::CheckButton(_("Keep aspect ratio")));
    registry::bindPropertyToKey(keepAspect->property_active(), 
                                RKEY_OVERLAY_PROPORTIONAL);
	keepAspect->signal_toggled().connect(
        sigc::mem_fun(GlobalSceneGraph(), &scene::Graph::sceneChanged)
    );

	_subTable->attach(*keepAspect, 1, 2, 5, 6);

	Gtk::CheckButton* scaleWithViewport = Gtk::manage(new Gtk::CheckButton(_("Zoom image with viewport")));
    registry::bindPropertyToKey(scaleWithViewport->property_active(),
                                RKEY_OVERLAY_SCALE_WITH_XY);
	scaleWithViewport->signal_toggled().connect(
        sigc::mem_fun(GlobalSceneGraph(), &scene::Graph::sceneChanged)
    );

	_subTable->attach(*scaleWithViewport, 1, 2, 6, 7);

	Gtk::CheckButton* panWithViewport = Gtk::manage(new Gtk::CheckButton(_("Pan image with viewport")));
    registry::bindPropertyToKey(panWithViewport->property_active(), 
                                RKEY_OVERLAY_PAN_WITH_XY);
	panWithViewport->signal_toggled().connect(
        sigc::mem_fun(GlobalSceneGraph(), &scene::Graph::sceneChanged)
    );

	_subTable->attach(*panWithViewport, 1, 2, 7, 8);

	// Pack table into vbox and return
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_subTable, 18, 1.0)), true, true, 0);

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
	instance->initialiseWidgets();
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

// Get the dialog state from the registry
void OverlayDialog::initialiseWidgets()
{
	// Image filename
    _fileChooserBtn->set_filename(GlobalRegistry().get(RKEY_OVERLAY_IMAGE));
    updateSensitivity();
}

void OverlayDialog::updateSensitivity()
{
	// If the "Use image" toggle is disabled, desensitise all the other widgets
	_subTable->set_sensitive(_useImageBtn->get_active());

    g_assert(_subTable->get_sensitive() == registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));
}

// Close button
void OverlayDialog::_onClose()
{
	hide();
}

void OverlayDialog::toggleUseImage()
{
    registry::setValue(RKEY_OVERLAY_VISIBLE, _useImageBtn->get_active());
	updateSensitivity();

	// Refresh
	GlobalSceneGraph().sceneChanged();
}

// File selection
void OverlayDialog::_onFileSelection()
{
	// Set registry key
	GlobalRegistry().set(RKEY_OVERLAY_IMAGE, _fileChooserBtn->get_filename());
}

// Scroll changes (triggers an update)
void OverlayDialog::_onScrollChange()
{
	// Refresh display
	GlobalSceneGraph().sceneChanged();
}

} // namespace ui
