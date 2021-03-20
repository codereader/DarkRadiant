#pragma once

#include "Binding.h"
#include <wx/spinctrl.h>

namespace ui
{

template<typename SpinCtrlType>
class SpinCtrlBinding :
    public TwoWayStageBinding<decltype(std::declval<SpinCtrlType>().GetValue())>
{
private:
    SpinCtrlType* _spinCtrl;
    using ValueType = decltype(std::declval<SpinCtrlType>().GetValue());

public:
    SpinCtrlBinding(SpinCtrlType* spinCtrl,
        const std::function<ValueType(const IShaderLayer::Ptr&)>& loadFunc,
        const std::function<IEditableShaderLayer::Ptr()>& acquireSaveTarget,
        const std::function<void(const IEditableShaderLayer::Ptr&, ValueType)>& saveFunc,
        const std::function<void()>& postChangeNotify = std::function<void()>()) :
        TwoWayStageBinding<ValueType>(loadFunc, acquireSaveTarget, saveFunc, postChangeNotify),
        _spinCtrl(spinCtrl)
    {
        if (saveFunc)
        {
            if (std::is_integral<ValueType>::value)
            {
                _spinCtrl->Bind(wxEVT_SPINCTRL, &SpinCtrlBinding::onValueChanged, this);
            }
            else
            {
                _spinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &SpinCtrlBinding::onValueChanged, this);
            }
        }
    }

protected:
    void setValueOnControl(const ValueType& value) override
    {
        _spinCtrl->SetValue(value);
    }

    void onValueChanged(wxCommandEvent& ev)
    {
        TwoWayStageBinding<ValueType>::updateValueOnTarget(_spinCtrl->GetValue());
    }
};

}
