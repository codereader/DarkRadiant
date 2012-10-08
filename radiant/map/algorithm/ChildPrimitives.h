#pragma once

#include <boost/shared_ptr.hpp>

// Forward Decl.
namespace scene { class INode; typedef boost::shared_ptr<INode> INodePtr; }

namespace map
{

/**
 * Traverses the given subgraph and moves all primitives such that
 * their geometry is positioned relative to their func_* parent's origin.
 */
void addOriginToChildPrimitives(const scene::INodePtr& root);

/**
 * Performs the inverse operation to addOriginToChildPrimitives().
 */
void removeOriginFromChildPrimitives(const scene::INodePtr& root);

} // namespace
