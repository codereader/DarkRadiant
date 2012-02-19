#include "ShaderChooser.h"

#include "i18n.h"
#include "iregistry.h"
#include "ishaders.h"
#include "texturelib.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "string/string.h"

#include <gtkmm/entry.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gdk/gdkkeysyms.h>

namespace ui
{
	namespace
	{
		const char* const LABEL_TITLE = N_("Choose Shader");
		const std::string SHADER_PREFIXES = "textures";
		const int DEFAULT_SIZE_X = 550;
		const int DEFAULT_SIZE_Y = 500;
		const std::string RKEY_WINDOW_STATE = "user/ui/textures/shaderChooser/window";
	}

// Construct the dialog
ShaderChooser::ShaderChooser(const Glib::RefPtr<Gtk::Window>& parent,
							 Gtk::Entry* targetEntry) :
	gtkutil::BlockingTransientWindow(_(LABEL_TITLE), parent),
	_targetEntry(targetEntry),
	_selector(Gtk::manage(new ShaderSelector(this, SHADER_PREFIXES)))
{
	set_border_width(12);

	if (_targetEntry != NULL)
	{
		_initialShader = targetEntry->get_text();

		// Set the cursor of the tree view to the currently selected shader
		_selector->setSelection(_initialShader);
	}

	// Set the default size and position of the window
	set_default_size(DEFAULT_SIZE_X, DEFAULT_SIZE_Y);

	// Connect the key handler to catch the ESC event
	signal_key_press_event().connect(sigc::mem_fun(*this, &ShaderChooser::onKeyPress), false);

	// Construct main VBox, and pack in the ShaderSelector and buttons panel
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 3));
	vbx->pack_start(*_selector, true, true, 0);
	vbx->pack_start(createButtons(), false, false, 0);

	add(*vbx);

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

void ShaderChooser::shutdown()
{
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
}

// Construct the buttons
Gtk::Widget& ShaderChooser::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 3));
	hbx->set_border_width(3);

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &ShaderChooser::callbackOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ShaderChooser::callbackCancel));

	hbx->pack_end(*okButton, false, false, 0);
	hbx->pack_end(*cancelButton, false, false, 0);

	return *hbx;
}

void ShaderChooser::shaderSelectionChanged(const std::string& shaderName,
										   const Glib::RefPtr<Gtk::ListStore>& listStore)
{
	if (_targetEntry != NULL)
	{
		_targetEntry->set_text(_selector->getSelection());
	}

	// Propagate the call up to the client (e.g. SurfaceInspector)
    _shaderChangedSignal.emit();

	// Get the shader, and its image map if possible
	MaterialPtr shader = _selector->getSelectedShader();
	// Pass the call to the static member
	ShaderSelector::displayShaderInfo(shader, listStore);
}

void ShaderChooser::revertShader()
{
	// Revert the shadername to the value it had at dialog startup
	if (_targetEntry != NULL)
	{
		_targetEntry->set_text(_initialShader);

		// Propagate the call up to the client (e.g. SurfaceInspector)
        _shaderChangedSignal.emit();
	}
}

void ShaderChooser::callbackCancel()
{
	// Revert the shadername to the value it had at dialog startup
	revertShader();

	destroy();
}

void ShaderChooser::callbackOK()
{
	if (_targetEntry != NULL)
	{
		_targetEntry->set_text(_selector->getSelection());
	}

	destroy();
}

bool ShaderChooser::onKeyPress(GdkEventKey* ev)
{
	// Check for ESC or ENTER to close the dialog
	switch (ev->keyval)
	{
		case GDK_Escape:
			callbackCancel();
			// Don't propagate the keypress if ESC could be processed
			return true;

		case GDK_Return:
			callbackOK();
			return true;
	};

	return false;
}

} // namespace ui
