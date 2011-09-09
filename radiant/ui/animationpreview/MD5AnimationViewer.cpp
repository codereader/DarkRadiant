#include "MD5AnimationViewer.h"

#include "i18n.h"
#include "imainframe.h"
#include "imodelcache.h"
#include "imd5anim.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/RightAlignment.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

namespace ui
{

MD5AnimationViewer::MD5AnimationViewer() :
	gtkutil::BlockingTransientWindow(_("MD5 Animation Viewer"), GlobalMainFrame().getTopLevelWindow()),
	_preview(new AnimationPreview)
{
	set_border_width(12);

	// Set the default size of the window
	const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	int height = static_cast<int>(rect.get_height() * 0.6f);

	set_default_size(
		static_cast<int>(rect.get_width() * 0.4f), height
	);

	// Set the default size of the window
	_preview->setSize(rect.get_width() * 0.2f, height);

	// Main dialog vbox
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 12));

	// Create a horizontal box to pack the treeview on the left and the preview on the right
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	//hbox->pack_start(createTreeView(), true, true, 0);

	Gtk::VBox* previewBox = Gtk::manage(new Gtk::VBox(false, 0));
	previewBox->pack_start(*_preview, true, true, 0);

	hbox->pack_start(*previewBox, true, true, 0);

	vbox->pack_start(*hbox, true, true, 0);
	vbox->pack_end(createButtons(), false, false, 0);

	// Add main vbox to dialog
	add(*vbox);

	// TEMPORARY
	scene::INodePtr model =  GlobalModelCache().getModelNode("models/md5/chars/guards/proguard/tdm_ai_proguard_use.md5mesh");

	_preview->setModelNode(model);

	md5::IMD5AnimPtr anim = GlobalAnimationCache().getAnim("models/md5/chars/guards/proguard/walk.md5anim");

	_preview->setAnim(anim);
}

void MD5AnimationViewer::Show(const cmd::ArgumentList& args)
{
	MD5AnimationViewer viewer;
	viewer.show();
}

void MD5AnimationViewer::_postShow()
{
	// Initialise the GL widget after the widgets have been shown
	_preview->initialisePreview();

	// Call the base class, will enter main loop
	BlockingTransientWindow::_postShow();
}

void MD5AnimationViewer::_onOK()
{
	destroy();
}

// Create the buttons panel
Gtk::Widget& MD5AnimationViewer::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));

	closeButton->signal_clicked().connect(sigc::mem_fun(*this, &MD5AnimationViewer::_onOK));

	hbx->pack_end(*closeButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

} // namespace
