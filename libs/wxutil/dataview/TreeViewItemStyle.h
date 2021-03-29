#pragma once

#include <wx/dataview.h>

namespace wxutil
{

class TreeViewItemStyle
{
public:
    // Get the item format for favourites / non-favourites item in a declaration tree
    static wxDataViewItemAttr Declaration(bool isFavourite)
    {
        if (isFavourite)
        {
            wxDataViewItemAttr blueBold;
            blueBold.SetColour(wxColor(0, 0, 255));
            blueBold.SetBold(true);

            return blueBold;
        }
         
        return wxDataViewItemAttr();
    }

    static wxDataViewItemAttr Modified(bool isModified)
    {
        if (isModified)
        {
            wxDataViewItemAttr bold;
            bold.SetBold(true);

            return bold;
        }

        return wxDataViewItemAttr();
    }
};

}
