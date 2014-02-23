#include "ConsoleView.h"

#include <boost/algorithm/string/replace.hpp>

namespace wxutil
{

ConsoleView::ConsoleView(wxWindow* parent) :
	wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_RICH2),
	_errorAttr(*wxRED),
	_warningAttr(wxColour(128, 128, 0)),
	_standardAttr(*wxBLACK)
{}

void ConsoleView::appendText(const std::string& text, TextMode mode)
{
	// The text usually arrives in single characters at a time
	// Directly writing to the wxTextCtrl is awfully slow, so let's do some buffering

	// In case the textmode changes, we need to flush our buffer
	if (_bufferMode != mode)
	{
		flushIdleCallback();
	}

	// Write to the buffer first
	_bufferMode = mode;
	_buffer.append(text);

	// Once we hit a single newline, flush the buffer
	// this should give just enough performance benefit
	if (text == "\n")
	{
		flushIdleCallback();
	}
	else
	{
		requestIdleCallback();
	}
}

void ConsoleView::onIdle()
{
	if (_buffer.empty()) return;

	switch (_bufferMode)
	{
		case ModeStandard:
			SetDefaultStyle(_standardAttr);
			break;
		case ModeWarning:
			SetDefaultStyle(_warningAttr);
			break;
		case ModeError:
			SetDefaultStyle(_errorAttr);
			break;
		default:
			SetDefaultStyle(_standardAttr);
	};

	// Replace NULL characters, this is not caught by localeToUTF8
	boost::algorithm::replace_all(_buffer, "\0", "NULL");

	// Insert at the end of the text buffer
	AppendText(_buffer);
	ShowPosition(GetLastPosition());

	_buffer.clear();
}

} // namespace

#include "gtkutil/nonmodal.h"
#include "gtkutil/IConv.h"
#include "i18n.h"

#include <gtkmm/textmark.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <boost/algorithm/string/replace.hpp>

namespace gtkutil
{

ConsoleView::ConsoleView() :
	Gtk::ScrolledWindow(),
	_textView(Gtk::manage(new Gtk::TextView))
{
	// Remember the buffer of this textview
	_buffer = _textView->get_buffer();

	_textView->set_size_request(0, -1); // allow shrinking
	_textView->set_wrap_mode(Gtk::WRAP_WORD);
	_textView->set_editable(false);

	widget_connect_escape_clear_focus_widget(GTK_WIDGET(_textView->gobj())); // TODO

	_textView->signal_populate_popup().connect(sigc::mem_fun(*this, &ConsoleView::onPopulatePopup));

	set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	add(*_textView);

	unset_focus_chain();

	// Initialise tags
	Gdk::Color yellow;
	yellow.set_rgb(0xb0ff, 0xb0ff, 0x0000);

	Gdk::Color red;
	red.set_rgb(0xffff, 0x0000, 0x0000);

	Gdk::Color black;
	black.set_rgb(0x0000, 0x0000, 0x0000);

	_errorTag = _buffer->create_tag("red_foreground");
	_warningTag = _buffer->create_tag("yellow_foreground");
	_standardTag = _buffer->create_tag("black_foreground");

	_errorTag->set_property("foreground-gdk", red);
	_warningTag->set_property("foreground-gdk", yellow);
	_standardTag->set_property("foreground-gdk", black);
}

void ConsoleView::appendText(const std::string& text, ETextMode mode)
{
	Glib::Mutex::Lock viewMutex(_mutex);

	// Select a tag according to the log level
	Glib::RefPtr<Gtk::TextBuffer::Tag> tag;

	switch (mode) {
		case MSTANDARD:
			tag = _standardTag;
			break;
		case MWARNING:
			tag = _warningTag;
			break;
		case MERROR:
			tag = _errorTag;
			break;
		default:
			tag = _standardTag;
	};

	if (!_end)
	{
		_end = Gtk::TextMark::create("end", false);
		_buffer->add_mark(_end, _buffer->end());
	}

	// GTK expects UTF8 characters, so convert the incoming string
	std::string converted = gtkutil::IConv::localeToUTF8(text);

	// Replace NULL characters, this is not caught by localeToUTF8
	boost::algorithm::replace_all(converted, "\0", "NULL");

	// Insert at the end of the text buffer
	_buffer->insert_with_tag(_buffer->end(), converted, tag);

	_buffer->move_mark(_end, _buffer->end());
	_textView->scroll_mark_onscreen(_end);
}

void ConsoleView::clear()
{
	_buffer->set_text("");
}

void ConsoleView::onClearConsole()
{
	clear();
}

void ConsoleView::onPopulatePopup(Gtk::Menu* menu)
{
	menu->add(*Gtk::manage(new Gtk::SeparatorMenuItem));

	Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem(_("Clear")));
	item->signal_activate().connect(sigc::mem_fun(*this, &ConsoleView::onClearConsole));

	item->show();
	menu->add(*item);
}

} // namespace gtkutil
