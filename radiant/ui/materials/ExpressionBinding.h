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

public:
    ExpressionBinding(wxTextCtrl* textCtrl, const std::function<shaders::IShaderExpressionPtr(const Source&)> loadFunc) :
        _textCtrl(textCtrl),
        _getExpression(loadFunc)
    {}

    virtual void updateFromSource(const Source& source) override
    {
        if (!source)
        {
            _textCtrl->SetValue("");
            return;
        }

        auto expression = _getExpression(source);
        _textCtrl->SetValue(expression ? expression->getExpressionString() : "");
    }
};

}
