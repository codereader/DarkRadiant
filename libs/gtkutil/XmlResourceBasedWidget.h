#pragma once

#include <wx/xrc/xmlres.h>
#include <wx/toolbar.h>
#include <wx/window.h>

namespace wxutil
{

/**
 * Base class for wxWindows that load all (or parts of their) controls
 * from an XRC file in the application's ui/ directory.
 */
class XmlResourceBasedWidget
{
protected:
	// Loads a named Panel from the XRC resources
	wxPanel* loadNamedPanel(wxWindow* parent, const std::string& name)
	{
		wxPanel* panel = wxXmlResource::Get()->LoadPanel(parent, name);

		assert(panel != NULL);

		return panel;
	}

	static const wxToolBarToolBase* getToolBarToolByLabel(wxToolBarBase* toolbar, const std::string& name)
	{
		wxString wxName(name);

		for (int i = 0; i < toolbar->GetToolsCount(); i++)
		{
			const wxToolBarToolBase* candidate = toolbar->GetToolByPos(i);

			if (candidate->GetLabel() == wxName)
			{
				return candidate;
			}
		}

		return NULL;
	}

	// Find a named panel among the parent's children
	wxPanel* findNamedPanel(wxWindow* parent, const std::string& name)
	{
		wxString wxName(name);

		wxPanel* panel = static_cast<wxPanel*>(parent->FindWindow(name));

		assert(panel != NULL);

		return panel;
	}
};

} // namespace
