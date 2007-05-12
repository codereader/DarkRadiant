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
#include "brushtokens.h"
#include "nameable.h"

class BrushNode :
	public scene::Node::Symbiot,
	public scene::Instantiable,
	public scene::Cloneable,
	public Nameable
{
	
	// The typecast class (needed to cast this node onto other types)
	class TypeCasts {
		NodeTypeCastTable m_casts;
	public:
		TypeCasts() {
			NodeStaticCast<BrushNode, scene::Instantiable>::install(m_casts);
			NodeStaticCast<BrushNode, scene::Cloneable>::install(m_casts);
			NodeStaticCast<BrushNode, Nameable>::install(m_casts);
			NodeContainedCast<BrushNode, Snappable>::install(m_casts);
			NodeContainedCast<BrushNode, TransformNode>::install(m_casts);
			NodeContainedCast<BrushNode, Brush>::install(m_casts);
			NodeContainedCast<BrushNode, MapImporter>::install(m_casts);
			NodeContainedCast<BrushNode, MapExporter>::install(m_casts);
			NodeContainedCast<BrushNode, BrushDoom3>::install(m_casts);
		}
		
		NodeTypeCastTable& get() {
			return m_casts;
		}
	};

	// The contained node
	scene::Node m_node;
	
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

	typedef LazyStatic<TypeCasts> StaticTypeCasts;
	
	// greebo: Returns the casted types of this node
	Snappable& get(NullType<Snappable>);
	TransformNode& get(NullType<TransformNode>);
	Brush& get(NullType<Brush>);
	MapImporter& get(NullType<MapImporter>);
	MapExporter& get(NullType<MapExporter>);
	BrushDoom3& get(NullType<BrushDoom3>);
	
	std::string name() const {
		return "Brush";
	}

	// Unused attach/detach functions (needed for nameable implementation)
	void attach(const NameCallback& callback) {}
	void detach(const NameCallback& callback) {}
	
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

// Casts the node onto a Brush and returns the pointer to it
inline Brush* Node_getBrush(scene::Node& node) {
	return NodeTypeCast<Brush>::cast(node);
}

#endif
