#pragma once

#include "MouseTool.h"

namespace ui
{

class BrushCreatorTool :
    public MouseTool
{
public:
    const std::string& getName()
    {
        static std::string name("BrushCreatorTool");
        return name;
    }


};

} // namespace
