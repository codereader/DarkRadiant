#include "ProcCompiler.h"

#include "itextstream.h"
#include "math/Plane3.h"
#include "ishaders.h"
#include "imodelcache.h"
#include "imodelsurface.h"
#include <limits>
#include <boost/format.hpp>

namespace map
{

std::size_t	BspTreeNode::nextNodeId = 0;
std::size_t ProcPortal::nextPortalId = 0;

const float CLIP_EPSILON = 0.1f;
const float SPLIT_WINDING_EPSILON = 0.001f;

const std::size_t MULTIAREA_CROSS = std::numeric_limits<std::size_t>::max();

ProcCompiler::ProcCompiler(const scene::INodePtr& root) :
	_root(root),
	_numActivePortals(0),
	_numPeakPortals(0),
	_numTinyPortals(0),
	_numUniqueBrushes(0),
	_numClusters(0),
	_numFloodedLeafs(0),
	_numOutsideLeafs(0),
	_numInsideLeafs(0),
	_numSolidLeafs(0),
	_numAreas(0),
	_numAreaFloods(0)
{}

ProcFilePtr ProcCompiler::generateProcFile()
{
	_procFile.reset(new ProcFile);

	// Load all entities into proc entities
	generateBrushData();

	processModels();

	return _procFile;
}

namespace
{

// Constructs all ProcPrimitives of a given ProcEntity
class ToolPrimitiveGenerator :
	public scene::NodeVisitor
{
private:
	ProcEntity& _entity;

	ProcBrush _buildBrush;

	const ProcFilePtr& _procFile;

	std::size_t _entityPrimitive;

public:
	ToolPrimitiveGenerator(ProcEntity& entity, const ProcFilePtr& procFile) :
		_entity(entity),
		_procFile(procFile),
		_entityPrimitive(0)
	{}

	bool pre(const scene::INodePtr& node)
	{
		// Is this a brush?
		IBrush* brush = Node_getIBrush(node);

		if (brush != NULL)
		{
			_entityPrimitive++;

			_buildBrush.sides.clear();
			
			for (std::size_t i = 0 ; i < brush->getNumFaces(); i++)
			{
				_buildBrush.sides.push_back(ProcFace());
				ProcFace& side = _buildBrush.sides.back();

				const IFace& mapFace = brush->getFace(i);

				side.planenum = findOrInsertPlane(mapFace.getPlane3());
				side.material = GlobalMaterialManager().getMaterialForName(mapFace.getShader());

				side.calculateTextureVectors(mapFace.getPlane3(), mapFace.getTexDefMatrix());
				// remove any integral shift, which will help with grouping
				side.texVec[0][3] -= floor(side.texVec[0][3]);
				side.texVec[1][3] -= floor(side.texVec[1][3]);
			}

			// if there are mirrored planes, the entire brush is invalid
			if (!removeDuplicateBrushPlanes())
			{
				return false;
			}

			// get the content for the entire brush
			setBrushContents();

			if (!finishBrush())
			{
				return false;
			}

			return false;
		}

		// or a patch?
		IPatch* patch = Node_getIPatch(node);

		if (patch != NULL)
		{
			_entityPrimitive++;

			// FIXME: Implement this option?
			// if ( dmapGlobals.noCurves ) {
			//	return;
			//}

			_procFile->numPatches++;

			MaterialPtr material = GlobalMaterialManager().getMaterialForName(patch->getShader());

			_entity.primitives.push_back(ProcPrimitive());
			ProcTris& tris = _entity.primitives.back().patch;

			// Get the tesselated mesh
			PatchMesh mesh = patch->getTesselatedPatchMesh();

			// Create triangles out of the quad-based tesselation
			for (std::size_t h = 0; h < mesh.height - 1; ++h)
			{
				for (std::size_t w = 1; w < mesh.width; ++w)
				{
					tris.resize(tris.size() + 2);

					std::size_t triIdx = tris.size() - 2;

					const PatchMesh::Vertex& v1 = mesh.vertices[h*mesh.width + w - 1];
					const PatchMesh::Vertex& v2 = mesh.vertices[h*mesh.width + w];
					const PatchMesh::Vertex& v3 = mesh.vertices[(h+1)*mesh.width + w - 1];
					const PatchMesh::Vertex& v4 = mesh.vertices[(h+1)*mesh.width + w];
					
					// greebo: Ordering such that it matches the one in D3
					tris[triIdx].v[0].vertex = v4.vertex;
					tris[triIdx].v[0].normal = v4.normal;
					tris[triIdx].v[0].texcoord = v4.texcoord;

					tris[triIdx].v[1].vertex = v2.vertex;
					tris[triIdx].v[1].normal = v2.normal;
					tris[triIdx].v[1].texcoord = v2.texcoord;

					tris[triIdx].v[2].vertex = v1.vertex;
					tris[triIdx].v[2].normal = v1.normal;
					tris[triIdx].v[2].texcoord = v1.texcoord;

					tris[triIdx].material = material;

					triIdx++;

					tris[triIdx].v[0].vertex = v3.vertex;
					tris[triIdx].v[0].normal = v3.normal;
					tris[triIdx].v[0].texcoord = v3.texcoord;

					tris[triIdx].v[1].vertex = v4.vertex;
					tris[triIdx].v[1].normal = v4.normal;
					tris[triIdx].v[1].texcoord = v4.texcoord;

					tris[triIdx].v[2].vertex = v1.vertex;
					tris[triIdx].v[2].normal = v1.normal;
					tris[triIdx].v[2].texcoord = v1.texcoord;

					tris[triIdx].material = material;
				}
			}

			// set merge groups if needed, to prevent multiple sides from being
			// merged into a single surface in the case of gui shaders, mirrors, and autosprites
			if (material->isDiscrete())
			{
				for (ProcTris::iterator tri = tris.begin(); tri != tris.end(); ++tri)
				{
					tri->mergePatch = patch;
				}
			}

			return false;
		}

		return true;
	}

private:
	bool createBrushWindings()
	{
		for (std::size_t i = 0; i < _buildBrush.sides.size(); ++i)
		{
			ProcFace& side = _buildBrush.sides[i];

			const Plane3& plane = _procFile->planes.getPlane(side.planenum);

			// We start out with a near-infinitely large winding
			side.winding.setFromPlane(plane);

			// Clip this large winding against all other windings
			for (std::size_t j = 0; j < _buildBrush.sides.size() && !side.winding.empty(); j++ )
			{
				if (i == j) continue;

				if (_buildBrush.sides[j].planenum == (_buildBrush.sides[i].planenum ^ 1))
				{
					continue;		// back side clipaway
				}

				side.winding.clip(_procFile->planes.getPlane(_buildBrush.sides[j].planenum ^ 1));
			}
		}

		return _buildBrush.bound();
	}

	bool finishBrush()
	{
		if (!createBrushWindings())
		{
			return false;
		}

		if (_buildBrush.contents & Material::SURF_AREAPORTAL)
		{
			if (_procFile->entities.size() != 1)
			{
				globalWarningStream() << 
					(boost::format("Entity %d, Brush %d: areaportals only allowed in world") % 
					 (_procFile->entities.size() - 1) % _entityPrimitive).str() << std::endl;
				return false;
			}
		}

		// keep it
		_entity.primitives.push_back(ProcPrimitive());

		ProcPrimitive& prim = _entity.primitives.back();

		// copy-construct the brush
		prim.brush.reset(new ProcBrush(_buildBrush));

		prim.brush->entitynum = _procFile->entities.size() - 1;
		prim.brush->brushnum = _entityPrimitive - 1;

		prim.brush->original = prim.brush; // reference to self

		_buildBrush.sides.clear();
		_buildBrush.entitynum = 0;

		return true;
	}

	void setBrushContents()
	{
		assert(!_buildBrush.sides.empty());

		const ProcFace& firstSide = _buildBrush.sides[0];
		int contents = firstSide.material->getSurfaceFlags();

		_buildBrush.contentShader = firstSide.material;
		
		bool mixed = false;

		// a brush is only opaque if all sides are opaque
		_buildBrush.opaque = true;

		for (std::size_t i = 1; i < _buildBrush.sides.size(); i++)
		{
			const ProcFace& side = _buildBrush.sides[i];

			if (!side.material)
			{
				continue;
			}

			int flags = side.material->getSurfaceFlags();

			if (flags != contents)
			{
				mixed = true;
				contents |= flags;
			}

			if (side.material->getCoverage() != Material::MC_OPAQUE)
			{
				_buildBrush.opaque = false;
			}
		}

		if (contents & Material::SURF_AREAPORTAL)
		{
			_procFile->numPortals++;
		}

		_buildBrush.contents = contents;
	}

	bool removeDuplicateBrushPlanes()
	{
		for (std::size_t i = 1 ; i < _buildBrush.sides.size(); ++i)
		{
			// check for a degenerate plane
			if (_buildBrush.sides[i].planenum == -1)
			{
				globalWarningStream() << 
					(boost::format("Entity %i, Brush %i: degenerate plane") % 
					 _buildBrush.entitynum % _buildBrush.brushnum).str() << std::endl;

				// remove it
				for (std::size_t k = i + 1 ; k < _buildBrush.sides.size(); ++k)
				{
					_buildBrush.sides[k-1] = _buildBrush.sides[k];
				}

				_buildBrush.sides.pop_back();

				i--;
				continue;
			}

			// check for duplication and mirroring
			for (std::size_t j = 0 ; j < i ; j++ )
			{
				if (_buildBrush.sides[i].planenum == _buildBrush.sides[j].planenum)
				{
					globalWarningStream() << 
						(boost::format("Entity %i, Brush %i: duplicate plane") % 
						 _buildBrush.entitynum % _buildBrush.brushnum).str() << std::endl;

					// remove the second duplicate
					for (std::size_t k = i + 1 ; k < _buildBrush.sides.size(); ++k)
					{
						_buildBrush.sides[k-1] = _buildBrush.sides[k];
					}

					_buildBrush.sides.pop_back();

					i--;
					break;
				}

				if (_buildBrush.sides[i].planenum == (_buildBrush.sides[j].planenum ^ 1))
				{
					// mirror plane, brush is invalid
					globalWarningStream() << 
						(boost::format("Entity %i, Brush %i: mirrored plane") % 
						 _buildBrush.entitynum % _buildBrush.brushnum).str() << std::endl;

					return false;
				}
			}
		}

		return true;
	}

	// Finds or insert the given plane
	std::size_t findOrInsertPlane(const Plane3& plane)
	{
		return _procFile->planes.findOrInsertPlane(plane, EPSILON_NORMAL, EPSILON_DIST);
	}
};

class ToolDataGenerator :
	public scene::NodeVisitor
{
private:
	ProcFilePtr _procFile;

public:
	ToolDataGenerator(const ProcFilePtr& procFile) :
		_procFile(procFile)
	{}

	bool pre(const scene::INodePtr& node)
	{
		IEntityNodePtr entityNode = boost::dynamic_pointer_cast<IEntityNode>(node);

		if (entityNode)
		{
			_procFile->entities.push_back(ProcEntityPtr(new ProcEntity(entityNode)));

			// Traverse this entity's primitives
			ToolPrimitiveGenerator primitiveGenerator(*_procFile->entities.back(), _procFile);
			node->traverse(primitiveGenerator);

			// Check if this is a light
			const Entity& entity = entityNode->getEntity();
			
			if (entity.getKeyValue("classname") == "light")
			{
				createMapLight(entity);
			}

			return false; // processed => stop traversal here
		}

		return true;
	}

	void buildStats()
	{
		_procFile->mapBounds = AABB();

		if (_procFile->entities.empty())
		{
			return;
		}

		// Accumulate worldspawn primitives
		const ProcEntity& entity = **_procFile->entities.begin();

		for (std::size_t p = 0; p < entity.primitives.size(); ++p)
		{
			if (entity.primitives[p].brush)
			{
				_procFile->numWorldBrushes++;
				_procFile->mapBounds.includeAABB(entity.primitives[p].brush->bounds);
			}
			else if (!entity.primitives[p].patch.empty())
			{
				_procFile->numWorldTriSurfs++;
			}
		}
	}

private:
	void createMapLight(const Entity& entity)
	{
		// designers can add the "noPrelight" flag to signal that
		// the lights will move around, so we don't want
		// to bother chopping up the surfaces under it or creating
		// shadow volumes
		if (entity.getKeyValue("noPrelight") == "1")
		{
			return;
		}

		_procFile->lights.push_back(ProcLight());

		ProcLight& light = _procFile->lights.back();

		// get the name for naming the shadow surfaces
		light.name = entity.getKeyValue("name");

		// TODO light->shadowTris = NULL;

		// parse parms exactly as the game do
		// use the game's epair parsing code so
		// we can use the same renderLight generation
		//gameEdit->ParseSpawnArgsToRenderLight( &mapEnt->epairs, &light->def.parms );
		light.parseFromSpawnargs(entity);
		
		// fills everything in based on light.parms
		light.deriveLightData();

		// Check the name
		if (light.name.empty())
		{
			globalErrorStream() <<
				(boost::format("Light at (%f,%f,%f) didn't have a name") %
				light.parms.origin[0], light.parms.origin[1], light.parms.origin[2] );

			_procFile->lights.pop_back();
			return;
		}
	}
};

}

void ProcCompiler::generateBrushData()
{
	ToolDataGenerator generator(_procFile);
	_root->traverse(generator);

	generator.buildStats();

	globalOutputStream() << (boost::format("%5i total world brushes") % _procFile->numWorldBrushes).str() << std::endl;
	globalOutputStream() << (boost::format("%5i total world triSurfs") % _procFile->numWorldTriSurfs).str() << std::endl;
	globalOutputStream() << (boost::format("%5i patches") % _procFile->numPatches).str() << std::endl;
	globalOutputStream() << (boost::format("%5i entities") % _procFile->entities.size()).str() << std::endl;
	globalOutputStream() << (boost::format("%5i planes") % _procFile->planes.size()).str() << std::endl;
	globalOutputStream() << (boost::format("%5i areaportals") % _procFile->numPortals).str() << std::endl;

	Vector3 minBounds = _procFile->mapBounds.origin - _procFile->mapBounds.extents;
	Vector3 maxBounds = _procFile->mapBounds.origin + _procFile->mapBounds.extents;

	globalOutputStream() << (boost::format("size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f") % 
		minBounds[0] % minBounds[1] % minBounds[2] % maxBounds[0] % maxBounds[1] % maxBounds[2]).str() << std::endl;
}

bool ProcCompiler::processModels()
{
	for (std::size_t i = 0; i < _procFile->entities.size(); ++i)
	{
		ProcEntity& entity = *_procFile->entities[i];

		if (entity.primitives.empty())
		{
			continue;
		}

		globalOutputStream() << "############### entity " << i << " ###############" << std::endl;

		// if we leaked, stop without any more processing, only floodfill the first entity (world)
		if (!processModel(entity, i == 0))
		{
			return false;
		}
	}

	return true;
}

void ProcCompiler::makeStructuralProcFaceList(const ProcEntity::Primitives& primitives)
{
	std::size_t brushCount = 0;
	std::size_t nonOpaqueCount = 0;

	for (ProcEntity::Primitives::const_reverse_iterator i = primitives.rbegin();
		i != primitives.rend(); ++i)
	{
		if (!i->brush) continue; // skip all patches

		brushCount++;

		ProcBrush& brush = *i->brush;

		if (!brush.opaque && !(brush.contents & Material::SURF_AREAPORTAL))
		{
			nonOpaqueCount++;
			continue; // skip all non-opaque non-portals
		}

		for (ProcBrush::ProcFaces::const_iterator s = brush.sides.begin(); s != brush.sides.end(); ++s)
		{
			const ProcFace& side = *s;

			if (side.winding.empty()) continue;

			int sideFlags = side.material->getSurfaceFlags();

			// Skip all faces of a portal brush that are not textured with the "areaportal" face
			if ((brush.contents & Material::SURF_AREAPORTAL) && !(sideFlags & Material::SURF_AREAPORTAL))
			{
				continue;
			}

			// Allocate a new BspFace
			_bspFaces.push_back(BspFacePtr(new BspFace()));
			BspFace& face = *_bspFaces.back();

			// Check if this is a portal face
			face.portal = (sideFlags & Material::SURF_AREAPORTAL) != 0;
			face.w = side.winding;
			face.planenum = side.planenum & ~1; // use the even plane number
		}
	}
}

#define	BLOCK_SIZE	1024

std::size_t ProcCompiler::selectSplitPlaneNum(const BspTreeNodePtr& node, BspFaces& faces)
{
	// if it is crossing a 1k block boundary, force a split
	// this prevents epsilon problems from extending an
	// arbitrary distance across the map

	Vector3 halfSize = node->bounds.extents;
	Vector3 nodeMin = node->bounds.origin - node->bounds.extents;
	Vector3 nodeMax = node->bounds.origin + node->bounds.extents;

	for (int axis = 0; axis < 3; ++axis)
	{
		float dist;

		if (halfSize[axis] > BLOCK_SIZE)
		{
			dist = BLOCK_SIZE * ( floor( (nodeMin[axis] + halfSize[axis]) / BLOCK_SIZE ) + 1.0f );
		}
		else
		{
			dist = BLOCK_SIZE * ( floor( nodeMin[axis] / BLOCK_SIZE ) + 1.0f );
		}

		if (dist > nodeMin[axis] + 1.0f && dist < nodeMax[axis] - 1.0f)
		{
			Plane3 plane(0, 0, 0, dist);
			plane.normal()[axis] = 1.0f;

			return _procFile->planes.findOrInsertPlane(plane, EPSILON_NORMAL, EPSILON_DIST);
		}
	}

	// pick one of the face planes
	// if we have any portal faces at all, only
	// select from them, otherwise select from
	// all faces
	int bestValue = -999999;
	BspFaces::const_reverse_iterator bestSplit = faces.rbegin();

	bool havePortals = false;

	for (BspFaces::const_reverse_iterator split = faces.rbegin(); split != faces.rend(); ++split)
	{
		(*split)->checked = false;

		if ((*split)->portal)
		{
			havePortals = true;
		}
	}

	for (BspFaces::const_reverse_iterator split = faces.rbegin(); split != faces.rend(); ++split)
	{
		if ((*split)->checked) continue;

		// greebo: prefer portals as split planes, if we have some
		if (havePortals != (*split)->portal) continue;

		const Plane3& mapPlane = _procFile->planes.getPlane((*split)->planenum);

		int splits = 0;
		int facing = 0;
		int front = 0;
		int back = 0;

		for (BspFaces::const_reverse_iterator check = faces.rbegin(); check != faces.rend(); ++check)
		{
			if ((*check)->planenum == (*split)->planenum)
			{
				facing++;
				(*check)->checked = true;	// won't need to test this plane again
				continue;
			}

			int side = (*check)->w.planeSide(mapPlane);

			if (side == SIDE_CROSS)
			{
				splits++;
			}
			else if (side == SIDE_FRONT)
			{
				front++;
			}
			else if (side == SIDE_BACK)
			{
				back++;
			}
		}

		int value = 5*facing - 5*splits; // - abs(front-back);

		if (PlaneSet::getPlaneType(mapPlane) < PlaneSet::PLANETYPE_TRUEAXIAL)
		{
			value += 5;		// axial is better
		}

		if (value > bestValue)
		{
			bestValue = value;
			bestSplit = split;
		}
	}

	if (bestValue == -999999)
	{
		return std::numeric_limits<std::size_t>::max();
	}

	return (*bestSplit)->planenum;
}

void ProcCompiler::buildFaceTreeRecursively(const BspTreeNodePtr& node, BspFaces& faces, BspTree& tree)
{
	std::size_t splitPlaneNum = selectSplitPlaneNum(node, faces);

	// if we don't have any more faces, this is a node
	if (splitPlaneNum == std::numeric_limits<std::size_t>::max())
	{
		node->planenum = PLANENUM_LEAF;
		tree.numFaceLeafs++;
		return;
	}

	// partition the list
	node->planenum = splitPlaneNum;

	const Plane3& plane = _procFile->planes.getPlane(splitPlaneNum);

	BspFaces childLists[2];

	BspFaces::reverse_iterator next;

	// greebo: We use a reverse iterator since D3 is pushing planes to front of the list,
	// this way we keep the same order
	for (BspFaces::reverse_iterator split = faces.rbegin(); split != faces.rend(); split = next )
	{
		next = split + 1; // remember the pointer to next

		if ((*split)->planenum == node->planenum)
		{
			split->reset();
			continue;
		}

		int side = (*split)->w.planeSide(plane);

		if (side == SIDE_CROSS)
		{
			// Split into front and back winding
			ProcWinding front;
			ProcWinding back;

			(*split)->w.split(plane, CLIP_EPSILON * 2, front, back);

			if (!front.empty())
			{
				childLists[0].push_back(BspFacePtr(new BspFace));

				childLists[0].back()->w = front;
				childLists[0].back()->planenum = (*split)->planenum;
			}

			if (!back.empty())
			{
				childLists[1].push_back(BspFacePtr(new BspFace));

				childLists[1].back()->w = back;
				childLists[1].back()->planenum = (*split)->planenum;
			}

			split->reset();
		}
		else if (side == SIDE_FRONT)
		{
			childLists[0].push_back(*split);
		}
		else if ( side == SIDE_BACK )
		{
			childLists[1].push_back(*split);
		}
	}

	// recursively process children
	for (std::size_t i = 0; i < 2; ++i)
	{
		node->children[i].reset(new BspTreeNode);
		node->children[i]->parent = node.get();
		node->children[i]->bounds = node->bounds;
	}

	// split the bounds if we have a nice axial plane
	for (std::size_t i = 0; i < 3; ++i)
	{
		if (fabs(plane.normal()[i] - 1.0f) < 0.001f)
		{
			// greebo: calculate the new origin and extents, if we set the children[0].min's and children[1].max's components to dist
			float distHalf = plane.dist() * 0.5f;
			float base = 0.5f * (node->children[0]->bounds.origin[i] + node->children[0]->bounds.extents[i]);
			
			node->children[0]->bounds.origin[i] = base + distHalf;
			node->children[0]->bounds.extents[i] = base - distHalf;

			base = 0.5f * (node->children[1]->bounds.extents[i] - node->children[1]->bounds.origin[i]);

			node->children[1]->bounds.origin[i] = distHalf - base;
			node->children[1]->bounds.extents[i] = distHalf + base;
			break;
		}
	}

	for (std::size_t i = 0; i < 2; ++i)
	{
		buildFaceTreeRecursively(node->children[i], childLists[i], tree);
	}

	// Cleanup
	faces.clear();
}

void ProcCompiler::faceBsp(ProcEntity& entity)
{
	globalOutputStream() << "--- FaceBSP: " << _bspFaces.size() << " faces ---" << std::endl;

	entity.tree.bounds = AABB();

	// Accumulate bounds
	for (BspFaces::const_iterator f = _bspFaces.begin(); f != _bspFaces.end(); ++f)
	{
		for (std::size_t i = 0; i < (*f)->w.size(); ++i)
		{
			entity.tree.bounds.includePoint((*f)->w[i].vertex);
		}
	}

	// Allocate the head node and use the total bounds
	entity.tree.head.reset(new BspTreeNode);
	entity.tree.head->bounds = entity.tree.bounds;

	buildFaceTreeRecursively(entity.tree.head, _bspFaces, entity.tree);

	globalOutputStream() << (boost::format("%5i leafs") % entity.tree.numFaceLeafs).str() << std::endl;

	//common->Printf( "%5.1f seconds faceBsp\n", ( end - start ) / 1000.0 );
}

void ProcCompiler::addPortalToNodes(const ProcPortalPtr& portal, const BspTreeNodePtr& front, const BspTreeNodePtr& back)
{
	if (portal->nodes[0] || portal->nodes[1])
	{
		globalErrorStream() << "AddPortalToNode: already included" << std::endl;
		return;
	}

	//globalOutputStream() << (boost::format("Adding portal %d to node front = %d and back = %d") % portal->portalId % front->nodeId % back->nodeId) << std::endl;

	portal->nodes[0] = front;
	portal->nodes[1] = back;

	// Add the given portal to the front and back node
	portal->next[0] = front->portals;
	front->portals = portal;

	portal->next[1] = back->portals;
	back->portals = portal;
}

void ProcCompiler::removePortalFromNode(const ProcPortalPtr& portal, const BspTreeNodePtr& node)
{
	ProcPortalPtr* portalRef = &node->portals;

	//globalOutputStream() << (boost::format("Removing portal %d from node %d") % portal->portalId % node->nodeId) << std::endl;
	
	// remove reference to the current portal
	while (true)
	{
		ProcPortalPtr& test = *portalRef;

		if (!test)
		{
			globalErrorStream() << "RemovePortalFromNode: portal not bounding leaf" << std::endl;
			return;
		}

		if (test == portal)
		{
			break; // found
		}

		if (test->nodes[0] == node)
		{
			portalRef = &(test->next[0]);
		}
		else if (test->nodes[1] == node)
		{
			portalRef = &(test->next[1]);
		}
		else
		{
			globalErrorStream() << "removePortalFromNode: portal not in leaf" << std::endl;	
			return;
		}
	}
	
	if (portal->nodes[0] == node)
	{
		*portalRef = portal->next[0];
		portal->nodes[0].reset();
	} 
	else if (portal->nodes[1] == node)
	{
		*portalRef = portal->next[1];	
		portal->nodes[1].reset();
	}
	else
	{
		globalErrorStream() << "removePortalFromNode: mislinked" << std::endl;
	}	
}

void ProcCompiler::makeHeadNodePortals(BspTree& tree)
{
	tree.outside->planenum = PLANENUM_LEAF;
	tree.outside->nodeId = 9999;
	tree.outside->brushlist.clear();
	tree.outside->portals.reset();
	tree.outside->opaque = false;

	BspTreeNodePtr& node = tree.head;

	// if no nodes, don't go any farther
	if (node->planenum == PLANENUM_LEAF)
	{
		return;
	}

	AABB bounds = tree.bounds;

	// pad with some space so there will never be null volume leafs
	static const float SIDESPACE = 8;
	bounds.extendBy(Vector3(SIDESPACE, SIDESPACE, SIDESPACE));

	// greebo: Six planes/portals to separate the whole tree from the "outside"
	// Each portal has the head node as front and the outside as back node
	ProcPortalPtr portals[6];
	Plane3 planes[6];

	for (std::size_t i = 0; i < 3; ++i)
	{
		for (std::size_t j = 0; j < 2 ; ++j)
		{
			std::size_t n = j*3 + i;

			portals[n].reset(new ProcPortal);

			_numActivePortals++;
			if (_numActivePortals > _numPeakPortals)
			{
				_numPeakPortals = _numActivePortals;
			}
	
			Plane3& pl = planes[n];
			pl = Plane3(0,0,0,0);

			if (j) // j == 1
			{
				pl.normal()[i] = -1;
				pl.dist() = -(bounds.origin + bounds.extents)[i];
			}
			else // j == 0
			{
				pl.normal()[i] = 1;
				pl.dist() = (bounds.origin - bounds.extents)[i];
			}

			portals[n]->plane = pl;
			portals[n]->winding.setFromPlane(pl);
			
			addPortalToNodes(portals[n], node, tree.outside);
		}
	}

	// clip the (near-infinitely large) windings by all the other planes
	for (std::size_t i = 0; i < 6; ++i)
	{
		for (std::size_t j = 0; j < 6; ++j)
		{
			if (j == i) continue;

			portals[i]->winding.clip(planes[j], ON_EPSILON);
		}
	}
}

void ProcCompiler::calculateNodeBounds(const BspTreeNodePtr& node)
{
	// calc mins/maxs for both leafs and nodes
	node->bounds = AABB();
	std::size_t s = 0;

	// Use raw pointers to avoid constant shared_ptr assigments
	for (ProcPortal* p = node->portals.get(); p != NULL; p = p->next[s].get())
	{
		s = (p->nodes[1] == node) ? 1 : 0;

		for (std::size_t i = 0; i < p->winding.size(); ++i)
		{
			node->bounds.includePoint(p->winding[i].vertex);
		}
	}
}

ProcWinding ProcCompiler::getBaseWindingForNode(const BspTreeNodePtr& node)
{
	ProcWinding winding(_procFile->planes.getPlane(node->planenum));

	// clip by all the parents
	BspTreeNode* nodeRaw = node.get(); // FIXME
	for (BspTreeNode* n = node->parent; n != NULL && !winding.empty(); )
	{
		const Plane3& plane = _procFile->planes.getPlane(n->planenum);
		static const float BASE_WINDING_EPSILON = 0.001f;

		if (n->children[0].get() == nodeRaw)
		{
			// take front
			winding.clip(plane, BASE_WINDING_EPSILON);
		} 
		else
		{
			// take back
			winding.clip(-plane, BASE_WINDING_EPSILON);
		}

		nodeRaw = n;
		n = n->parent;
	}

	return winding;
}

void ProcCompiler::makeNodePortal(const BspTreeNodePtr& node)
{
	ProcWinding w = getBaseWindingForNode(node);

	std::size_t side;

	// clip the portal by all the other portals in the node
	for (ProcPortal* p = node->portals.get(); p != NULL && !w.empty(); p = p->next[side].get())
	{
		Plane3 plane;

		if (p->nodes[0] == node)
		{
			side = 0;
			plane = p->plane;
		}
		else if (p->nodes[1] == node)
		{
			side = 1;
			plane = -p->plane;
		}
		else
		{
			globalErrorStream() << "makeNodePortal: mislinked portal" << std::endl;
			side = 0;	// quiet a compiler warning
			return;
		}

		w.clip(plane, CLIP_EPSILON);
	}

	if (w.empty())
	{
		return;
	}

	if (w.isTiny())
	{
		_numTinyPortals++;
		w.clear();
		return;
	}
	
	ProcPortalPtr newPortal(new ProcPortal);

	newPortal->plane = _procFile->planes.getPlane(node->planenum);
	newPortal->onnode = node;
	newPortal->winding = w;

	addPortalToNodes(newPortal, node->children[0], node->children[1]);
}

void ProcCompiler::splitNodePortals(const BspTreeNodePtr& node)
{
	const Plane3& plane = _procFile->planes.getPlane(node->planenum);

	const BspTreeNodePtr& front = node->children[0];
	const BspTreeNodePtr& back = node->children[1];

	//globalOutputStream() << "-- Split node portals on node " << node->nodeId << std::endl;

	ProcPortalPtr nextPortal;

	for (ProcPortalPtr portal = node->portals; portal; portal = nextPortal)
	{
		std::size_t side;

		if (portal->nodes[0] == node)
		{
			side = 0;
		}
		else if (portal->nodes[1] == node)
		{
			side = 1;
		}
		else
		{
			globalErrorStream() << "splitNodePortals: mislinked portal" << std::endl;
			side = 0;	// quiet a compiler warning
		}

		nextPortal = portal->next[side];

		// Remember the other node before removing the portal from them
		BspTreeNodePtr otherNode = portal->nodes[!side];

		removePortalFromNode(portal, portal->nodes[0]);
		removePortalFromNode(portal, portal->nodes[1]);

		// cut the portal into two portals, one on each side of the cut plane
		ProcWinding frontwinding;
		ProcWinding backwinding;
		portal->winding.split(plane, SPLIT_WINDING_EPSILON, frontwinding, backwinding);

		if (!frontwinding.empty() && frontwinding.isTiny())
		{
			//globalOutputStream() << " Discarding tiny portal " << portal->portalId << std::endl;

			frontwinding.clear();
			_numTinyPortals++;
		}

		if (!backwinding.empty() && backwinding.isTiny())
		{
			//globalOutputStream() << " Discarding portal " << portal->portalId << std::endl;

			backwinding.clear();
			_numTinyPortals++;
		}

		if (frontwinding.empty() && backwinding.empty())
		{	
			continue; // tiny windings on both sides
		}

		if (frontwinding.empty())
		{
			//globalOutputStream() << " No front winding " << portal->portalId << std::endl;

			backwinding.clear();

			if (side == 0)
			{
				addPortalToNodes(portal, back, otherNode);
			}
			else
			{
				addPortalToNodes(portal, otherNode, back);
			}

			continue;
		}

		if (backwinding.empty())
		{
			//globalOutputStream() << " No back winding " << portal->portalId << std::endl;

			frontwinding.clear();

			if (side == 0)
			{
				addPortalToNodes(portal, front, otherNode);
			}
			else
			{
				addPortalToNodes(portal, otherNode, front);
			}

			continue;
		}

		//globalOutputStream() << " Splitting portal " << portal->portalId << std::endl;
		
		// the winding is split
		ProcPortalPtr newPortal(new ProcPortal(*portal)); // copy-construct
		newPortal->winding = backwinding;
		
		portal->winding = frontwinding;

		if (side == 0)
		{
			addPortalToNodes(portal, front, otherNode);
			addPortalToNodes(newPortal, back, otherNode);
		}
		else
		{
			addPortalToNodes(portal, otherNode, front);
			addPortalToNodes(newPortal, otherNode, back);
		}
	}

	node->portals.reset();
}

void ProcCompiler::makeTreePortalsRecursively(const BspTreeNodePtr& node)
{
	calculateNodeBounds(node);

	Vector3 minbounds = node->bounds.origin - node->bounds.extents;
	Vector3 maxBounds = node->bounds.origin + node->bounds.extents;

	if (node->bounds.extents.getLengthSquared() <= 0)
	{
		globalWarningStream() << "node without a volume" << std::endl;
	}

	Vector3 boundsMin = node->bounds.origin - node->bounds.extents;
	Vector3 boundsMax = node->bounds.origin + node->bounds.extents;

	for (std::size_t i = 0; i < 3; ++i)
	{
		if (boundsMin[i] < MIN_WORLD_COORD || boundsMax[i] > MAX_WORLD_COORD )
		{
			globalWarningStream() << "node with unbounded volume" << std::endl;
			break;
		}
	}

	if (node->planenum == PLANENUM_LEAF)
	{
		return;
	}

	makeNodePortal(node);
	splitNodePortals(node);

	makeTreePortalsRecursively(node->children[0]);
	makeTreePortalsRecursively(node->children[1]);
}

void ProcCompiler::makeTreePortals(BspTree& tree)
{
	globalOutputStream() << "----- MakeTreePortals -----" << std::endl;

	makeHeadNodePortals(tree);

	makeTreePortalsRecursively(tree.head);
}

float ProcCompiler::calculateBrushVolume(const ProcBrushPtr& brush) 
{
	if (!brush) return 0;

	ProcWinding* w = NULL;
	std::size_t i = 0;

	// grab the first valid point as the corner
	for (i = 0; i < brush->sides.size(); ++i)
	{
		w = &brush->sides[i].winding;

		if (!w->empty())
			break;
	}

	if (w->empty())
	{
		return 0;
	}

	Vector3 corner = (*w)[0].vertex;

	// make tetrahedrons to all other faces
	float volume = 0;

	for (; i < brush->sides.size(); ++i)
	{
		const ProcWinding& winding = brush->sides[i].winding;

		if (winding.empty())
		{
			continue;
		}

		const Plane3& plane = _procFile->planes.getPlane(brush->sides[i].planenum);

		float d = -plane.distanceToPoint(corner);
		float area = winding.getArea();
		volume += d * area;
	}

	return volume / 3;
}

void ProcCompiler::splitBrush(const ProcBrushPtr& brush, std::size_t planenum, ProcBrushPtr& front, ProcBrushPtr& back)
{
	const Plane3& plane = _procFile->planes.getPlane(planenum);

	// check all points
	float d_front = 0;
	float d_back = 0;

	for (std::size_t i = 0; i < brush->sides.size(); ++i)
	{
		ProcWinding& w = brush->sides[i].winding;

		for (std::size_t j = 0; j < w.size(); ++j)
		{
			float d = plane.distanceToPoint(w[j].vertex);

			if (d > 0 && d > d_front)
			{
				d_front = d;
			}

			if (d < 0 && d < d_back)
			{
				d_back = d;
			}
		}
	}

	if (d_front < 0.1f) // PLANESIDE_EPSILON)
	{	// only on back
		back.reset(new ProcBrush(*brush)); // copy
		return;
	}

	if (d_back > -0.1) // PLANESIDE_EPSILON)
	{	// only on front
		front.reset(new ProcBrush(*brush)); // copy
		return;
	}

	// create a new winding from the split plane
	ProcWinding w(plane);

	for (std::size_t i = 0; i < brush->sides.size() && !w.empty(); ++i)
	{
		const Plane3& plane2 = _procFile->planes.getPlane(brush->sides[i].planenum ^ 1);
		w.clip(plane2, 0);
	}

	if (w.empty() || w.isTiny())
	{
		// the brush isn't really split
		int side = brush->mostlyOnSide(plane);

		if (side == PSIDE_FRONT)
		{
			front.reset(new ProcBrush(*brush));
		}

		if (side == PSIDE_BACK)
		{
			back.reset(new ProcBrush(*brush));
		}

		return;
	}

	if (w.isHuge())
	{
		globalWarningStream() << "huge winding" << std::endl;
	}

	ProcWinding midwinding = w;

	// split it for real

	ProcBrushPtr parts[2];

	for (std::size_t i = 0; i < 2; ++i)
	{
		parts[i].reset(new ProcBrush(*brush));

		parts[i]->sides.clear(); // reserve(brush->sides.size() + 1);
		parts[i]->original = brush->original;
	}

	// split all the current windings
	ProcWinding cw[2];

	for (std::size_t i = 0; i < brush->sides.size(); ++i)
	{
		ProcFace& side = brush->sides[i];
		ProcWinding w = side.winding;

		if (w.empty()) continue;

		w.split(plane, 0 /*PLANESIDE_EPSILON*/, cw[0], cw[1]);

		for (std::size_t j = 0; j < 2; ++j)
		{
			if (cw[j].empty()) continue;

			parts[j]->sides.push_back(side);
			parts[j]->sides.back().winding.swap(cw[j]);
		}
	}

	// see if we have valid polygons on both sides

	for (std::size_t i = 0 ; i < 2 ; ++i)
	{
		if (!parts[i]->bound())
		{
			break;
		}

		if (parts[i]->sides.size() < 3)
		{
			parts[i].reset();
		}
	}

	if (!(parts[0] && parts[1]))
	{
		if (!parts[0] && !parts[1])
		{
			globalOutputStream() << "split removed brush" << std::endl;
		}
		else
		{
			globalOutputStream() << "split not on both sides" << std::endl;
		}

		if (parts[0])
		{
			parts[0].reset();
			front.reset(new ProcBrush(*brush)); // copy
		}

		if (parts[1])
		{
			parts[1].reset();
			back.reset(new ProcBrush(*brush)); // copy
		}

		return;
	}

	// add the midwinding to both sides
	for (std::size_t i = 0; i < 2; ++i)
	{
		parts[i]->sides.push_back(ProcFace());

		parts[i]->sides.back().planenum = planenum^i^1;
		
		if (i==0)
		{
			parts[i]->sides.back().winding = midwinding; // copy
		}
		else
		{
			parts[i]->sides.back().winding.swap(midwinding); // move
		}
	}

	{
		for (std::size_t i = 0; i < 2; ++i)
		{
			float v1 = calculateBrushVolume(parts[i]);

			if (v1 < 1.0f)
			{
				parts[i].reset();
			}
		}
	}

	front.swap(parts[0]); // assign return values
	back.swap(parts[1]);
}

std::size_t ProcCompiler::filterBrushIntoTreeRecursively(const ProcBrushPtr& brush, const BspTreeNodePtr& node)
{
	if (!brush)
	{
		return 0;
	}

	// add it to the leaf list
	if (node->planenum == PLANENUM_LEAF)
	{
		node->brushlist.push_back(brush);

		// classify the leaf by the structural brush
		if (brush->opaque)
		{
			node->opaque = true;
		}

		return 1;
	}

	// split it by the node plane
	ProcBrushPtr front;
	ProcBrushPtr back;
	splitBrush(brush, node->planenum, front, back);

	std::size_t count = 0;

	count += filterBrushIntoTreeRecursively(front, node->children[0]);
	count += filterBrushIntoTreeRecursively(back, node->children[1]);

	return count;
}

void ProcCompiler::filterBrushesIntoTree(ProcEntity& entity)
{
	globalOutputStream() << "----- FilterBrushesIntoTree -----" << std::endl;

	_numUniqueBrushes = 0;
	_numClusters = 0;

	for (ProcEntity::Primitives::const_iterator prim = entity.primitives.begin(); prim != entity.primitives.end(); ++prim)
	{
		const ProcBrushPtr& brush = prim->brush;

		if (!brush) continue;
		
		_numUniqueBrushes++;

		// Copy the brush
		ProcBrushPtr newBrush(new ProcBrush(*brush));

		_numClusters += filterBrushIntoTreeRecursively(newBrush, entity.tree.head);
	}

	globalOutputStream() << (boost::format("%5i total brushes") % _numUniqueBrushes).str() << std::endl;
	globalOutputStream() << (boost::format("%5i cluster references") % _numClusters).str() << std::endl;
}

#if 1 // Debug helpers
inline std::string printPortals(const BspTreeNodePtr& node)
{
	if (!node->portals) return "-";

	std::string rv;
	std::size_t s;
	std::size_t count = 0;

	for (ProcPortalPtr p = node->portals; p; p = p->next[s])
	{
		if (p->nodes[0] == node)
		{
			s = 0;
		}
		else
		{
			s = 1;
		}

		count++;

		rv += (!rv.empty()) ? ", " : "";
		rv += sizetToStr(p->portalId);

		rv += (s == 0) ? " (f)" : " (b)";
	}

	rv += " [count: " + sizetToStr(count) + "]";

	return rv;
}

inline void printBrushCount(const BspTreeNodePtr& node, std::size_t level)
{
	for (std::size_t i = 0; i < level; ++i) globalOutputStream() << " ";
	globalOutputStream() << level << ": node " << node->nodeId << " has " << node->brushlist.size() << " brushes." << std::endl;

	for (std::size_t i = 0; i < level; ++i) globalOutputStream() << " ";
	globalOutputStream() << level << ": node " << node->nodeId << " has portals: " << printPortals(node) << std::endl;

	if (node->children[0])
	{
		printBrushCount(node->children[0], level+1);
	}

	if (node->children[1])
	{
		printBrushCount(node->children[1], level+1);
	}
}
#endif

void ProcCompiler::floodPortalsRecursively(const BspTreeNodePtr& node, int dist)
{
	if (node->occupied)
	{
		return;
	}

	if (node->opaque)
	{
		return;
	}

	_numFloodedLeafs++;
	node->occupied = dist;

	for (ProcPortal* p = node->portals.get(); p != NULL; )
	{
		std::size_t s = p->nodes[1] == node ? 0 : 1;

		floodPortalsRecursively(p->nodes[s], dist + 1);

		p = p->next[1-s].get();
	}
}

bool ProcCompiler::placeOccupant(const BspTreeNodePtr& node, const Vector3& origin, const ProcEntityPtr& entity)
{
	assert(node);

	// find the leaf to start in
	BspTreeNodePtr nodeIter = node;

	while (nodeIter->planenum != PLANENUM_LEAF)
	{
		const Plane3& plane = _procFile->planes.getPlane(nodeIter->planenum);

		float d = plane.distanceToPoint(origin);

		if (d >= 0.0f)
		{
			nodeIter = nodeIter->children[0];
		}
		else
		{
			nodeIter = nodeIter->children[1];
		}

		assert(nodeIter);
	}

	if (nodeIter->opaque)
	{
		return false;
	}

	nodeIter->occupant = entity;

	floodPortalsRecursively(nodeIter, 1);

	return true;
}

bool ProcCompiler::floodEntities(BspTree& tree)
{
	const BspTreeNodePtr& headnode = tree.head;

	globalOutputStream() << "--- FloodEntities ---" << std::endl;

	bool inside = false;
	tree.outside->occupied = 0;

	_numFloodedLeafs = 0;
	bool errorShown = false;

	for (std::size_t i = 1; i < _procFile->entities.size(); ++i)
	{
		Entity& mapEnt = _procFile->entities[i]->mapEntity->getEntity();
		
		std::string originStr = mapEnt.getKeyValue("origin");

		if (originStr.empty())
		{
			continue;
		}

		Vector3 origin(originStr);

		// any entity can have "noFlood" set to skip it
		if (!mapEnt.getKeyValue("noFlood").empty())
		{
			continue;
		}

		std::string className = mapEnt.getKeyValue("classname");
		
		if (className == "light")
		{
			// don't place lights that have a light_start field, because they can still
			// be valid if their origin is outside the world
			if (!mapEnt.getKeyValue("light_start").empty())
			{
				continue;
			}

			// don't place fog lights, because they often
			// have origins outside the light
			std::string texture = mapEnt.getKeyValue("texture");

			if (!texture.empty())
			{
				MaterialPtr mat = GlobalMaterialManager().getMaterialForName(texture);

				if (mat->isFogLight())
				{
					continue;
				}
			}
		}

		if (placeOccupant(headnode, origin, _procFile->entities[i]))
		{
			inside = true;
		}

		if (tree.outside->occupied && !errorShown)
		{
			errorShown = true;
			globalErrorStream() << "Leak on entity #" << i << std::endl;
			globalErrorStream() << "Entity classname was " << mapEnt.getKeyValue("classname") << std::endl;
			globalErrorStream() << "Entity name was " << mapEnt.getKeyValue("name") << std::endl;
			globalErrorStream() << "Entity origin is " << Vector3(mapEnt.getKeyValue("origin")) << std::endl;
		}
	}

	globalOutputStream() << (boost::format("%5i flooded leafs") % _numFloodedLeafs).str() << std::endl;

	if (!inside)
	{
		globalOutputStream() << "no entities in open -- no filling" << std::endl;
	}
	else if (tree.outside->occupied > 0)
	{
		globalOutputStream() << "entity reached from outside -- no filling" << std::endl;
	}

	return inside && tree.outside->occupied == 0;
}

void ProcCompiler::fillOutsideRecursively(const BspTreeNodePtr& node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		fillOutsideRecursively(node->children[0]);
		fillOutsideRecursively(node->children[1]);
		return;
	}

	// anything not reachable by an entity
	// can be filled away
	if (!node->occupied)
	{
		if (!node->opaque)
		{
			_numOutsideLeafs++;
			node->opaque = true;
		} 
		else
		{
			_numSolidLeafs++;
		}
	}
	else
	{
		_numInsideLeafs++;
	}
}

void ProcCompiler::fillOutside(const ProcEntity& entity)
{
	_numOutsideLeafs = 0;
	_numInsideLeafs = 0;
	_numSolidLeafs = 0;

	globalOutputStream() << "--- FillOutside ---" << std::endl;

	fillOutsideRecursively(entity.tree.head);

	globalOutputStream() << (boost::format("%5i solid leafs") % _numSolidLeafs).str() << std::endl;
	globalOutputStream() << (boost::format("%5i leafs filled") % _numOutsideLeafs).str() << std::endl;
	globalOutputStream() << (boost::format("%5i inside leafs") % _numInsideLeafs).str() << std::endl;
}

void ProcCompiler::clipSideByTreeRecursively(ProcWinding& winding, ProcFace& side, const BspTreeNodePtr& node)
{
	if (winding.empty()) return;

	if (node->planenum != PLANENUM_LEAF)
	{
		if (side.planenum == node->planenum)
		{
			clipSideByTreeRecursively(winding, side, node->children[0]);
			return;
		}

		if (side.planenum == (node->planenum ^ 1))
		{
			clipSideByTreeRecursively(winding, side, node->children[1]);
			return;
		}

		ProcWinding front;
		ProcWinding back;
		winding.split(_procFile->planes.getPlane(node->planenum), ON_EPSILON, front, back);
		
		clipSideByTreeRecursively(front, side, node->children[0]);
		clipSideByTreeRecursively(back, side, node->children[1]);
		return;
	}

	// if opaque leaf, don't add
	if (!node->opaque)
	{
		if (side.visibleHull.empty())
		{
			side.visibleHull.swap(winding);
		}
		else
		{
			side.visibleHull.addToConvexHull(winding, _procFile->planes.getPlane(side.planenum).normal());
		}
	}
}

void ProcCompiler::clipSidesByTree(ProcEntity& entity)
{
	globalOutputStream() << "----- ClipSidesByTree -----" << std::endl;

	for (ProcEntity::Primitives::const_iterator prim = entity.primitives.begin(); prim != entity.primitives.end(); ++prim)
	{
		const ProcBrushPtr& brush = prim->brush;

		if (!brush) continue;

		for (std::size_t i = 0; i < brush->sides.size(); ++i)
		{
			ProcFace& side = brush->sides[i];

			if (side.winding.empty()) continue;
			
			ProcWinding winding(side.winding); // copy
			
			side.visibleHull.clear();

			clipSideByTreeRecursively(winding, side, entity.tree.head);

			// FIXME: Implement noClipSide option?
		}
	}
}

void ProcCompiler::clearAreasRecursively(const BspTreeNodePtr& node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		clearAreasRecursively(node->children[0]);
		clearAreasRecursively(node->children[1]);
		return;
	}

	node->area = -1;
}

bool ProcCompiler::portalIsPassable(const ProcPortal& portal)
{
	if (!portal.onnode)
	{
		return false;	// to global outsideleaf
	}

	if (portal.nodes[0]->planenum != PLANENUM_LEAF || portal.nodes[1]->planenum != PLANENUM_LEAF)
	{
		globalErrorStream() << "ProcCompiler::portalIsPassable: not a leaf" << std::endl;
		return false;
	}

	if (!portal.nodes[0]->opaque && !portal.nodes[1]->opaque )
	{
		return true;
	}

	return false;
}

ProcFace* ProcCompiler::findSideForPortal(const ProcPortalPtr& portal)
{
	// scan both bordering nodes brush lists for a portal brush
	// that shares the plane
	for (std::size_t i = 0; i < 2; ++i)
	{
		BspTreeNodePtr node = portal->nodes[i];

		for (BspTreeNode::Brushes::const_iterator b = node->brushlist.begin(); b != node->brushlist.end(); ++b)
		{
			ProcBrush& brush = **b;

			if (!(brush.contents & Material::SURF_AREAPORTAL))
			{
				continue;
			}

			ProcBrushPtr orig = brush.original.lock();

			assert(orig);

			for (std::size_t j = 0 ; j < orig->sides.size(); ++j)
			{
				ProcFace& s = orig->sides[j];

				if (s.visibleHull.empty())
				{
					continue;
				}

				if (!(s.material->getSurfaceFlags() & Material::SURF_AREAPORTAL))
				{
					continue;
				}

				if ((s.planenum & ~1) != (portal->onnode->planenum & ~1))
				{
					continue;
				}

				// remove the visible hull from any other portal sides of this portal brush
				for (std::size_t k = 0; k < orig->sides.size(); ++k)
				{
					if (k == j)
					{
						continue;
					}

					ProcFace& s2 = orig->sides[k];

					if (s2.visibleHull.empty())
					{
						continue;
					}

					if (!(s2.material->getSurfaceFlags() & Material::SURF_AREAPORTAL))
					{
						continue;
					}

					Vector3 center = s2.visibleHull.getCenter();

					globalWarningStream() << "brush has multiple area portal sides at " << center << std::endl;

					s2.visibleHull.clear();
				}

				return &s;
			}
		}
	}

	return NULL;
}

void ProcCompiler::floodAreasRecursively(const BspTreeNodePtr& node)
{
	if (node->area != -1)
	{
		return;		// already got it
	}

	if (node->opaque)
	{
		return;
	}

	_numAreaFloods++;
	node->area = _numAreas;

	std::size_t s = 0;
	for (ProcPortalPtr p = node->portals; p; p = p->next[s])
	{
		s = (p->nodes[1] == node);
		const BspTreeNodePtr& other = p->nodes[1-s];

		if (!portalIsPassable(*p))
		{
			continue;
		}

		// can't flood through an area portal
		if (findSideForPortal(p))
		{
			continue;
		}

		floodAreasRecursively(other);
	}
}

void ProcCompiler::findAreasRecursively(const BspTreeNodePtr& node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		findAreasRecursively(node->children[0]);
		findAreasRecursively(node->children[1]);
		return;
	}

	if (node->opaque)
	{
		return;
	}

	if (node->area != -1)
	{
		return;		// already got it
	}

	_numAreaFloods = 0;
	floodAreasRecursively(node);

	globalOutputStream() << (boost::format("Area %i has %i leafs") % _numAreas % _numAreaFloods) << std::endl;

	_numAreas++;
}

void ProcCompiler::checkAreasRecursively(const BspTreeNodePtr& node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		checkAreasRecursively(node->children[0]);
		checkAreasRecursively(node->children[1]);
		return;
	}

	if (!node->opaque && node->area < 0)
	{
		globalErrorStream() << "ProcCompiler::checkAreasRecursively: area = %i" << node->area << std::endl;
	}
}

void ProcCompiler::findInterAreaPortalsRecursively(const BspTreeNodePtr& node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		findInterAreaPortalsRecursively(node->children[0]);
		findInterAreaPortalsRecursively(node->children[1]);
		return;
	}

	if (node->opaque)
	{
		return;
	}

	std::size_t s = 0;
	for (ProcPortalPtr p = node->portals; p; p = p->next[s])
	{
		s = (p->nodes[1] == node) ? 1 : 0;
		const BspTreeNodePtr& other = p->nodes[1-s];

		if (other->opaque)
		{
			continue;
		}

		// only report areas going from lower number to higher number
		// so we don't report the portal twice
		if (other->area <= node->area)
		{
			continue;
		}

		ProcFace* side = findSideForPortal(p);

		if (side == NULL)
		{
			globalWarningStream() << "findSideForPortal failed at " << p->winding.getCenter() << std::endl;
			continue;
		}

		const ProcWinding& w = side->visibleHull;

		if (w.empty())
		{
			continue;
		}

		// see if we have created this portal before
		std::size_t i = 0;

		for (i = 0; i < _procFile->interAreaPortals.size(); ++i)
		{
			ProcInterAreaPortal& iap = _procFile->interAreaPortals[i];

			if (side == iap.side &&
				((p->nodes[0]->area == iap.area0 && p->nodes[1]->area == iap.area1) || 
				 (p->nodes[1]->area == iap.area0 && p->nodes[0]->area == iap.area1))) 
			{
				break;
			}
		}

		if (i != _procFile->interAreaPortals.size())
		{
			continue;	// already emitted
		}

		_procFile->interAreaPortals.push_back(ProcInterAreaPortal());
		ProcInterAreaPortal& iap = _procFile->interAreaPortals.back();

		if (side->planenum == p->onnode->planenum)
		{
			iap.area0 = p->nodes[0]->area;
			iap.area1 = p->nodes[1]->area;
		}
		else
		{
			iap.area0 = p->nodes[1]->area;
			iap.area1 = p->nodes[0]->area;
		}

		iap.side = side;
	}
}

void ProcCompiler::floodAreas(ProcEntity& entity)
{
	globalOutputStream() <<	"--- FloodAreas ---" << std::endl;

	// set all areas to -1
	clearAreasRecursively(entity.tree.head);

	// flood fill from non-opaque areas
	_numAreas = 0;

	findAreasRecursively(entity.tree.head);

	globalOutputStream() << (boost::format("%5i areas") % _numAreas) << std::endl;
	entity.numAreas = _numAreas;

	// make sure we got all of them
	checkAreasRecursively(entity.tree.head);

	// identify all portals between areas if this is the world
	if (&entity == _procFile->entities.begin()->get())
	{
		_procFile->interAreaPortals.clear();
		findInterAreaPortalsRecursively(entity.tree.head);

		globalOutputStream() << (boost::format("%5i interarea portals") % _procFile->interAreaPortals.size()) << std::endl;
	}
}

ProcTris ProcCompiler::triangleListForSide(const ProcFace& side, const ProcWinding& winding)
{
	const MaterialPtr& si = side.material;

	// skip any generated faces
	if (!si) return ProcTris();
	
	// don't create faces for non-visible sides
	if (!si->surfaceCastsShadow() && !si->isDrawn())
	{
		return ProcTris();
	}

#if 1
	// triangle fan using only the outer verts
	// this gives the minimum triangle count,
	// but may have some very distended triangles
	ProcTris triList;

	for (std::size_t i = 2; i < winding.size(); ++i)
	{
		triList.push_back(ProcTri());
		ProcTri& tri = triList.back();

		tri.material = si;	
		
		for (std::size_t j = 0; j < 3; ++j)
		{
			const Vector3* vec;

			if (j == 0)
			{
				vec = &(winding[0].vertex);
			} 
			else if ( j == 1 )
			{
				vec = &(winding[i-1].vertex);
			} 
			else
			{
				vec = &(winding[i].vertex);
			}

			ArbitraryMeshVertex& dv = tri.v[j];
#if 0
			// round the xyz to a given precision
			for ( k = 0 ; k < 3 ; k++ )
			{
				dv->xyz[k] = SNAP_INT_TO_FLOAT * floor( vec[k] * SNAP_FLOAT_TO_INT + 0.5 );
			}
#else
			dv.vertex = *vec;
#endif
				
			// calculate texture s/t from brush primitive texture matrix
			dv.texcoord[0] = dv.vertex.dot(side.texVec[0].getVector3()) + side.texVec[0][3];
			dv.texcoord[1] = dv.vertex.dot(side.texVec[1].getVector3()) + side.texVec[1][3];

			// copy normal
			dv.normal = _procFile->planes.getPlane(side.planenum).normal();

			if (dv.normal.getLength() < 0.9f || dv.normal.getLength() > 1.1f)
			{
				globalErrorStream() << "Bad normal in TriListForSide" << std::endl;
				return triList;
			}
		}
	}
#else
	// triangle fan from central point, more verts and tris, but less distended
	// I use this when debugging some tjunction problems
	triList = NULL;
	for ( i = 0 ; i < w->GetNumPoints() ; i++ ) {
		idVec3	midPoint;

		tri = AllocTri();
		tri->material = si;	
		tri->next = triList;
		triList = tri;

		for ( j = 0 ; j < 3 ; j++ ) {
			if ( j == 0 ) {
				vec = &midPoint;
				midPoint = w->GetCenter();
			} else if ( j == 1 ) {
				vec = &((*w)[i]).ToVec3();
			} else {
				vec = &((*w)[(i+1)%w->GetNumPoints()]).ToVec3();
			}

			dv = tri->v + j;

			VectorCopy( *vec, dv->xyz );
				
			// calculate texture s/t from brush primitive texture matrix
			dv->st[0] = DotProduct( dv->xyz, s->texVec.v[0] ) + s->texVec.v[0][3];
			dv->st[1] = DotProduct( dv->xyz, s->texVec.v[1] ) + s->texVec.v[1][3];

			// copy normal
			dv->normal = dmapGlobals.mapPlanes[s->planenum].Normal();
			if ( dv->normal.Length() < 0.9f || dv->normal.Length() > 1.1f ) {
				common->Error( "Bad normal in TriListForSide" );
			}
		}
	}
#endif

	// set merge groups if needed, to prevent multiple sides from being
	// merged into a single surface in the case of gui shaders, mirrors, and autosprites
	if (side.material->isDiscrete())
	{
		for (ProcTris::iterator tri = triList.begin(); tri != triList.end(); ++tri)
		{
			tri->mergeGroup = &side;
		}
	}

	return triList;
}

std::size_t ProcCompiler::checkWindingInAreasRecursively(const ProcWinding& winding, const BspTreeNodePtr& node)
{
	assert(!winding.empty());

	if (node->planenum != PLANENUM_LEAF)
	{
		ProcWinding front;
		ProcWinding back;
		winding.split(_procFile->planes.getPlane(node->planenum), ON_EPSILON, front, back);

		std::size_t	a1 = front.empty() ? 0 : checkWindingInAreasRecursively(front, node->children[0]);
		std::size_t a2 = back.empty() ? 0 : checkWindingInAreasRecursively(back, node->children[1]);
		
		if (a1 == MULTIAREA_CROSS || a2 == MULTIAREA_CROSS)
		{
			return MULTIAREA_CROSS;	// different
		}

		if (a1 == -1)
		{
			return a2;	// one solid
		}

		if (a2 == -1)
		{
			return a1;	// one solid
		}

		if (a1 != a2)
		{
			return MULTIAREA_CROSS;	// cross areas
		}

		return a1;
	}

	return node->area;
}

#define	 TEXTURE_OFFSET_EQUAL_EPSILON	0.005f
#define	 TEXTURE_VECTOR_EQUAL_EPSILON	0.001f

void ProcCompiler::addTriListToArea(ProcEntity& entity, const ProcTris& triList, 
									std::size_t planeNum, std::size_t areaNum, 
									Vector4 texVec[2])
{
	if (triList.empty())
	{
		return;
	}

	ProcArea& area = entity.areas[areaNum];
	ProcArea::OptimizeGroups::iterator group;

	for (group = area.groups.begin(); group != area.groups.end(); ++group)
	{
		if (group->material == triList[0].material && 
			group->planeNum == planeNum && 
			(group->mergeGroup == triList[0].mergeGroup || 
			 group->mergeSurf == triList[0].mergeSurf || 
			 group->mergePatch == triList[0].mergePatch))
		{
			// check the texture vectors
			std::size_t i = 0;

			for (i = 0; i < 2; ++i)
			{
				std::size_t j = 0;

				for (j = 0; j < 3; ++j)
				{
					if (fabs(texVec[i][j] - group->texVec[i][j]) > TEXTURE_VECTOR_EQUAL_EPSILON)
					{
						break;
					}
				}

				if (j != 3)
				{
					break;
				}

				if (fabs(texVec[i][3] - group->texVec[i][3]) > TEXTURE_OFFSET_EQUAL_EPSILON)
				{
					break;
				}
			}

			if (i == 2)
			{
				break;	// exact match
			}
			/*else
			{
				// different texture offsets
				i = 1;	// just for debugger breakpoint
			}*/
		}
	}

	if (group == area.groups.end())
	{
		area.groups.push_back(ProcOptimizeGroup());
		group = area.groups.end() - 1;

		group->numGroupLights = 0;
		group->smoothed = false;
		group->surfaceEmitted = false;

		group->planeNum = planeNum;
		group->mergeGroup = triList[0].mergeGroup;
		group->mergeSurf = triList[0].mergeSurf;
		group->mergePatch = triList[0].mergePatch;
		group->material = triList[0].material;
		group->texVec[0] = texVec[0];
		group->texVec[1] = texVec[1];
	}

	// Append the incoming triangles to the existing ones in the group
	group->triList.insert(group->triList.end(), triList.begin(), triList.end());
}

void ProcCompiler::putWindingIntoAreasRecursively(ProcEntity& entity, const ProcWinding& winding, 
	ProcFace& side, const BspTreeNodePtr& node)
{
	if (winding.empty()) return;

	if (node->planenum != PLANENUM_LEAF)
	{
		if (side.planenum == node->planenum)
		{
			putWindingIntoAreasRecursively(entity, winding, side, node->children[0]);
			return;
		}

		if (side.planenum == (node->planenum ^ 1))
		{
			putWindingIntoAreasRecursively(entity, winding, side, node->children[1]);
			return;
		}

		// see if we need to split it
		// adding the "noFragment" flag to big surfaces like sky boxes
		// will avoid potentially dicing them up into tons of triangles
		// that take forever to optimize back together
		if (/*FIXME !dmapGlobals.fullCarve || */side.material->getSurfaceFlags() & Material::SURF_NOFRAGMENT)
		{
			std::size_t area = checkWindingInAreasRecursively(winding, node);

			if (area != MULTIAREA_CROSS)
			{
				ProcTris tris = triangleListForSide(side, winding);

				addTriListToArea(entity, tris, side.planenum, area, side.texVec);
				return;
			}
		}

		ProcWinding front;
		ProcWinding back;
		winding.split(_procFile->planes.getPlane(node->planenum), ON_EPSILON, front, back);

		putWindingIntoAreasRecursively(entity, front, side, node->children[0]);
		putWindingIntoAreasRecursively(entity, back, side, node->children[1]);

		return;
	}

	// if opaque leaf, don't add
	if (node->area >= 0 && !node->opaque)
	{
		ProcTris tris = triangleListForSide(side, winding);
		addTriListToArea(entity, tris, side.planenum, node->area, side.texVec);
	}
}

// TODO: Make this a member method of ProcTri
inline void getTexVecForTri(Vector4 texVec[2], const ProcTri& tri)
{
	float d0[5];
	float d1[5];

	const ArbitraryMeshVertex& a = tri.v[0];
	const ArbitraryMeshVertex& b = tri.v[1];
	const ArbitraryMeshVertex& c = tri.v[2];

	d0[0] = b.vertex[0] - a.vertex[0];
	d0[1] = b.vertex[1] - a.vertex[1];
	d0[2] = b.vertex[2] - a.vertex[2];
	d0[3] = b.texcoord[0] - a.texcoord[0];
	d0[4] = b.texcoord[1] - a.texcoord[1];

	d1[0] = c.vertex[0] - a.vertex[0];
	d1[1] = c.vertex[1] - a.vertex[1];
	d1[2] = c.vertex[2] - a.vertex[2];
	d1[3] = c.texcoord[0] - a.texcoord[0];
	d1[4] = c.texcoord[1] - a.texcoord[1];

	float area = d0[3] * d1[4] - d0[4] * d1[3];
	float inva = 1.0f / area;

	Vector3	temp(
		(d0[0] * d1[4] - d0[4] * d1[0]) * inva,
		(d0[1] * d1[4] - d0[4] * d1[1]) * inva,
		(d0[2] * d1[4] - d0[4] * d1[2]) * inva
	);

	temp.normalise();

	texVec[0].x() = temp.x();
	texVec[0].y() = temp.y();
	texVec[0].z() = temp.z();

	texVec[0][3] = tri.v[0].vertex.dot(texVec[0].getVector3()) - tri.v[0].texcoord[0];

    temp[0] = (d0[3] * d1[0] - d0[0] * d1[3]) * inva;
    temp[1] = (d0[3] * d1[1] - d0[1] * d1[3]) * inva;
    temp[2] = (d0[3] * d1[2] - d0[2] * d1[3]) * inva;

	temp.normalise();

	texVec[1].x() = temp.x();
	texVec[1].y() = temp.y();
	texVec[1].z() = temp.z();
	texVec[1][3] = tri.v[0].vertex.dot(texVec[0].getVector3()) - tri.v[0].texcoord[1];
}

// TODO: Make this a member method of ProcTri
// Regenerate the texcoords and colors on a fragmented tri from the plane equations
void triVertsFromOriginal(ProcTri& tri, const ProcTri& original)
{
	float denom = ProcWinding::getTriangleArea(original.v[0].vertex, original.v[1].vertex, original.v[2].vertex);

	if (denom == 0)
	{
		return;		// original was degenerate, so it doesn't matter
	}
	
	for (std::size_t i = 0; i < 3; ++i)
	{
		// find the barycentric coordinates
		float a = ProcWinding::getTriangleArea(tri.v[i].vertex, original.v[1].vertex, original.v[2].vertex ) / denom;
		float b = ProcWinding::getTriangleArea(tri.v[i].vertex, original.v[2].vertex, original.v[0].vertex ) / denom;
		float c = ProcWinding::getTriangleArea(tri.v[i].vertex, original.v[0].vertex, original.v[1].vertex ) / denom;

		// regenerate the interpolated values
		tri.v[i].texcoord[0] = a * original.v[0].texcoord[0] + b * original.v[1].texcoord[0] + c * original.v[2].texcoord[0];
		tri.v[i].texcoord[1] = a * original.v[0].texcoord[1] + b * original.v[1].texcoord[1] + c * original.v[2].texcoord[1];

		for (std::size_t j = 0; j < 3; ++j)
		{
			tri.v[i].normal[j] = a * original.v[0].normal[j] + b * original.v[1].normal[j] + c * original.v[2].normal[j];
		}

		tri.v[i].normal.normalise();
	}
}

inline ProcTris windingToTriList(const ProcWinding& w, const ProcTri& originalTri)
{
	assert(!w.empty());

	ProcTris triList;

	for (std::size_t i = 2 ; i < w.size(); ++i)
	{
		triList.push_back(originalTri);

		ProcTri& tri = triList.back();

		for (std::size_t j = 0; j < 3; ++j)
		{
			if (j == 0)
			{
				tri.v[j].vertex = w[0].vertex;
			}
			else if (j == 1)
			{
				tri.v[j].vertex = w[i-1].vertex;
			} 
			else
			{
				tri.v[j].vertex = w[i].vertex;
			}
		}

		triVertsFromOriginal(tri, originalTri);
	}

	return triList;
}

void ProcCompiler::clipTriIntoTreeRecursively(const ProcWinding& winding, const ProcTri& originalTri, 
											  ProcEntity& entity, const BspTreeNodePtr& node)
{
	assert(!winding.empty());

	if (node->planenum != PLANENUM_LEAF)
	{
		ProcWinding front;
		ProcWinding back;

		winding.split(_procFile->planes.getPlane(node->planenum), ON_EPSILON, front, back);

		if (!front.empty())
		{
			clipTriIntoTreeRecursively(front, originalTri, entity, node->children[0]);
		}

		if (!back.empty())
		{
			clipTriIntoTreeRecursively(back, originalTri, entity, node->children[1]);
		}

		return;
	}

	// if opaque leaf, don't add
	if (!node->opaque && node->area != MULTIAREA_CROSS)
	{
		ProcTris list = windingToTriList(winding, originalTri);

		Plane3 plane(originalTri.v[0].vertex, originalTri.v[1].vertex, originalTri.v[2].vertex);

		std::size_t planeNum = _procFile->planes.findOrInsertPlane(plane, EPSILON_NORMAL, EPSILON_DIST);

		Vector4 texVec[2];
		getTexVecForTri(texVec, originalTri);

		addTriListToArea(entity, list, planeNum, node->area, texVec);
	}
}

void ProcCompiler::addMapTrisToAreas(const ProcTris& tris, ProcEntity& entity)
{
	for (ProcTris::const_iterator tri = tris.begin(); tri != tris.end(); ++tri)
	{
		// skip degenerate triangles from pinched curves
		if (ProcWinding::getTriangleArea(tri->v[0].vertex, tri->v[1].vertex, tri->v[2].vertex) <= 0)
		{
			continue;
		}

		// FIXME: fullCarve can be disabled, is enabled by default
		if (true/* dmapGlobals.fullCarve*/)
		{
			// always fragment into areas
			ProcWinding w(tri->v[0].vertex, tri->v[1].vertex, tri->v[2].vertex);
			clipTriIntoTreeRecursively(w, *tri, entity, entity.tree.head);
			continue;
		}

		ProcWinding w(tri->v[0].vertex, tri->v[1].vertex, tri->v[2].vertex);

		std::size_t area = checkWindingInAreasRecursively(w, entity.tree.head);

		if (area != MULTIAREA_CROSS)
		{
			// put in single area
			ProcTris newTri(1, *tri); // list with 1 triangle

			Plane3 plane(tri->v[0].vertex, tri->v[1].vertex, tri->v[2].vertex);

			std::size_t planeNum = _procFile->planes.findOrInsertPlane(plane, EPSILON_NORMAL, EPSILON_DIST);

			Vector4 texVec[2];
			getTexVecForTri(texVec, newTri[0]);

			addTriListToArea(entity, newTri, planeNum, area, texVec);
		} 
		else
		{
			// fragment into areas
			clipTriIntoTreeRecursively(w, *tri, entity, entity.tree.head);
		}
	}
}

void ProcCompiler::putPrimitivesInAreas(ProcEntity& entity)
{
	globalOutputStream() << "----- PutPrimitivesInAreas -----" << std::endl;

	// allocate space for surface chains for each area
	entity.areas.resize(entity.numAreas);

	// for each primitive, clip it to the non-solid leafs
	// and divide it into different areas
	std::size_t primCount = 0;
	for (ProcEntity::Primitives::const_reverse_iterator prim = entity.primitives.rbegin();
		prim != entity.primitives.rend(); ++prim, ++primCount)
	{
		const ProcBrushPtr& brush = prim->brush;

		if (!brush)
		{
			//globalOutputStream() << "--- Primitive: " << primCount << " (patch) " << std::endl;

			// add curve triangles
			addMapTrisToAreas(prim->patch, entity);
			
			/*// Print result
			for (std::size_t a = 0; a < entity.areas.size(); ++a)
			{
				globalOutputStream() << " Area " << a << ": ";
		
				globalOutputStream() << entity.areas[a].groups.size() << " groups" << std::endl;

				for (ProcArea::OptimizeGroups::const_reverse_iterator g = entity.areas[a].groups.rbegin(); 
					g != entity.areas[a].groups.rend(); ++g)
				{
					globalOutputStream() << "  plane " << g->planeNum << ", " <<
						g->triList.size() << " tris" << std::endl;
				}
			}*/

			continue;
		}

		//globalOutputStream() << "--- Primitive: " << primCount << " (brush " << brush->brushnum << ") " << std::endl;

		// clip in brush sides
		for (std::size_t i = 0; i < brush->sides.size(); ++i)
		{
			ProcFace& side = brush->sides[i];

			if (side.visibleHull.empty())
			{
				continue;
			}

			putWindingIntoAreasRecursively(entity, side.visibleHull, side, entity.tree.head);
		}

		// Print result
		/*for (std::size_t a = 0; a < entity.areas.size(); ++a)
		{
			globalOutputStream() << " Area " << a << ": ";
		
			globalOutputStream() << entity.areas[a].groups.size() << " groups" << std::endl;

			for (ProcArea::OptimizeGroups::const_reverse_iterator g = entity.areas[a].groups.rbegin(); 
				g != entity.areas[a].groups.rend(); ++g)
			{
				globalOutputStream() << "  plane " << g->planeNum << ", " <<
					g->triList.size() << " tris" << std::endl;
			}
		}*/
	}

	// optionally inline some of the func_static models
	if (&entity == _procFile->entities[0].get())
	{
		IEntityNodePtr worldspawn = _procFile->entities[0]->mapEntity;

		bool inlineAll = worldspawn->getEntity().getKeyValue("inlineAllStatics") == "1";

		for (std::size_t eNum = 1; eNum < _procFile->entities.size(); ++eNum)
		{
			ProcEntity& entity = *_procFile->entities[eNum];
			Entity& mapEnt = entity.mapEntity->getEntity();

			if (mapEnt.getKeyValue("classname") != "func_static")
			{
				continue;
			}

			if (mapEnt.getKeyValue("inline") != "1" && !inlineAll)
			{
				continue;
			}

			std::string modelName = mapEnt.getKeyValue("model");

			if (modelName.empty())
			{
				continue;
			}

			model::IModelPtr model = GlobalModelCache().getModel(modelName);
			
			if (model == NULL)
			{
				globalWarningStream() << "Cannot inline entity " << mapEnt.getKeyValue("name") <<
					" since the model cannot be loaded: " << modelName << std::endl;
				continue;
			}

			globalOutputStream() << "inlining " << mapEnt.getKeyValue("name") << std::endl;
			
			// get the rotation matrix in either full form, or single angle form
			std::string rotation = mapEnt.getKeyValue("rotation");

			Matrix4 axis;

			if (rotation.empty())
			{
				float angle = strToFloat(mapEnt.getKeyValue("angle"));

				// idMath::AngleNormalize360
				if (angle >= 360.0f || angle < 0.0f)
				{
					angle -= floor(angle / 360.0f) * 360.0f;
				}

				axis = Matrix4::getRotationAboutZDegrees(angle);
			}
			else
			{
				axis = Matrix4::getRotation(rotation);
			}

			Vector3 origin = Vector3(mapEnt.getKeyValue("origin"));

			for (int i = 0; i < model->getSurfaceCount(); ++i)
			{
				const model::IModelSurface& surface = model->getSurface(i);

				MaterialPtr material = GlobalMaterialManager().getMaterialForName(surface.getDefaultMaterial());

				ProcTris tris;

				int numTris = surface.getNumTriangles();
				for (int j = 0; j < numTris; ++j)
				{
					tris.push_back(ProcTri());
					ProcTri& tri = tris.back();

					tri.material = material;
					
					if (material->isDiscrete())
					{
						tri.mergeSurf = &surface;
					}

					model::ModelPolygon poly = surface.getPolygon(j);
					
					tri.v[0].vertex = axis.transformPoint(poly.a.vertex) + origin;
					tri.v[0].normal = axis.transformDirection(poly.a.normal);
					tri.v[0].texcoord = poly.a.texcoord;

					tri.v[1].vertex = axis.transformPoint(poly.b.vertex) + origin;
					tri.v[1].normal = axis.transformDirection(poly.b.normal);
					tri.v[1].texcoord = poly.b.texcoord;

					tri.v[2].vertex = axis.transformPoint(poly.c.vertex) + origin;
					tri.v[2].normal = axis.transformDirection(poly.c.normal);
					tri.v[2].texcoord = poly.c.texcoord;

					// greebo: This is probably the point where the normals should be renormalised to
					// fix the weird lighting when applying "rotation hack scaling"
				}

				addMapTrisToAreas(tris, entity);
			}
		}
	}

	/*// Print result
	for (std::size_t a = 0; a < entity.areas.size(); ++a)
	{
		globalOutputStream() << "Area " << a << ": ";
		
		globalOutputStream() << entity.areas[a].groups.size() << " groups" << std::endl;

		for (std::size_t g = 0; g < entity.areas[a].groups.size(); ++g)
		{
			globalOutputStream() << "  Group " << g << ": Plane " << entity.areas[a].groups[g].planeNum << std::endl;
		}
	}*/
}

void ProcCompiler::boundOptimizeGroup(ProcOptimizeGroup& group)
{
	group.bounds = AABB();

	for (ProcTris::reverse_iterator tri = group.triList.rbegin(); tri != group.triList.rend(); ++tri)
	{
		group.bounds.includePoint(tri->v[0].vertex);
		group.bounds.includePoint(tri->v[1].vertex);
		group.bounds.includePoint(tri->v[2].vertex);
	}
}

void ProcCompiler::clipTriByLight(const ProcLight& light, const ProcTri& tri, ProcTris& in, ProcTris& out)
{
	in.clear();
	out.clear();

	// clip this winding to the light
	ProcWinding inside(tri.v[0].vertex, tri.v[1].vertex, tri.v[2].vertex);
	ProcWinding outside[6];

	bool hasOutside = false;
	ProcWinding oldInside;

	for (std::size_t i = 0; i < 6 ; ++i)
	{
		oldInside = inside;

		if (!oldInside.empty())
		{
			oldInside.split(light.getFrustumPlane(i), 0, outside[i], inside);
			oldInside.clear();
		}
		else
		{
			outside[i].clear();
		}

		if (!outside[i].empty())
		{
			hasOutside = true;
		}
	}

	if (inside.empty())
	{
		// the entire winding is outside this light
		out.push_back(tri);
		return;
	}

	if (!hasOutside)
	{
		// the entire winding is inside this light
		in.push_back(tri);
		return;
	}

	// the winding is split
	in = windingToTriList(inside, tri);
	inside.clear();

	// combine all the outside fragments
	for (std::size_t i = 0; i < 6; ++i)
	{
		if (!outside[i].empty())
		{
			ProcTris temp = windingToTriList(outside[i], tri);
			out.insert(out.end(), temp.begin(), temp.end());
		}
	}
}

std::size_t ProcCompiler::countGroupListTris(ProcArea::OptimizeGroups& groupList)
{
	std::size_t c = 0;

	for (ProcArea::OptimizeGroups::const_iterator group = groupList.begin(); group != groupList.end(); ++group)
	{
		c += group->triList.size();
	}

	return c;
}

void ProcCompiler::hashTriangles(ProcArea::OptimizeGroups& groups)
{
	// clear the hash tables
	_triangleHash.reset(new TriangleHash);

	// bound all the triangles to determine the bucket size
	_triangleHash->calculateBounds(groups);

	_triangleHash->hashTriangles(groups);
}

void ProcCompiler::fixAreaGroupsTjunctions(ProcArea::OptimizeGroups& groups)
{
	if (false/*dmapGlobals.noTJunc*/) return; // FIXME

	if (groups.empty()) return;

	if (false/*dmapGlobals.verbose*/) // FIXME
	{
		std::size_t startCount = countGroupListTris(groups);
		globalOutputStream() << "----- FixAreaGroupsTjunctions -----" << std::endl;
		globalOutputStream() << (boost::format("%6i triangles in") % startCount) << std::endl;
	}

	hashTriangles(groups);

	for (ProcArea::OptimizeGroups::reverse_iterator group = groups.rbegin();
		 group != groups.rend(); ++group)
	{
		// don't touch discrete surfaces
		if (group->material && group->material->isDiscrete())
		{
			continue;
		}

		ProcTris newList;

		for (ProcTris::const_iterator tri = group->triList.begin(); tri != group->triList.end(); ++tri)
		{
			_triangleHash->fixTriangleAgainstHash(*tri, newList);
		}

		group->triList.swap(newList);
	}

	if (false/*dmapGlobals.verbose*/) // FIXME
	{
		std::size_t endCount = countGroupListTris(groups);
		globalOutputStream() << (boost::format("%6i triangles out") % endCount) << std::endl;
	}
}

void ProcCompiler::optimizeOptList(ProcOptimizeGroup& group)
{
	ProcArea::OptimizeGroups tempList(1, group);

	// fix the t junctions among this single list
	// so we can match edges
	// can we avoid doing this if colinear vertexes break edges?
	fixAreaGroupsTjunctions(tempList);
	
	/*// create the 2D vectors
	dmapGlobals.mapPlanes[opt->planeNum].Normal().NormalVectors( opt->axis[0], opt->axis[1] );

	AddOriginalEdges( opt );
	SplitOriginalEdgesAtCrossings( opt );

#if 0
	// seperate any discontinuous areas for individual optimization
	// to reduce the scope of the problem
	SeparateIslands( opt );
#else
	DontSeparateIslands( opt );
#endif

	// now free the hash verts
	FreeTJunctionHash();
	_triangleHash.reset();

	// free the original list and use the new one
	FreeTriList( opt->triList );
	opt->triList = opt->regeneratedTris;
	opt->regeneratedTris = NULL;*/
}

void ProcCompiler::optimizeGroupList(ProcArea::OptimizeGroups& groupList)
{
	if (groupList.empty()) return;

	std::size_t c_in = countGroupListTris(groupList);

	// optimize and remove colinear edges, which will
	// re-introduce some t junctions
	for (ProcArea::OptimizeGroups::reverse_iterator group = groupList.rbegin(); 
		 group != groupList.rend(); ++group)
	{
		optimizeOptList(*group);
	}

	/*std::size_t c_edge = CountGroupListTris( groupList );

	// fix t junctions again
	FixAreaGroupsTjunctions( groupList );
	FreeTJunctionHash();
	std::size_t c_tjunc2 = CountGroupListTris( groupList );

	SetGroupTriPlaneNums( groupList );

	common->Printf( "----- OptimizeAreaGroups Results -----\n" );
	common->Printf( "%6i tris in\n", c_in );
	common->Printf( "%6i tris after edge removal optimization\n", c_edge );
	common->Printf( "%6i tris after final t junction fixing\n", c_tjunc2 );*/
}

Surface ProcCompiler::createLightShadow(ProcArea::OptimizeGroups& shadowerGroups, const ProcLight& light)
{
	globalOutputStream() << (boost::format("----- CreateLightShadow %p -----") % (&light)) << std::endl;

	// optimize all the groups
	optimizeGroupList(shadowerGroups);

	Surface shadowTris;
	/*// combine all the triangles into one list
	mapTri_t	*combined;

	combined = NULL;
	for ( optimizeGroup_t *group = shadowerGroups ; group ; group = group->nextGroup ) {
		combined = MergeTriLists( combined, CopyTriList( group->triList ) );
	}

	if ( !combined ) {
		return NULL;
	}

	// find uniqued vertexes
	srfTriangles_t	*occluders = ShareMapTriVerts( combined );

	FreeTriList( combined );

	// find silhouette information for the triSurf
	R_CleanupTriangles( occluders, false, true, false );

	// let the renderer build the shadow volume normally
	idRenderEntityLocal		space;

	space.modelMatrix[0] = 1;
	space.modelMatrix[5] = 1;
	space.modelMatrix[10] = 1;
	space.modelMatrix[15] = 1;

	srfCullInfo_t cullInfo;
	memset( &cullInfo, 0, sizeof( cullInfo ) );

	// call the normal shadow creation, but with the superOptimize flag set, which will
	// call back to SuperOptimizeOccluders after clipping the triangles to each frustum
	srfTriangles_t	*shadowTris;
	if ( dmapGlobals.shadowOptLevel == SO_MERGE_SURFACES ) {
		shadowTris = R_CreateShadowVolume( &space, occluders, &light->def, SG_STATIC, cullInfo );
	} else {
		shadowTris = R_CreateShadowVolume( &space, occluders, &light->def, SG_OFFLINE, cullInfo );
	}
	R_FreeStaticTriSurf( occluders );

	R_FreeInteractionCullInfo( cullInfo );

	if ( shadowTris ) {
		dmapGlobals.totalShadowTriangles += shadowTris->numIndexes / 3;
		dmapGlobals.totalShadowVerts += shadowTris->numVerts / 3;
	}*/

	return shadowTris;
}

void ProcCompiler::buildLightShadows(ProcEntity& entity, ProcLight& light)
{
	//
	// build a group list of all the triangles that will contribute to
	// the optimized shadow volume, leaving the original triangles alone
	//

	// shadowers will contain all the triangles that will contribute to the
	// shadow volume
	ProcArea::OptimizeGroups shadowerGroups;
	const Vector3& lightOrigin = light.getGlobalLightOrigin();
	bool hasPerforatedSurface = false;

	// if the light is no-shadows, don't add any surfaces
	// to the beam tree at all
	if (!light.parms.noShadows && light.getLightShader()->lightCastsShadows())
	{
		for (std::size_t i = 0; i < entity.numAreas; ++i)
		{
			const ProcArea& area = entity.areas[i];

			for (ProcArea::OptimizeGroups::const_reverse_iterator group = area.groups.rbegin(); 
				 group != area.groups.rend(); ++group)
			{
				// if the surface doesn't cast shadows, skip it
				if (!group->material->surfaceCastsShadow())
				{
					continue;
				}

				// if the group doesn't face away from the light, it
				// won't contribute to the shadow volume
				if (_procFile->planes.getPlane(group->planeNum).distanceToPoint(lightOrigin) > 0)
				{
					continue;
				}

				// if the group bounds doesn't intersect the light bounds,
				// skip it
				if (!group->bounds.intersects(light.getFrustumTris().bounds))
				{
					continue;
				}

				// build up a list of the triangle fragments inside the
				// light frustum
				ProcTris shadowers;

				for (ProcTris::const_reverse_iterator tri = group->triList.rbegin();
					 tri != group->triList.rend(); ++tri)
				{
					// clip it to the light frustum
					ProcTris in;
					ProcTris out;

					clipTriByLight(light, *tri, in, out);
					
					shadowers.insert(shadowers.end(), in.begin(), in.end());
				}

				// if we didn't get any out of this group, we don't
				// need to create a new group in the shadower list
				if (shadowers.empty())
				{
					continue;
				}

				// find a group in shadowerGroups to add these to
				// we will ignore everything but planenum, and we
				// can merge across areas
				ProcArea::OptimizeGroups::iterator check = shadowerGroups.begin();

				for ( ; check != shadowerGroups.end(); ++check)
				{
					if (check->planeNum == group->planeNum)
					{
						break;
					}
				}

				if (check == shadowerGroups.end())
				{
					shadowerGroups.push_back(*group);
					check = shadowerGroups.end() - 1;
				}

				// if any surface is a shadow-casting perforated or translucent surface, we
				// can't use the face removal optimizations because we can see through
				// some of the faces
				if (group->material->getCoverage() != Material::MC_OPAQUE)
				{
					hasPerforatedSurface = true;
				}

				check->triList.insert(check->triList.end(), shadowers.begin(), shadowers.end());
			}
		}
	}

	// take the shadower group list and create a beam tree and shadow volume
	light.shadowTris = createLightShadow(shadowerGroups, light);

	/* TODO
	if ( light->shadowTris && hasPerforatedSurface ) {
		// can't ever remove front faces, because we can see through some of them
		light->shadowTris->numShadowIndexesNoCaps = light->shadowTris->numShadowIndexesNoFrontCaps = 
			light->shadowTris->numIndexes;
	}*/

	// we don't need the original shadower triangles for anything else
	//FreeOptimizeGroupList( shadowerGroups );
}

void ProcCompiler::preLight(ProcEntity& entity)
{
	/*int			i;
	int			start, end;
	mapLight_t	*light;*/

	// don't prelight anything but the world entity
	if (&entity != _procFile->entities[0].get())
	{
		return;
	}
	
	if (1 > 0 /*dmapGlobals.shadowOptLevel > 0*/) // FIXME: shadowopt level is 1 by default
	{
		globalOutputStream() << "----- BuildLightShadows -----" << std::endl;
		
		// calc bounds for all the groups to speed things up
		for (std::size_t i = 0; i < entity.numAreas; ++i)
		{
			ProcArea& area = entity.areas[i];

			for (ProcArea::OptimizeGroups::reverse_iterator group = area.groups.rbegin();
				 group != area.groups.rend(); ++group)
			{
				boundOptimizeGroup(*group);
			}
		}

		for (std::size_t i = 0; i < _procFile->lights.size(); ++i)
		{
			ProcLight& light = _procFile->lights[i];

			buildLightShadows(entity, light);
		}
	}


	/*if ( !dmapGlobals.noLightCarve ) {
		common->Printf( "----- CarveGroupsByLight -----\n" );
		start = Sys_Milliseconds();
		// now subdivide the optimize groups into additional groups for
		// each light that illuminates them
		for ( i = 0 ; i < dmapGlobals.mapLights.Num() ; i++ ) {
			light = dmapGlobals.mapLights[i];
			CarveGroupsByLight( e, light );
		}

		end = Sys_Milliseconds();
		common->Printf( "%5.1f seconds for CarveGroupsByLight\n", ( end - start ) / 1000.0 );
	}*/
}

bool ProcCompiler::processModel(ProcEntity& entity, bool floodFill)
{
	_bspFaces.clear();

	BspTreeNode::nextNodeId = 0;
	ProcPortal::nextPortalId = 0;

	// build a bsp tree using all of the sides
	// of all of the structural brushes
	makeStructuralProcFaceList(entity.primitives);

	// Sort all the faces into the tree
	faceBsp(entity);

	// create portals at every leaf intersection
	// to allow flood filling
	makeTreePortals(entity.tree);

	// classify the leafs as opaque or areaportal
	filterBrushesIntoTree(entity);

#if 0
	printBrushCount(entity.tree.head, 0);
#endif

	// see if the bsp is completely enclosed
	if (floodFill/* && !dmapGlobals.noFlood*/)	// TODO: noflood option
	{
		if (floodEntities(entity.tree))
		{
			// set the outside leafs to opaque
			fillOutside(entity);
		}
		else
		{
			globalOutputStream() <<	 "**********************" << std::endl;
			globalWarningStream() << "******* leaked *******" << std::endl;
			globalOutputStream() <<  "**********************" << std::endl;
			
			// Generate a new leakfile
			_procFile->leakFile.reset(new LeakFile(entity.tree));

			// bail out here.  If someone really wants to
			// process a map that leaks, they should use
			// -noFlood
			return false;
		}
	}

	// get minimum convex hulls for each visible side
	// this must be done before creating area portals,
	// because the visible hull is used as the portal
	clipSidesByTree(entity);

	// determine areas before clipping tris into the
	// tree, so tris will never cross area boundaries
	floodAreas(entity);

	// we now have a BSP tree with solid and non-solid leafs marked with areas
	// all primitives will now be clipped into this, throwing away
	// fragments in the solid areas
	putPrimitivesInAreas(entity);

	// now build shadow volumes for the lights and split
	// the optimize lists by the light beam trees
	// so there won't be unneeded overdraw in the static
	// case
	preLight(entity);

	/*// optimizing is a superset of fixing tjunctions
	if ( !dmapGlobals.noOptimize ) {
		OptimizeEntity( e );
	} else  if ( !dmapGlobals.noTJunc ) {
		FixEntityTjunctions( e );
	}

	// now fix t junctions across areas
	FixGlobalTjunctions( e );*/

	return true;
}

} // namespace
