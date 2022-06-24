#pragma once

#include <string>
#include "idecltypes.h"

namespace decl
{

class DeclarationFile
{
public:
    // mod-relative path
    std::string fullPath;

    // The default type of this file, used for all
    // declarations without explicitly declared type
    decl::Type defaultDeclType;

    bool operator< (const DeclarationFile& other) const
    {
        if (defaultDeclType < other.defaultDeclType)
        {
            return true;
        }

        if (defaultDeclType == other.defaultDeclType)
        {
            return fullPath < other.fullPath;
        }

        return false;
    }
};

}
