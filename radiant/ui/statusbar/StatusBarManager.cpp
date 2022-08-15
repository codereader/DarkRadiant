#include "StatusBarManager.h"

#include "itextstream.h"
#include "ui/imainframe.h"

#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/frame.h>
#include <wx/statbmp.h>

#include "module/StaticModule.h"
#include "wxutil/Bitmap.h"

namespace ui
{

namespace statusbar
{

StatusBarManager::StatusBarManager() :
	_tempParent(new wxFrame(nullptr, wxID_ANY, "")),
    _statusBar(new wxPanel(_tempParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxNO_BORDER))
{
    _tempParent->SetName("StatusBarTemporaryParent");
	_statusBar->SetName("Statusbar");
#ifdef __WXMSW__
    _statusBar->SetBackgroundColour(wxColour("#ABABAB"));
#endif
	_tempParent->Hide();

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	_statusBar->SetSizer(sizer);
}

const std::string& StatusBarManager::getName() const
{
    static std::string _name(MODULE_STATUSBARMANAGER);
    return _name;
}

const StringSet& StatusBarManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_MAINFRAME
    };

    return _dependencies;
}

void StatusBarManager::initialiseModule(const IApplicationContext& ctx)
{
    GlobalMainFrame().signal_MainFrameShuttingDown().connect(
        sigc::mem_fun(this, &StatusBarManager::onMainFrameShuttingDown));

    // Do a full re-paint when the mainframe is resized
    GlobalMainFrame().signal_MainFrameReady().connect([this]()
    {
        GlobalMainFrame().getWxTopLevelWindow()->Bind(wxEVT_SIZE, [this](wxSizeEvent& ev)
        {
            ev.Skip();
            if (_statusBar)
            {
                _statusBar->Refresh(true);
            }
        });
    });
}

wxWindow* StatusBarManager::getStatusBar()
{
	return _statusBar;
}

void StatusBarManager::addElement(const std::string& name, wxWindow* widget, int pos)
{
	// Get a free position
	int freePos = getFreePosition(pos);

	auto element = std::make_shared<StatusBarElement>(widget);

	// Store this element
	_elements.emplace(name, element);
	_positions.emplace(freePos, element);

	rebuildStatusBar();
}

wxWindow* StatusBarManager::getElement(const std::string& name)
{
	// Look up the key
	auto found = _elements.find(name);

	// return NULL if not found
	return found != _elements.end() ? found->second->toplevel : nullptr;
}

void StatusBarManager::addTextElement(const std::string& name, const std::string& icon,
	int pos, const std::string& description)
{
	// Get a free position
	int freePos = getFreePosition(pos);

	auto textPanel = new wxPanel(_statusBar, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
	textPanel->SetSizer(new wxBoxSizer(wxHORIZONTAL));
	textPanel->SetName("Statusbarconainer " + name);

    if (pos == StandardPosition::Commands)
    {
        textPanel->SetMinSize(wxSize(250, -1));
    }

	if (!description.empty())
	{
		textPanel->SetToolTip(description);
	}

	if (!icon.empty())
	{
		auto img = new wxStaticBitmap(textPanel, wxID_ANY, wxutil::GetLocalBitmap(icon));
		textPanel->GetSizer()->Add(img, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	}

	auto label = new wxStaticText(textPanel, wxID_ANY, "");
	textPanel->GetSizer()->Add(label, 1, wxALIGN_CENTER_VERTICAL);

	if (!description.empty())
	{
		label->SetToolTip(description);
	}

	auto element = std::make_shared<StatusBarElement>(textPanel, label);

	// Store this element
	_elements.emplace(name, element);
    _positions.emplace(freePos, element);

	rebuildStatusBar();
}

void StatusBarManager::setText(const std::string& name, const std::string& text, bool immediateUpdate)
{
	// Look up the key
	ElementMap::const_iterator found = _elements.find(name);

	// return NULL if not found
	if (found != _elements.end() && found->second->label != NULL)
	{
        if (found->second->text != text)
        {
            // Set the text
            found->second->text = text;

            // Do the rest of the work in the idle callback
            requestIdleCallback();

            if (immediateUpdate)
            {
                flushIdleCallback();
            }
        }
	}
	else
	{
		rError() << "Could not find text status bar element with the name "
			<< name << std::endl;
	}
}

void StatusBarManager::onIdle()
{
    // Do the size calculations of the widgets
    std::for_each(_elements.begin(), _elements.end(), [&](ElementMap::value_type& pair)
    {
        // Set the text on the widget
        StatusBarElement& element = *pair.second;

        if (element.label != NULL)
        {
            element.label->SetLabelMarkup(element.text);

            if (!element.text.empty())
            {
                element.label->SetMinClientSize(element.label->GetVirtualSize());
            }
            else
            {
                element.label->SetMinClientSize(wxSize(20, -1)); // reset to 20 pixels if empty text is passed
            }
        }
    });

    // Post a size event
	_statusBar->PostSizeEvent();
}

int StatusBarManager::getFreePosition(int desiredPosition)
{
	// Do we have an easy job?
	if (_positions.empty())
	{
		// nothing to calculate
		return desiredPosition;
	}

	PositionMap::const_iterator i = _positions.find(desiredPosition);

	if (i == _positions.end()) {
		return desiredPosition;
	}

	// Let's see if there's space between the desired position and the next larger one
	i = _positions.upper_bound(desiredPosition);

	if (i == _positions.end()) {
		// There is no position larger than the desired one, return this one
		return desiredPosition + 1;
	}
	// Found an existing position which is larger than the desired one
	else if (i->first == desiredPosition + 1) {
		// No space between these two items, put to back
		return _positions.rbegin()->first + 1;
	}
	else {
		return desiredPosition + 1;
	}
}

void StatusBarManager::rebuildStatusBar()
{
	if (_elements.empty()) return; // done here if empty

	// Resize the table to fit the widgets
	_statusBar->GetSizer()->Clear(false); // detach all children

	std::size_t col = 0;

	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i)
	{
		int flags = wxEXPAND | wxLEFT | wxRIGHT;

		// The first and the last status bar widget get a smaller left/right border
        auto spacing = col == 0 || col == _positions.size() - 1 ? 6 : 24;

        // A few default elements don't need to use 1 as proportion
        auto proportion = i->first == StandardPosition::MapStatistics || i->first == StandardPosition::GridSize ||
            i->first == StandardPosition::MapEditStopwatch || i->first == StandardPosition::OrthoViewPosition ||
            i->first == StandardPosition::Commands ? 0 : 1;

		_statusBar->GetSizer()->Add(i->second->toplevel, proportion, flags, spacing);

		col++;
	}

	_statusBar->Show();
}

void StatusBarManager::onMainFrameShuttingDown()
{
    flushIdleCallback();

    _statusBar = nullptr;
    _tempParent->Destroy();
    _tempParent = nullptr;
}

module::StaticModuleRegistration<StatusBarManager> statusBarManagerModule;

} // namespace

} // namespace
