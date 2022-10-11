#pragma once

#include "CodeTokeniser.h"

namespace parser
{

class GuiTokeniser :
    public CodeTokeniser
{
private:
    constexpr static auto GUI_OPERATORS =
    {
        "*", "/", "%", "+", "-",
        ">", ">=", "<", "<=", "==", "!=",
        "&&", "||",
        "?", ":"
    };

public:
    GuiTokeniser(const ArchiveTextFilePtr& file) :
        CodeTokeniser(file, WHITESPACE, KEPT_DELIMS, GUI_OPERATORS)
    {}
};

}
