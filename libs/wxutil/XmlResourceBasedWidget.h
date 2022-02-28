#pragma once

#include <wx/xrc/xmlres.h>
#include <wx/toolbar.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>

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

		return nullptr;
	}

    // Tries to find the toolbar item hosting the named control
    static const wxToolBarToolBase* getToolBarControlByName(wxToolBarBase* toolbar, const std::string& name)
    {
        wxString wxName(name);

        for (unsigned int i = 0; i < toolbar->GetToolsCount(); i++)
        {
            const wxToolBarToolBase* candidate = toolbar->GetToolByPos(i);

            if (candidate->IsControl() && candidate->GetControl()->GetName() == name)
            {
                return candidate;
            }
        }

        return nullptr;
    }

    /// Look up a toolbar tool by label and return its ID
    static int getToolID(wxToolBarBase* toolbar, const std::string& label)
    {
        auto tool = getToolBarToolByLabel(toolbar, label);
        if (tool)
            return tool->GetId();
        else
            return wxID_NONE;
    }

	// Find a named panel among the parent's children
	wxPanel* findNamedPanel(wxWindow* parent, const std::string& name)
	{
		return findNamedObject<wxPanel>(parent, name);
	}

	// Find a named object among the parent's children
    template <typename ObjectClass>
    ObjectClass* findNamedObject(const wxWindow* parent, const std::string& name) const
    {
        auto* named = dynamic_cast<ObjectClass*>(parent->FindWindow(wxString(name)));
        wxASSERT_MSG(named, "findNamedObject() failed (child not found)");

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

    void replaceControl(wxWindow* oldCtrl, wxWindow* newCtrl)
    {
        bool wasEnabled = oldCtrl->IsEnabled();
        auto name = oldCtrl->GetName();
        auto minSize = oldCtrl->GetMinSize();
        oldCtrl->GetContainingSizer()->Replace(oldCtrl, newCtrl);
        oldCtrl->Destroy();

        newCtrl->SetName(name);
        newCtrl->Enable(wasEnabled);
        newCtrl->SetMinSize(minSize);
        newCtrl->GetContainingSizer()->Layout();
    }

    // Swaps out the wxSpinCtrl with wxSpinCtrlDouble
    // This is a workaround for http://trac.wxwidgets.org/ticket/15425 - wxGTK < 3.1.1 don't ship an XRC handler for wxSpinCtrlDouble
    wxSpinCtrlDouble* convertToSpinCtrlDouble(wxWindow* parent, const std::string& nameOfControlToReplace, double min, double max, double increment, int digits)
    {
        auto oldCtrl = findNamedObject<wxSpinCtrl>(parent, nameOfControlToReplace);

        auto spinCtrlDouble = new wxSpinCtrlDouble(oldCtrl->GetParent(), wxID_ANY);

        spinCtrlDouble->SetRange(min, max);
        spinCtrlDouble->SetDigits(digits);
        spinCtrlDouble->SetIncrement(increment);

        replaceControl(oldCtrl, spinCtrlDouble);

        return spinCtrlDouble;
    }
};

} // namespace
