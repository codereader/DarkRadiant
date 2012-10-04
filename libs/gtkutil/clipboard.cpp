#include "clipboard.h"
#include <gtkmm/clipboard.h>

namespace gtkutil
{

void copyToClipboard(const Glib::ustring& contents)
{
    Glib::RefPtr<Gtk::Clipboard> cb = Gtk::Clipboard::get();
    cb->set_text(contents);
}

Glib::ustring pasteFromClipboard()
{
    Glib::RefPtr<Gtk::Clipboard> cb = Gtk::Clipboard::get();
    return cb->wait_for_text();
}

} // namespace gtkutil
