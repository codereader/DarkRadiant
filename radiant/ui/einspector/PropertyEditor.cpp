#include "PropertyEditor.h"

#include "ientity.h"
#include "iundo.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>

namespace ui
{

PropertyEditor::PropertyEditor(IEntitySelection& entities) :
	_mainWidget(nullptr),
	_entities(entities)
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

std::string PropertyEditor::getKeyValueFromSelection(const std::string& key)
{
    return _entities.getSharedKeyValue(key, true);
}

void PropertyEditor::setKeyValueOnSelection(const std::string& key, const std::string& value)
{
    if (_entities.empty()) return;

    UndoableCommand cmd("setProperty");

    _entities.foreachEntity([&](const IEntityNodePtr& entity)
    {
        entity->getEntity().setKeyValue(key, value);
    });

    signal_keyValueApplied().emit(key, value);
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

sigc::signal<void(const std::string&, const std::string&)>& PropertyEditor::signal_keyValueApplied()
{
    return _sigKeyValueApplied;
}

} // namespace ui
