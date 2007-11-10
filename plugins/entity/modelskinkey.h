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

#if !defined(INCLUDED_MODELSKINKEY_H)
#define INCLUDED_MODELSKINKEY_H

#include "modelskin.h"
#include "traverselib.h"

class SkinChangedWalker :
	public scene::Graph::Walker
{
	std::string _newSkinName;
public:
	SkinChangedWalker(const std::string& newSkinName) :
		_newSkinName(newSkinName)
	{}

	virtual bool pre(const scene::Path& path, scene::Instance& instance) const {
		// Check if we have a skinnable model
		SkinnedModel* skinned = dynamic_cast<SkinnedModel*>(&instance);

		if (skinned != NULL) {
			skinned->skinChanged(_newSkinName);
		}

		return true; // traverse children
	}
};

#endif
