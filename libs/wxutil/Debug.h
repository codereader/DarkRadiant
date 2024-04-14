#pragma once

#include <iostream>
#include <wx/sizer.h>

inline std::ostream& operator<< (std::ostream& s, const wxSize& size)
{
    return s << "wxSize(w=" << size.GetWidth() << ", h=" << size.GetHeight() << ")";
}
