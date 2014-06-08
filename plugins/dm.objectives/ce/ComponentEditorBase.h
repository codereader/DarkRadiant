#pragma once

#include "ComponentEditor.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <stdexcept>
#include <boost/bind.hpp>

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

	// Constructors may only be used by subclasses
	ComponentEditorBase() :
		_panel(NULL)
	{}

	ComponentEditorBase(wxWindow* parent) :
		_panel(new wxPanel(parent, wxID_ANY))
	{
		_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	}

public:
	virtual ~ComponentEditorBase()
	{
		// When destroyed, remove the panel from its parent
		if (_panel != NULL)
		{
			_panel->GetParent()->RemoveChild(_panel);
			_panel->Destroy();
			_panel = NULL;
		}
	}

	virtual wxWindow* getWidget()
	{
		if (_panel == NULL)
		{
			throw std::runtime_error("Cannot pack a ComponentEditor created by its default constructor!");
		}

		return _panel;
	}

protected:
	// Shortcut used by subclasses to acquire a bind to to the onChange() method
	std::function<void()> getChangeCallback()
	{
		return boost::bind(&ComponentEditorBase::onChange, this);
	}

	// When anything changes, just trigger the writeToComponent callback,
	// the Component will be updated and in turn fires its changed signal.
	void ComponentEditorBase::onChange()
	{
		this->writeToComponent();
	}
};

}

}
