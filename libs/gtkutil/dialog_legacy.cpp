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

#include "dialog.h"
#include "dialog/MessageBox.h"
#include "EntryAbortedException.h"

namespace gtkutil
{

// Display a Gtk Error dialog

void errorDialog(const std::string& errorText, const Glib::RefPtr<Gtk::Window>& mainFrame)
{
	MessageBox msg("Error", errorText, MessageBox::MESSAGE_ERROR, mainFrame);
	msg.run();
}

// Display a fatal Gtk Error dialog

void fatalErrorDialog(const std::string& errorText, const Glib::RefPtr<Gtk::Window>& mainFrame)
{
	errorDialog(errorText, mainFrame);
	abort();
}

// Display a text entry dialog
const std::string textEntryDialog(const std::string& title,
								  const std::string& prompt,
								  const std::string& defaultText,
								  const Glib::RefPtr<Gtk::Window>& mainFrame)
{
	Dialog dialog(title, mainFrame);

	Dialog::Handle entryHandle = dialog.addEntryBox(prompt);

	Dialog::Result result = dialog.run();

	if (result == Dialog::RESULT_OK)
	{
		return dialog.getElementValue(entryHandle);
	}
    else
	{
        throw EntryAbortedException("textEntryDialog(): dialog cancelled");
	}
}

} // namespace gtkutil
