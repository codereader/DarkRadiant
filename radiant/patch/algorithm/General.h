#pragma once

#include "icommandsystem.h"
#include <boost/shared_ptr.hpp>

class PatchNode;
typedef boost::shared_ptr<PatchNode> PatchNodePtr;

namespace patch
{

namespace algorithm
{

/**
 * Thicken the given source patch.
 *
 * @param thickness: the amount of units to move the newly created patch away from the source.
 * @param createSeams: whether to create the "side wall" patches.
 * @param axis: extrusion axis, x = 0, y = 1, z = 2. 3 means: extrude along normals.
 *
 * The newly created patches will be selected in the scene.
 */
void thicken(const PatchNodePtr& sourcePatch, float thickness, bool createSeams, int axis);

void stitchTextures(const cmd::ArgumentList& args);

void bulge(const cmd::ArgumentList& args);

} // namespace

} // namespace
