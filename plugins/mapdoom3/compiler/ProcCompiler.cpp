#include "ProcCompiler.h"

#include "itextstream.h"
#include "math/Plane3.h"
#include "ishaders.h"
#include <limits>
#include <boost/format.hpp>

namespace map
{

std::size_t	BspTreeNode::nextNodeId = 0;
std::size_t ProcPortal::nextPortalId = 0;

const float CLIP_EPSILON = 0.1f;
const float SPLIT_WINDING_EPSILON = 0.001f;

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

				// FIXME: Required?
				//ms->GetTextureVectors( s->texVec.v );
				// remove any integral shift, which will help with grouping
				//s->texVec.v[0][3] -= floor( s->texVec.v[0][3] );
				//s->texVec.v[1][3] -= floor( s->texVec.v[1][3] );
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

#if 0 // FIXME: Implement "discrete" surface flag
			// set merge groups if needed, to prevent multiple sides from being
			// merged into a single surface in the case of gui shaders, mirrors, and autosprites
			if ( material->IsDiscrete() ) {
				for ( tri = prim->tris ; tri ; tri = tri->next ) {
					tri->mergeGroup = (void *)patch;
				}
			}
#endif

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

			base = 0.5f * (node->children[1]->bounds.origin[i] - node->children[1]->bounds.extents[i]);

			node->children[1]->bounds.origin[i] = base + distHalf;
			node->children[1]->bounds.extents[i] = base - distHalf;
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
	// TODO tree.outside.brushlist = NULL;
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
	/*CheckAreas_r( e->tree->headnode );

	// identify all portals between areas if this is the world
	if ( e == &dmapGlobals.uEntities[0] ) {
		numInterAreaPortals = 0;
		FindInterAreaPortals_r( e->tree->headnode );
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
	/*PutPrimitivesInAreas( e );

	// now build shadow volumes for the lights and split
	// the optimize lists by the light beam trees
	// so there won't be unneeded overdraw in the static
	// case
	Prelight( e );

	// optimizing is a superset of fixing tjunctions
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
