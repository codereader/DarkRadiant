#pragma once

#include "iselection.h"
#include "render/View.h"
#include "View.h"

namespace test
{

namespace algorithm
{

// Performs an orthoview point selection test top-down (XY view) on the specified position
inline void performPointSelectionOnPosition(const Vector3& position, selection::SelectionSystem::EModifier modifier)
{
    // Construct an orthoview centered at the position
    render::View view(false);
    constructCenteredOrthoview(view, position);
    auto test = constructOrthoviewSelectionTest(view);

    GlobalSelectionSystem().selectPoint(test, modifier, false);
}

// Performs an orthoview point selection test top-down (XY view) on the node's world position
inline void performPointSelectionOnNodePosition(const scene::INodePtr& node, selection::SelectionSystem::EModifier modifier)
{
    auto nodePosition = node->worldAABB().getOrigin();
    performPointSelectionOnPosition(nodePosition, modifier);
}

} // namespace

} // namespace
