#include "LightTextureChooser.h"

#include "i18n.h"
#include "ishaders.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "igame.h"
#include "igroupdialog.h"
#include "texturelib.h"
#include "iregistry.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/MultiMonitor.h"
#include <string>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

namespace ui
{

namespace
{
	const char* const LIGHT_PREFIX_XPATH = "/light/texture//prefix";

	/** greebo: Loads the prefixes from the registry and creates a
	 * 			comma-separated list string
	 */
	inline std::string getPrefixList()
	{
		std::string prefixes;

		// Get the list of light texture prefixes from the registry
		xml::NodeList prefList = GlobalGameManager().currentGame()->getLocalXPath(LIGHT_PREFIX_XPATH);

		// Copy the Node contents into the prefix vector
		for (xml::NodeList::iterator i = prefList.begin();
			 i != prefList.end();
			 ++i)
		{
			prefixes += (prefixes.empty()) ? "" : ",";
			prefixes += i->getContent();
		}

		return prefixes;
	}
}

// Construct the dialog
LightTextureChooser::LightTextureChooser()
:	gtkutil::BlockingTransientWindow(_("Choose texture"), GlobalMainFrame().getTopLevelWindow()),
	_selector(Gtk::manage(new ShaderSelector(this, getPrefixList(), true))) // true >> render a light texture
{
	// Set the default size of the window
	Gdk::Rectangle rect;

	if (GlobalGroupDialog().getDialogWindow()->is_visible())
	{
		rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalGroupDialog().getDialogWindow());
	}
	else
	{
		rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());
	}

	set_default_size(static_cast<int>(rect.get_width()*0.6f), static_cast<int>(rect.get_height()*0.6f));

	// Construct main VBox, and pack in ShaderSelector and buttons panel
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 6));

	vbx->pack_start(*_selector, true, true, 0);
	vbx->pack_start(createButtons(), false, false, 0);

	add(*vbx);
}

// Construct the buttons
Gtk::Widget& LightTextureChooser::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));
	hbx->set_border_width(3);

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &LightTextureChooser::callbackOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &LightTextureChooser::callbackCancel));

	hbx->pack_end(*okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

// Block for a selection
std::string LightTextureChooser::chooseTexture()
{
	// Show all widgets and enter a recursive main loop
	show();

	// Return the last selection
	return _selectedTexture;
}

void LightTextureChooser::shaderSelectionChanged(
	const std::string& shaderName,
	const Glib::RefPtr<Gtk::ListStore>& listStore)
{
	// Get the shader, and its image map if possible
	MaterialPtr shader = _selector->getSelectedShader();
	// Pass the call to the static member light shader info
	ShaderSelector::displayLightShaderInfo(shader, listStore);
}

void LightTextureChooser::callbackCancel()
{
	_selectedTexture.clear();
	destroy();
}

void LightTextureChooser::callbackOK()
{
	_selectedTexture = _selector->getSelection();
	destroy();
}

} // namespace ui
