#pragma once

#include "Binding.h"
#include "ishaderexpression.h"

namespace ui
{

template<typename Source>
class ExpressionBinding :
    public Binding<Source>
{
private:
    wxTextCtrl* _textCtrl;
    std::function<shaders::IShaderExpressionPtr(const Source&)> _getExpression;
    std::function<void()> _prepareSave;
    std::function<void(const Source&, const std::string&)> _updateExpression;

public:
    ExpressionBinding(wxTextCtrl* textCtrl,
        const std::function<shaders::IShaderExpressionPtr(const Source&)>& loadFunc,
        const std::function<void()>& prepareSaveFunc,
        const std::function<void(const Source&, const std::string&)>& saveFunc) :
        _textCtrl(textCtrl),
        _getExpression(loadFunc),
        _prepareSave(prepareSaveFunc),
        _updateExpression(saveFunc)
    {
        if (saveFunc)
        {
            _textCtrl->Bind(wxEVT_TEXT, &ExpressionBinding<Source>::onTextChanged, this);
        }
    }

    virtual void updateFromSource() override
    {
        if (!Binding<Source>::getSource())
        {
            _textCtrl->SetValue("");
            return;
        }

        auto expression = _getExpression(Binding<Source>::getSource());
        _textCtrl->SetValue(expression ? expression->getExpressionString() : "");
    }

private:
    void onTextChanged(wxCommandEvent& ev)
    {
        _prepareSave();
        _updateExpression(Binding<Source>::getSource(), _textCtrl->GetValue().ToStdString());
    }
};

}
