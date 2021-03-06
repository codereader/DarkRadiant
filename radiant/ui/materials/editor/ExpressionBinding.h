#pragma once

#include "Binding.h"
#include "ishaderexpression.h"
#include "util/ScopedBoolLock.h"

namespace ui
{

template<typename Source>
class ExpressionBinding :
    public TwoWayBinding<Source, std::string>
{
public:
    using BaseBinding = TwoWayBinding<Source, std::string>;

private:
    wxTextCtrl* _textCtrl;

public:
    ExpressionBinding(wxTextCtrl* textCtrl,
        const typename BaseBinding::LoadFunc& loadFunc,
        const typename BaseBinding::UpdateFunc& saveFunc) :
        ExpressionBinding(textCtrl, loadFunc, saveFunc, typename BaseBinding::PostUpdateFunc())
    {}

    ExpressionBinding(wxTextCtrl* textCtrl,
        const typename BaseBinding::LoadFunc& loadFunc,
        const typename BaseBinding::UpdateFunc& saveFunc,
        const typename BaseBinding::PostUpdateFunc& postChangeNotify) :
        ExpressionBinding(textCtrl, loadFunc, saveFunc, postChangeNotify, typename BaseBinding::AcquireTargetFunc())
    {}

    ExpressionBinding(wxTextCtrl* textCtrl,
                      const typename BaseBinding::LoadFunc& loadFunc,
                      const typename BaseBinding::UpdateFunc& saveFunc,
                      const typename BaseBinding::PostUpdateFunc& postChangeNotify,
                      const typename BaseBinding::AcquireTargetFunc& acquireSaveTarget) :
        BaseBinding(loadFunc, saveFunc, postChangeNotify, acquireSaveTarget),
        _textCtrl(textCtrl)
    {
        if (BaseBinding::_updateValue)
        {
            _textCtrl->Bind(wxEVT_TEXT, &ExpressionBinding::onTextChanged, this);
        }
    }

    virtual ~ExpressionBinding()
    {
        if (BaseBinding::_updateValue)
        {
            _textCtrl->Unbind(wxEVT_TEXT, &ExpressionBinding::onTextChanged, this);
        }
    }

protected:
    void setValueOnControl(const std::string& value) override
    {
        _textCtrl->SetValue(value);
    }

    void onTextChanged(wxCommandEvent& ev)
    {
        BaseBinding::updateValueOnTarget(_textCtrl->GetValue().ToStdString());
    }
};

}
