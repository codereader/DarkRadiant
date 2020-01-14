#include "PropertyEditor.h"

#include "ientity.h"
#include "iundo.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>

namespace ui
{

PropertyEditor::PropertyEditor() :
	_mainWidget(nullptr),
	_entity(nullptr)
{}

PropertyEditor::PropertyEditor(Entity* entity) :
	_mainWidget(nullptr),
	_entity(entity)
{}

PropertyEditor::~PropertyEditor()
{
	// Destroy the widget
	if (_mainWidget != nullptr)
	{
		_mainWidget->Destroy();
        _mainWidget = nullptr;
	}
}

void PropertyEditor::setMainWidget(wxPanel* widget)
{
	_mainWidget = widget;

    // Get notified upon main widget destruction, we need
    // to forget about our reference to avoid double deletions
    _mainWidget->Bind(wxEVT_DESTROY, [&] (wxWindowDestroyEvent&)
    {
        _mainWidget = nullptr;
    });
}

wxPanel* PropertyEditor::getWidget()
{
	assert(_mainWidget); // should be set by the subclass at this point
	return _mainWidget;
}

void PropertyEditor::setEntity(Entity* entity)
{
	if (entity == nullptr) throw std::logic_error("No nullptrs allowed as entity argument");

	if (_entity != entity)
	{
		_entity = entity;

		// Let any subclasses update themselves now that the entity got changed
		updateFromEntity();
	}
}

std::string PropertyEditor::getKeyValue(const std::string& key)
{
	return _entity != nullptr ? _entity->getKeyValue(key) : std::string();
}

void PropertyEditor::setKeyValue(const std::string& key, const std::string& value)
{
	if (_entity == nullptr) return;

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
	button->Bind(wxEVT_BUTTON, &PropertyEditor::_onBrowseButtonClick, this);

	mainVBox->GetSizer()->Add(button, 0, wxALIGN_CENTER_VERTICAL);
}

void PropertyEditor::_onBrowseButtonClick(wxCommandEvent& ev)
{
	// Redirect the event to the method overridden by subclasses
	onBrowseButtonClick();
}

} // namespace ui
