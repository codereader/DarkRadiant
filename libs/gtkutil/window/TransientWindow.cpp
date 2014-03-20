#include "TransientWindow.h"

#include "iuimanager.h"
#include <wx/artprov.h>

namespace wxutil
{

TransientWindow::TransientWindow(const std::string& title, wxWindow* parent, bool hideOnDelete) :
	wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, 
		wxSYSTEM_MENU | wxRESIZE_BORDER | wxMINIMIZE_BOX | 
		wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN | wxFRAME_FLOAT_ON_PARENT | 
		wxFRAME_NO_TASKBAR),
	_hideOnDelete(hideOnDelete)
{
	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(TransientWindow::_onDelete), NULL, this);
	Connect(wxEVT_SHOW, wxShowEventHandler(TransientWindow::_onShowHide), NULL, this);

	CenterOnParent();

	// Set the window icon
	wxIcon appIcon;
	appIcon.CopyFromBitmap(wxArtProvider::GetBitmap(
		GlobalUIManager().ArtIdPrefix() + "darkradiant_icon_64x64.png"));
	SetIcon(appIcon);
}

bool TransientWindow::Show(bool show)
{
	if (show)
	{
		_preShow();
	}
	else
	{
		_preHide();
	}

	// Pass the call to base
	return wxFrame::Show(show);
}

void TransientWindow::_onShowHide(wxShowEvent& ev)
{
	ev.Skip();

	if (ev.IsShown())
	{
		_postShow();
	}
	else
	{
		_postHide();
	}
}

bool TransientWindow::_onDeleteEvent()
{
	if (_hideOnDelete)
    {
        Hide();
		return true; // veto event
    }

	_preDestroy();
	
	Destroy();

	_postDestroy();

	return false;
}

void TransientWindow::_onDelete(wxCloseEvent& ev)
{
	if (_onDeleteEvent())
	{
		ev.Veto();
	}
}

} // namespace

#include <cassert>

using namespace sigc;

namespace gtkutil
{

TransientWindow::TransientWindow(const std::string& title,
                                 const Glib::RefPtr<Gtk::Window>& parent,
                                 bool hideOnDelete)
: _hideOnDelete(hideOnDelete),
  _isFullscreen(false)
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
    signal_delete_event().connect(
        bind_return(
            sigc::hide(mem_fun(*this, &TransientWindow::_onDeleteEvent)), true
        )
    );
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
    }
    else
    {
        unfullscreen();
    }

    _isFullscreen = isFullScreen;
}

void TransientWindow::toggleFullscreen()
{
    setFullscreen(!isFullscreen());
}

bool TransientWindow::isFullscreen()
{
    return _isFullscreen;
}

}

