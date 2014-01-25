#include "StatusBarManager.h"

#include "iradiant.h"
#include "imainframe.h"
#include "itextstream.h"
#include "gtkutil/FramedWidget.h"

#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

#include <wx/panel.h>
#include <wx/artprov.h>
#include <wx/frame.h>

namespace ui {

StatusBarManager::StatusBarManager() :
	_tempParent(new wxFrame(NULL, wxID_ANY, "")),
	_statusBar(new wxPanel(_tempParent, wxID_ANY))
{
	_tempParent->Hide();

	wxFlexGridSizer* sizer = new wxFlexGridSizer(1, 1, 0, 2);
	_statusBar->SetSizer(sizer);
}

StatusBarManager::~StatusBarManager()
{
	_tempParent->Destroy();
}

wxWindow* StatusBarManager::getStatusBar()
{
	return _statusBar;
}

void StatusBarManager::addElement(const std::string& name, wxWindow* widget, int pos)
{
	// Get a free position
	int freePos = getFreePosition(pos);

	StatusBarElementPtr element(new StatusBarElement(widget));

	// Store this element
	_elements.insert(ElementMap::value_type(name, element));
	_positions.insert(PositionMap::value_type(freePos, element));

	rebuildStatusBar();
}

wxWindow* StatusBarManager::getElement(const std::string& name)
{
	// Look up the key
	ElementMap::const_iterator found = _elements.find(name);

	// return NULL if not found
	return (found != _elements.end()) ? found->second->toplevel : NULL;
}

void StatusBarManager::addTextElement(const std::string& name, const std::string& icon, int pos)
{
	// Get a free position
	int freePos = getFreePosition(pos);

	wxPanel* textPanel = new wxPanel(_statusBar, wxID_ANY);
	textPanel->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	wxStaticText* label = new wxStaticText(textPanel, wxID_ANY, "");

	textPanel->GetSizer()->Add(label, 0, wxEXPAND);

	if (!icon.empty())
	{
		wxStaticBitmap* img = new wxStaticBitmap(textPanel, wxID_ANY, wxNullBitmap);
		img->SetBitmap(wxArtProvider::GetBitmap(wxART_ERROR));
		textPanel->GetSizer()->Add(img, 0, wxEXPAND);
	}

	StatusBarElementPtr element(new StatusBarElement(textPanel, label));

	// Store this element
	_elements.insert(ElementMap::value_type(name, element));
	_positions.insert(PositionMap::value_type(freePos, element));

	rebuildStatusBar();
}

void StatusBarManager::setText(const std::string& name, const std::string& text)
{
	// Look up the key
	ElementMap::const_iterator found = _elements.find(name);

	// return NULL if not found
	if (found != _elements.end() && found->second->label != NULL)
	{
		// Set the text
		found->second->text = text;

		// Request an idle callback
		requestIdleCallback();
	}
	else
	{
		rError() << "Could not find text status bar element with the name "
			<< name << std::endl;
	}
}

void StatusBarManager::onIdle()
{
	// Fill in all buffered texts
	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i)
	{
		// Shortcut
		const StatusBarElement& element = *(i->second);

		// Skip non-labels
		if (element.label == NULL) continue;

		element.label->SetLabelText(element.text);
	}
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
	/*wxPanel* tempPanel = new wxPanel(_tempParent, wxID_ANY);

	wxBoxSizer* tempSizer = new wxBoxSizer(wxHORIZONTAL);
	tempPanel->SetSizer(tempSizer);

	// Prevent child widgets from destruction before clearing the container
	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i)
	{
		// Grab a reference of the widgets (a new widget will be "floating")
		tempPanel->GetSizer()->Add(tempPanel);
	}*/

	if (_elements.empty()) return; // done here if empty

	// Resize the table to fit the widgets
	wxFlexGridSizer* sizer = static_cast<wxFlexGridSizer*>(_statusBar->GetSizer());
	sizer->SetCols(static_cast<int>(_elements.size()));

	sizer->Clear(false); // detach all children

	int col = 0;

	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i)
	{
		// Add the widget at the appropriate position
		//i->second->toplevel->Reparent(_statusBar);

		sizer->Add(i->second->toplevel, 0, wxEXPAND);

		col++;
	}

	_statusBar->Show();

	//_tempParent->RemoveChild(tempPanel);
}

} // namespace ui
