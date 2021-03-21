#pragma once

#include "Binding.h"
#include "ishaderlayer.h"
#include <wx/radiobut.h>

namespace ui
{

template<typename Source>
class RadioButtonBinding :
    public TwoWayBinding<Source, bool>
{
public:
    using BaseBinding = TwoWayBinding<Source, bool>;

private:
    wxRadioButton* _radioButton;

public:
    RadioButtonBinding(wxRadioButton* radioButton,
        const typename BaseBinding::LoadFunc& loadFunc,
        const typename BaseBinding::UpdateFunc& saveFunc,
        const typename BaseBinding::PostUpdateFunc& postChangeNotify = BaseBinding::PostUpdateFunc(),
        const typename BaseBinding::AcquireTargetFunc& acquireSaveTarget = BaseBinding::UseSourceAsTarget) :
        BaseBinding(loadFunc, saveFunc, postChangeNotify, acquireSaveTarget),
        _radioButton(radioButton)
    {
        if (BaseBinding::_updateValue)
        {
            _radioButton->Bind(wxEVT_RADIOBUTTON, &RadioButtonBinding::onValueChanged, this);
        }
    }

    virtual ~RadioButtonBinding()
    {
        if (BaseBinding::_updateValue)
        {
            _radioButton->Unbind(wxEVT_RADIOBUTTON, &RadioButtonBinding::onValueChanged, this);
        }
    }

protected:
    void setValueOnControl(const bool& value) override
    {
        _radioButton->SetValue(value);
    }

    void onValueChanged(wxCommandEvent& ev)
    {
        BaseBinding::updateValueOnTarget(_radioButton->GetValue());
    }
};

}
