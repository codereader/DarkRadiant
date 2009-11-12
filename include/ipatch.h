/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_IPATCH_H)
#define INCLUDED_IPATCH_H

#include "imodule.h"

#include "inode.h"
#include "math/Vector2.h"
#include "math/Vector3.h"

/* greebo: A PatchControl consists of a vertex and a set of texture coordinates.
 * Multiple PatchControls form a PatchControlArray or (together with width and height) a PatchControlMatrix.
 */
struct PatchControl
{
	Vector3 vertex;		// The coordinates of the control point
	Vector2 texcoord;	// The texture coordinates of this point
};

/* greebo: the abstract base class for a patch-creating class.
 * At the moment, the CommonPatchCreator, Doom3PatchCreator and Doom3PatchDef2Creator derive from this base class.   
 */
class PatchCreator :
	public RegisterableModule
{
public:
	// Create a patch and return the sceneNode 
	virtual scene::INodePtr createPatch() = 0;
};

class Patch;
class IPatchNode
{
public:
    virtual ~IPatchNode() {}

	/** 
	 * greebo: Retrieves the actual patch from a PatchNode
	 */
	virtual Patch& getPatch() = 0;
};
typedef boost::shared_ptr<IPatchNode> IPatchNodePtr;

inline bool Node_isPatch(scene::INodePtr node)
{
	return boost::dynamic_pointer_cast<IPatchNode>(node) != NULL;
}

// Casts a node onto a patch
inline Patch* Node_getPatch(scene::INodePtr node)
{
	IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(node);

	if (patchNode != NULL)
	{
		return &patchNode->getPatch();;
	}

	return NULL;
}

const std::string MODULE_PATCH("PatchModule");
const std::string DEF2("Def2");
const std::string DEF3("Def3");

// Acquires the PatchCreator of the given type ("Def2", "Def3")
inline PatchCreator& GlobalPatchCreator(const std::string& defType)
{
	boost::shared_ptr<PatchCreator> _patchCreator(
		boost::static_pointer_cast<PatchCreator>(
			module::GlobalModuleRegistry().getModule(MODULE_PATCH + defType) // e.g. "PatchModuleDef2"
		)
	);

	return *_patchCreator;
}

#endif
