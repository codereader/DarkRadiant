#pragma once

#include "ComponentEditor.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <stdexcept>
#include <functional>

namespace objectives
{

namespace ce
{

/**
 * greebo: Common base class for all component editor implementations.
 * This base class implements the required getWidget() method, returning a panel.
 */
class ComponentEditorBase :
	public ComponentEditor
{
protected:
	wxPanel* _panel;
    bool _active; // whether writeToComponent is active

	// Constructors may only be used by subclasses
	ComponentEditorBase() :
		_panel(nullptr),
        _active(false)
	{}

	ComponentEditorBase(wxWindow* parent) :
		_panel(new wxPanel(parent, wxID_ANY)),
        _active(false)
	{
		_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	}

public:
	virtual ~ComponentEditorBase()
	{
		// When destroyed, remove the panel from its parent
        if (_panel != nullptr)
		{
			_panel->GetParent()->RemoveChild(_panel);
			_panel->Destroy();
            _panel = nullptr;
		}
	}

	virtual wxWindow* getWidget() override
	{
        if (_panel == nullptr)
		{
			throw std::runtime_error("Cannot pack a ComponentEditor created by its default constructor!");
		}

		return _panel;
	}

    virtual void setActive(bool active) override
    {
        _active = active;
    }

protected:
	// Shortcut used by subclasses to acquire a bind to to the onChange() method
	std::function<void()> getChangeCallback()
	{
		return std::bind(&ComponentEditorBase::onChange, this);
	}

	// When anything changes, just trigger the writeToComponent callback,
	// the Component will be updated and in turn fires its changed signal.
	void onChange()
	{
        if (_active)
        {
            this->writeToComponent();
        }
	}
};

}

}
