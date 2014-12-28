#pragma once

#include "imousetoolmanager.h"
#include "wxutil/dialog/DialogBase.h"

class wxStaticText;
class wxPanel;
class wxMouseEvent;

namespace ui
{

/** 
 * A small dialog allowing to select a new button/modifier combination for a mousetool.
 */
class BindToolDialog :
    public wxutil::DialogBase
{
private:
    unsigned int _selectedState;
    IMouseToolGroup& _group;
    MouseToolPtr _tool;

    wxStaticText* _clickArea;
    wxPanel* _clickPanel;

public:
    BindToolDialog(wxWindow* parent, IMouseToolGroup& group, const MouseToolPtr& tool);

    // Returns the button/modifier flags as selected by the user
    unsigned int getChosenMouseButtonState();

private:
    void populateWindow();
    void onClick(wxMouseEvent& ev);
};

}
