#pragma once

#include <memory>
#include "inode.h"
#include "imapformat.h"

namespace map
{

/**
 * Exporter class used to serialise a map to an output stream.
 * Use the given exportMap() method to write a scene to the
 * attached stream. For scene traversal the given functor
 * is used to allow for custom filtering of the scene nodes.
 *
 * Use GlobalMapModule().createMapExporter() to acquire an 
 * instance of IMapExporter.
 *
 * Note: This is a scoped object by design, which will prepare 
 * the scene during construction and clean it up on destruction.
 */
class IMapExporter
{
public:
    using Ptr = std::shared_ptr<IMapExporter>;

    virtual ~IMapExporter() {}

    // Export the scene below the given root node using the given traversal function
    virtual void exportMap(const scene::INodePtr& root, const GraphTraversalFunc& traverse) = 0;
};

}
