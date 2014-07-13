#pragma once

#include <wx/xrc/xmlres.h>
#include <wx/toolbar.h>
#include <wx/panel.h>
#include <wx/stattext.h>

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

		for (unsigned int i = 0; i < toolbar->GetToolsCount(); i++)
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
		return findNamedObject<wxPanel>(parent, name);
	}

	// Find a named panel among the parent's children
	template<typename ObjectClass>
	ObjectClass* findNamedObject(wxWindow* parent, const std::string& name)
	{
		wxString wxName(name);

		ObjectClass* named = dynamic_cast<ObjectClass*>(parent->FindWindow(name));

		assert(named != NULL);

		return named;
	}

	// Find a named panel among the parent's children, doesn't throw when the item is not found
	template<typename ObjectClass>
	ObjectClass* tryGetNamedObject(wxWindow* parent, const std::string& name)
	{
		wxString wxName(name);

		ObjectClass* named = dynamic_cast<ObjectClass*>(parent->FindWindow(name));

		return named;
	}

	void makeLabelBold(wxWindow* parent, const std::string& widgetName)
	{
		wxStaticText* text = findNamedObject<wxStaticText>(parent, widgetName);
		text->SetFont(text->GetFont().Bold());
	}
};

} // namespace
