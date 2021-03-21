#pragma once

#include "Binding.h"
#include <wx/spinctrl.h>

namespace ui
{

template<typename SpinCtrlType, typename Source>
class SpinCtrlBinding :
    public TwoWayBinding<Source, decltype(std::declval<SpinCtrlType>().GetValue())>
{
public:
    using ValueType = decltype(std::declval<SpinCtrlType>().GetValue());
    using BaseBinding = TwoWayBinding<Source, ValueType>;

private:
    SpinCtrlType* _spinCtrl;

public:
    SpinCtrlBinding(SpinCtrlType* spinCtrl,
        const typename BaseBinding::LoadFunc& loadFunc,
        const typename BaseBinding::UpdateFunc& saveFunc) :
        SpinCtrlBinding(spinCtrl, loadFunc, saveFunc, BaseBinding::PostUpdateFunc())
    {}

    SpinCtrlBinding(SpinCtrlType* spinCtrl,
        const typename BaseBinding::LoadFunc& loadFunc,
        const typename BaseBinding::UpdateFunc& saveFunc,
        const typename BaseBinding::PostUpdateFunc& postChangeNotify) :
        SpinCtrlBinding(spinCtrl, loadFunc, saveFunc, postChangeNotify, std::bind(&BaseBinding::UseSourceAsTarget, this))
    {}

    SpinCtrlBinding(SpinCtrlType* spinCtrl,
                    const typename BaseBinding::LoadFunc& loadFunc,
                    const typename BaseBinding::UpdateFunc& saveFunc,
                    const typename BaseBinding::PostUpdateFunc& postChangeNotify,
                    const typename BaseBinding::AcquireTargetFunc& acquireSaveTarget) :
        BaseBinding(loadFunc, saveFunc, postChangeNotify, acquireSaveTarget),
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
        BaseBinding::updateValueOnTarget(_spinCtrl->GetValue());
    }
};

}
