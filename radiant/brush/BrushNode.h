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

#if !defined(INCLUDED_BRUSHNODE_H)
#define INCLUDED_BRUSHNODE_H

#include "instancelib.h"
#include "TexDef.h"
#include "ibrush.h"
#include "brushtokens.h"
#include "nameable.h"

class BrushNode :
	public scene::Node,
	public scene::Instantiable,
	public scene::Cloneable,
	public Nameable,
	public Snappable,
	public TransformNode,
	public BrushDoom3,
	public MapImporter,
	public MapExporter,
	public IBrushNode
{
	// The instances of this node
	InstanceSet m_instances;
	
	// The actual contained brush (NO reference)
	Brush m_brush;
	
	// The map importer/exporters
	BrushTokenImporter m_mapImporter;
	BrushTokenExporter m_mapExporter;

public:

	// Constructor
	BrushNode();
	
	// Copy Constructor
	BrushNode(const BrushNode& other);

	// IBrushNode implementtation
	virtual Brush& getBrush();
	
	std::string name() const {
		return "Brush";
	}
	
	// MapImporter implementation
	virtual bool importTokens(Tokeniser& tokeniser);
	// MapExporter implementation
	virtual void exportTokens(std::ostream& os) const;
	
	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

	// Snappable implementation
	virtual void snapto(float snap);

	// BrushDoom3 implementation
	virtual void translateDoom3Brush(const Vector3& translation);

	// Returns the actual scene node
	scene::Node& node();

	// Allocates a new node on the heap (via copy construction)
	scene::Node& clone() const;
	
	// Creates a new instance on the heap
	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	// Inserts / erases an instance	
	void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance);
	scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path);
	
	// Loops through all instances with the given visitor class
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	
}; // class BrushNode

// Casts the node onto a BrushNode and returns the Brush pointer
inline Brush* Node_getBrush(scene::Node& node) {
	IBrushNode* brushNode = dynamic_cast<IBrushNode*>(&node);
	if (brushNode != NULL) {
		return &brushNode->getBrush();
	}
	return NULL;
}

#endif
