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
                      const typename BaseBinding::AcquireTargetFunc& acquireSaveTarget,
                      const typename BaseBinding::UpdateFunc& saveFunc,
                      const typename BaseBinding::PostUpdateFunc& postChangeNotify = BaseBinding::PostUpdateFunc()) :
        BaseBinding(loadFunc, saveFunc, postChangeNotify, acquireSaveTarget),
        _textCtrl(textCtrl)
    {
        if (saveFunc)
        {
            _textCtrl->Bind(wxEVT_TEXT, &ExpressionBinding::onTextChanged, this);
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
