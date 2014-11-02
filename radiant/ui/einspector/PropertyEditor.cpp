#include "PropertyEditor.h"

#include "ientity.h"
#include "iundo.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>

namespace ui
{

PropertyEditor::PropertyEditor() :
	_mainWidget(NULL),
	_entity(NULL)
{}

PropertyEditor::PropertyEditor(Entity* entity) :
	_mainWidget(NULL),
	_entity(entity)
{}

PropertyEditor::~PropertyEditor()
{
	// Destroy the widget
	if (_mainWidget != NULL)
	{
		_mainWidget->Destroy();
        _mainWidget = NULL;
	}
}

void PropertyEditor::setMainWidget(wxPanel* widget)
{
	_mainWidget = widget;

    // Get notified upon main widget destruction, we need
    // to forget about our reference to avoid double deletions
    _mainWidget->Bind(wxEVT_DESTROY, [&] (wxWindowDestroyEvent&)
    {
        _mainWidget = NULL;
    });
}

wxPanel* PropertyEditor::getWidget()
{
	assert(_mainWidget); // should be set by the subclass at this point
	return _mainWidget;
}

std::string PropertyEditor::getKeyValue(const std::string& key)
{
	return (_entity != NULL) ? _entity->getKeyValue(key) : "";
}

void PropertyEditor::setKeyValue(const std::string& key, const std::string& value)
{
	if (_entity == NULL) return;

	UndoableCommand cmd("setProperty");

	_entity->setKeyValue(key, value);
}

void PropertyEditor::constructBrowseButtonPanel(wxWindow* parent, const std::string& label,
											 const wxBitmap& bitmap)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Button with image
	wxButton* button = new wxButton(mainVBox, wxID_ANY, label);
	button->SetBitmap(bitmap);
	button->Connect(wxEVT_BUTTON, wxCommandEventHandler(PropertyEditor::_onBrowseButtonClick), NULL, this);

	mainVBox->GetSizer()->Add(button, 0, wxALIGN_CENTER_VERTICAL);
}

void PropertyEditor::_onBrowseButtonClick(wxCommandEvent& ev)
{
	// Redirect the event to the method overridden by subclasses
	onBrowseButtonClick();
}

} // namespace ui
