#include "TransientWindow.h"

#include <cassert>

namespace gtkutil
{

TransientWindow::TransientWindow(const std::string& title, 
                                 const Glib::RefPtr<Gtk::Window>& parent, 
                                 bool hideOnDelete)
: _hideOnDelete(hideOnDelete)
{
    // Set up the window
    set_title(title);

    // Set transient
    setParentWindow(parent);

    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

#ifdef POSIX
    set_skip_taskbar_hint(true);
#endif
    set_skip_pager_hint(true);
    
    // Connect up the destroy signal (close box)
    signal_delete_event().connect(sigc::mem_fun(*this, &TransientWindow::_onDelete));
}

void TransientWindow::_onDeleteEvent()
{
    if (_hideOnDelete)
    {
        hide();
    }
    else
    {
        destroy();
    }
}

bool TransientWindow::_onDelete(GdkEventAny* ev) 
{
    // Invoke the virtual function
    _onDeleteEvent();
    return true;
}

void TransientWindow::setParentWindow(const Glib::RefPtr<Gtk::Window>& parent)
{
    if (parent)
    {
        Gtk::Container* toplevel = parent->get_toplevel();

        if (toplevel != NULL && toplevel->is_toplevel() &&
            dynamic_cast<Gtk::Window*>(toplevel) != NULL)
        {
            set_transient_for(*static_cast<Gtk::Window*>(toplevel));
        }
    }
}

Glib::RefPtr<Gtk::Window> TransientWindow::getRefPtr()
{
    return Glib::RefPtr<Gtk::Window>(Glib::wrap(gobj(), true)); // copy reference
}

void TransientWindow::show()
{
    if (!is_visible())
    {
        _preShow();
        show_all();
        _postShow();
    }
}

void TransientWindow::hide()
{
    _preHide();

    Window::hide();

    _postHide();
}

void TransientWindow::toggleVisibility()
{
    if (is_visible())
    {
        hide();
    }
    else
    {
        show();
    }
}

void TransientWindow::destroy()
{
    // Trigger a hide sequence if necessary
    if (is_visible())
    {
        TransientWindow::hide();
    }
    
    // Invoke destroy callbacks and destroy the Gtk widget
    _preDestroy();

    // No destroy anymore, this is handled by the destructors
    //Gtk::Widget::destroy();

    _postDestroy();
}

void TransientWindow::setFullscreen(bool isFullScreen)
{
    if (isFullScreen)
    {
        fullscreen();

        // Set the flag to 1
        set_data("dr-fullscreen", reinterpret_cast<void*>(1));
    }
    else
    {
        unfullscreen();

        // Set the flag to 0
        set_data("dr-fullscreen", NULL);
    }
}

void TransientWindow::toggleFullscreen()
{
    setFullscreen(!isFullscreen());
}

bool TransientWindow::isFullscreen()
{
    intptr_t val = reinterpret_cast<intptr_t>(get_data("dr-fullscreen"));
    
    return val != 0;
}

}

