#include "PathEntry.h"

#include "iregistry.h"

#include "i18n.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/window.h>

#include "FramedWidget.h"
#include "IConv.h"
#include "FileChooser.h"

#include "os/path.h"

namespace gtkutil
{

PathEntry::PathEntry(bool foldersOnly)
{
	set_shadow_type(Gtk::SHADOW_IN);

	// path entry
	_entry = Gtk::manage(new Gtk::Entry);
	_entry->set_has_frame(false);

	// generate browse button image
	std::string fullFileName = GlobalRegistry().get(RKEY_BITMAPS_PATH) + "ellipsis.png";
	Gtk::Widget* image = Gtk::manage(new Gtk::Image(Gdk::Pixbuf::create_from_file(fullFileName)));

	// browse button
	_button = Gtk::manage(new Gtk::Button);
	_button->add(*image);

	// Connect the button
	if (foldersOnly)
	{
		_button->signal_clicked().connect(sigc::mem_fun(*this, &PathEntry::onBrowseFolders));
	}
	else
	{
		_button->signal_clicked().connect(sigc::mem_fun(*this, &PathEntry::onBrowseFiles));
	}

	// Pack entry + button into an hbox
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 0));

	hbox->pack_start(*_entry, true, true, 0);
	hbox->pack_end(*_button, false, false, 0);

	// Add the contained widget as children to the frame
	add(*hbox);
}

void PathEntry::setValue(const std::string& val)
{
	// Convert the value to UTF8 before writing it to the entry box
	std::string utf8Val = gtkutil::IConv::filenameToUTF8(val);

	_entry->set_text(utf8Val);
}

std::string PathEntry::getValue() const
{
	return _entry->get_text();
}

Gtk::Entry& PathEntry::getEntryWidget()
{
	return *_entry;
}

void PathEntry::onBrowseFiles()
{
	Gtk::Container* toplevel = _button->get_toplevel();

	if (!toplevel->is_toplevel()) return; // Warning?

	// Get a new reference and create a shared pointer from the toplevel
	Glib::RefPtr<Gtk::Window> window(Glib::wrap(GTK_WINDOW(toplevel->gobj()), true));

	FileChooser fileChooser(window, _("Choose File"), true, false);

	fileChooser.setCurrentPath(getValue());

	std::string filename = fileChooser.display();

	window->present();

	if (!filename.empty())
	{
		setValue(gtkutil::IConv::filenameToUTF8(filename));
	}
}

void PathEntry::onBrowseFolders()
{
	Gtk::Container* toplevel = _button->get_toplevel();

	if (!toplevel->is_toplevel()) return; // Warning?

	// Get a new reference and create a shared pointer from the toplevel
	Glib::RefPtr<Gtk::Window> window(Glib::wrap(GTK_WINDOW(toplevel->gobj()), true));

	FileChooser fileChooser(window, _("Choose Directory"), true, true);

	std::string curEntry = getValue();

	if (!path_is_absolute(curEntry.c_str()))
	{
		curEntry.clear();
	}

	fileChooser.setCurrentPath(curEntry);

	std::string filename = fileChooser.display();

	window->present();

	if (!filename.empty())
	{
		setValue(gtkutil::IConv::filenameToUTF8(filename));
	}
}

} // namespace gtkutil
