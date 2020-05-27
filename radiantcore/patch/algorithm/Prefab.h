#pragma once

#include "iorthoview.h"
#include "icommandsystem.h"
#include "math/AABB.h"
#include "patch/Patch.h"
#include "patch/PatchConstants.h"

namespace patch
{

namespace algorithm
{

/**
 * Construct a patch prefab of the given type, size and AABB.
 * This method will de-select all items in the scene, and the newly created
 * item will be selected.
 */
void constructPrefab(const AABB& aabb, const std::string& shader, EPatchPrefab eType, 
					 EViewType viewType, std::size_t width = 3, std::size_t height = 3);

// General-purpose command target, taking one string argument to specify the prefab type
void createPrefab(const cmd::ArgumentList& args);

// Various command targets to create patch prefabs
void createCylinder(const cmd::ArgumentList& args);
void createDenseCylinder(const cmd::ArgumentList& args);
void createVeryDenseCylinder(const cmd::ArgumentList& args);
void createSquareCylinder(const cmd::ArgumentList& args);
void createSphere(const cmd::ArgumentList& args);
void createEndcap(const cmd::ArgumentList& args);
void createBevel(const cmd::ArgumentList& args);
void createCone(const cmd::ArgumentList& args);

// Creates a simple patch mesh, firing the dialog if necessary
void createSimplePatch(const cmd::ArgumentList& args);

// Constructs the given cap type for the patch, creating new patches, and inserting them to the given parent
// The parent node must not be NULL.
void createCaps(Patch& patch, const scene::INodePtr& parent, EPatchCap type, const std::string& shader);

} // namespace

} // namespace
