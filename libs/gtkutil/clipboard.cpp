/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "clipboard.h"

#include "stream/BufferInputStream.h"

#include <memory.h>
#include <gtkmm/clipboard.h>

/// \file
/// \brief Platform-independent GTK clipboard support.
/// \todo Using GDK_SELECTION_CLIPBOARD fails on win32, so we use the win32 API directly for now.
#if defined(WIN32)

const char* c_clipboard_format = "RadiantClippings";

#include <windows.h>

namespace gtkutil
{

void copyToClipboard(const Glib::ustring& contents)
{
  bool bClipped = false;
  UINT nClipboard = ::RegisterClipboardFormat(c_clipboard_format);
  if (nClipboard > 0)
  {
    if (::OpenClipboard(0))
    {
      EmptyClipboard();
      std::size_t length = contents.size();
      HANDLE h = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, length + sizeof(std::size_t));
      if (h != 0)
      {
        char *buffer = reinterpret_cast<char*>(::GlobalLock(h));
        *reinterpret_cast<std::size_t*>(buffer) = length;
        buffer += sizeof(std::size_t);
        memcpy(buffer, contents.c_str(), length);
        ::GlobalUnlock(h);
        ::SetClipboardData(nClipboard, h);
        ::CloseClipboard();
        bClipped = true;
      }
    }
  }

  if (!bClipped)
  {
    rMessage() << "Unable to register Windows clipboard formats, copy/paste between editors will not be possible\n";
  }
}

Glib::ustring pasteFromClipboard()
{
  UINT nClipboard = ::RegisterClipboardFormat(c_clipboard_format);
  if (nClipboard > 0 && ::OpenClipboard(0))
  {
    if(IsClipboardFormatAvailable(nClipboard))
    {
      HANDLE h = ::GetClipboardData(nClipboard);
      if(h)
      {
        const char *buffer = reinterpret_cast<const char*>(::GlobalLock(h));
        std::size_t length = *reinterpret_cast<const std::size_t*>(buffer);
        buffer += sizeof(std::size_t);
        BufferInputStream istream(buffer, length);
        rcvr(istream);
        ::GlobalUnlock(h);
      }
    }
    ::CloseClipboard();
  }
}

} // namespace gtkutil

#else

#include <gtk/gtkclipboard.h>

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

#endif
