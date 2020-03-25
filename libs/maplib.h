#pragma once

#include "imap.h"
#include "ientity.h"

namespace map
{

namespace current
{

/// Convenience method to return the worldspawn entity pointer
inline Entity* getWorldspawn()
{
    return Node_getEntity(GlobalMapModule().getWorldspawn());
}

}

}
