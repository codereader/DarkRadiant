#pragma once

#include "ComponentEditor.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <stdexcept>

namespace objectives
{

namespace ce
{

/**
 * greebo: Common base class for all component editor implementations.
 * Most component editors pack their widgets into a VBox, which is what
 * this class derives from (privately). This base class implements the required
 * getWidget() method, return "this".
 */
class ComponentEditorBase :
	public ComponentEditor,
	protected wxPanel
{
protected:
	wxPanel* _panel;

	// Default constructor may only be used by subclasses
	ComponentEditorBase() :
		_panel(NULL)
	{}

public:
	ComponentEditorBase(wxWindow* parent) :
		_panel(new wxPanel(parent, wxID_ANY))
	{
		_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	}

	virtual wxWindow* getWidget()
	{
		if (_panel == NULL)
		{
			throw std::runtime_error("Cannot pack a ComponentEditor created by its default constructor!");
		}

		return this;
	}
};

}

}
