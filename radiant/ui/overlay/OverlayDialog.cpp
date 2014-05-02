#include "OverlayDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "iscenegraph.h"
#include "registry/bind.h"

#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/button.h>

#include "OverlayRegistryKeys.h"

namespace ui
{

/* CONSTANTS */
namespace
{
	const char* DIALOG_TITLE = N_("Background image");

	const std::string RKEY_ROOT = "user/ui/overlayDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

OverlayDialog::OverlayDialog() :
	TransientWindow(_(DIALOG_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true),
	_callbackActive(false)
{
	loadNamedPanel(this, "OverlayDialogMainPanel");

	InitialiseWindowPosition(500, 350, RKEY_WINDOW_STATE);
	
	setupDialog();
}

void OverlayDialog::setupDialog()
{
	wxCheckBox* useImageBtn = findNamedObject<wxCheckBox>(this, "OverlayDialogUseBackgroundImage");
	useImageBtn->SetValue(registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));
	useImageBtn->Connect(wxEVT_CHECKBOX, 
		wxCommandEventHandler(OverlayDialog::_onToggleUseImage), NULL, this);

	wxButton* closeButton = findNamedObject<wxButton>(this, "OverlayDialogCloseButton");

	closeButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(OverlayDialog::_onClose), NULL, this);

	wxFilePickerCtrl* filepicker = findNamedObject<wxFilePickerCtrl>(this, "OverlayDialogFilePicker");
	filepicker->Connect(wxEVT_FILEPICKER_CHANGED, 
		wxFileDirPickerEventHandler(OverlayDialog::_onFileSelection), NULL, this);



#if 0
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
#endif
}

// Static toggle method
void OverlayDialog::toggle(const cmd::ArgumentList& args)
{
	Instance().ToggleVisibility();
}

void OverlayDialog::_preShow()
{
	initialiseWidgets();
}

void OverlayDialog::onRadiantShutdown()
{
    rMessage() << "OverlayDialog shutting down." << std::endl;

    // Destroy the window
	SendDestroyEvent();
    InstancePtr().reset();
}

OverlayDialog& OverlayDialog::Instance()
{
	OverlayDialogPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
    {
        // Not yet instantiated, do it now
        instancePtr.reset(new OverlayDialog);

        // Register this instance with GlobalRadiant() at once
        GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &OverlayDialog::onRadiantShutdown)
        );
    }

    return *instancePtr;
}

OverlayDialogPtr& OverlayDialog::InstancePtr()
{
	static OverlayDialogPtr _instancePtr;
	return _instancePtr;
}

// Get the dialog state from the registry
void OverlayDialog::initialiseWidgets()
{
	// Image filename
	wxFilePickerCtrl* filepicker = findNamedObject<wxFilePickerCtrl>(this, "OverlayDialogFilePicker");
	filepicker->SetFileName(wxFileName(GlobalRegistry().get(RKEY_OVERLAY_IMAGE)));

    updateSensitivity();
}

void OverlayDialog::updateSensitivity()
{
	// If the "Use image" toggle is disabled, desensitise all the other widgets
	wxCheckBox* useImageBtn = findNamedObject<wxCheckBox>(this, "OverlayDialogUseBackgroundImage");

	wxPanel* controls = findNamedObject<wxPanel>(this, "OverlayDialogControlPanel");
	controls->Enable(useImageBtn->GetValue());

	assert(controls->IsEnabled() == registry::getValue<bool>(RKEY_OVERLAY_VISIBLE));
}

void OverlayDialog::_onToggleUseImage(wxCommandEvent& ev)
{
	wxCheckBox* useImageBtn = static_cast<wxCheckBox*>(ev.GetEventObject());

    registry::setValue(RKEY_OVERLAY_VISIBLE, useImageBtn->GetValue());
	updateSensitivity();

	// Refresh
	GlobalSceneGraph().sceneChanged();
}

// File selection
void OverlayDialog::_onFileSelection(wxFileDirPickerEvent& ev)
{
	// Set registry key
	wxFilePickerCtrl* filepicker = findNamedObject<wxFilePickerCtrl>(this, "OverlayDialogFilePicker");

	GlobalRegistry().set(RKEY_OVERLAY_IMAGE, filepicker->GetFileName().GetFullPath().ToStdString());
}

void OverlayDialog::_onClose(wxCommandEvent& ev)
{
	Hide();
}

// Scroll changes (triggers an update)
void OverlayDialog::_onScrollChange()
{
	// Refresh display
	GlobalSceneGraph().sceneChanged();
}

} // namespace ui
