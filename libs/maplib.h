#pragma once

#include "imap.h"
#include "scene/EntityNode.h"

namespace map
{

namespace current
{

/// Convenience method to return the worldspawn entity pointer
inline Entity* getWorldspawn(bool createIfNotFound = false)
{
    scene::INodePtr wsNode {
        createIfNotFound ? GlobalMapModule().findOrInsertWorldspawn()
                         : GlobalMapModule().getWorldspawn()
    };
    return Node_getEntity(wsNode);
}

}

}
