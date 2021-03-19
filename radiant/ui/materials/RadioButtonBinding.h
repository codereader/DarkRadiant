#pragma once

#include "Binding.h"
#include "ishaderlayer.h"
#include <wx/radiobut.h>

namespace ui
{

class RadioButtonBinding :
    public TwoWayStageBinding<bool>
{
private:
    wxRadioButton* _radioButton;

public:
    RadioButtonBinding(wxRadioButton* radioButton,
        const std::function<bool(const IShaderLayer::Ptr&)>& loadFunc,
        const std::function<IEditableShaderLayer::Ptr()>& acquireSaveTarget,
        const std::function<void(const IEditableShaderLayer::Ptr&, bool)>& saveFunc,
        const std::function<void()>& postChangeNotify = std::function<void()>()) :
        TwoWayStageBinding(loadFunc, acquireSaveTarget, saveFunc, postChangeNotify),
        _radioButton(radioButton)
    {
        if (saveFunc)
        {
            _radioButton->Bind(wxEVT_RADIOBUTTON, &RadioButtonBinding::onValueChanged, this);
        }
    }

protected:
    void setValueOnControl(const bool& value) override
    {
        _radioButton->SetValue(value);
    }

    void onValueChanged(wxCommandEvent& ev)
    {
        updateValueOnTarget(_radioButton->GetValue());
    }
};

}
