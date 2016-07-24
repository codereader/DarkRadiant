#pragma once

#include "imodel.h"
#include "iparticlenode.h"

#include "../infofile/InfoFile.h"

namespace map
{

class AssignLayerMappingWalker :
    public scene::NodeVisitor
{
private:
    InfoFile& _infoFile;

public:
    AssignLayerMappingWalker(InfoFile& infoFile) :
        _infoFile(infoFile)
    {}

    virtual ~AssignLayerMappingWalker() {}

    bool pre(const scene::INodePtr& node)
    {
        // To prevent all the support node types from getting layers assigned
        // filter them out, only Entities and Primitives get mapped in the info file
        if (Node_isEntity(node) || Node_isPrimitive(node))
        {
            // Retrieve the next set of layer mappings and assign them
            node->assignToLayers(_infoFile.getNextLayerMapping());
            return true;
        }

        // All other node types inherit the layers from their parent node
        // Model / particle / target line
        scene::INodePtr parent = node->getParent();

        if (parent)
        {
            node->assignToLayers(parent->getLayers());
        }

        return true;
    }
};

} // namespace
