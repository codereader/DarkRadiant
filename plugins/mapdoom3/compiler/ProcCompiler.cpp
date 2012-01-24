#include "ProcCompiler.h"

#include "itextstream.h"
#include "math/Plane3.h"
#include "ishaders.h"
#include <limits>
#include <boost/format.hpp>

namespace map
{

const std::size_t PLANENUM_LEAF = std::numeric_limits<std::size_t>::max();
const float CLIP_EPSILON = 0.1f;

ProcCompiler::ProcCompiler(const scene::INodePtr& root) :
	_root(root)
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
	bool boundBrush()
	{
		_buildBrush.bounds = AABB();

		for (std::size_t i = 0; i < _buildBrush.sides.size(); ++i)
		{
			const ProcWinding& winding = _buildBrush.sides[i].winding;

			for (std::size_t j = 0; j < winding.size(); ++j)
			{
				_buildBrush.bounds.includePoint(winding[j].vertex);
			}
		}

		Vector3 corner1 = _buildBrush.bounds.origin + _buildBrush.bounds.extents;

		if (corner1[0] < MIN_WORLD_COORD || corner1[1] < MIN_WORLD_COORD || corner1[2] < MIN_WORLD_COORD)
		{
			return false;
		}

		Vector3 corner2 = _buildBrush.bounds.origin - _buildBrush.bounds.extents;

		if (corner2[0] > MAX_WORLD_COORD || corner2[1] > MAX_WORLD_COORD || corner2[2] > MAX_WORLD_COORD)
		{
			return false;
		}

		return true;
	}

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

		return boundBrush();
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
		prim.brush->brushnum = _entityPrimitive;

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

			// TODO: DarkRadiant's Material doesn't deliver coverage yet, use material flags in the meantime
			//if (side.material->Coverage() != MC_OPAQUE )
			if (side.material->getMaterialFlags() & Material::FLAG_TRANSLUCENT)
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
			_procFile->entities.push_back(ProcEntity(entityNode));

			// Traverse this entity's primitives
			ToolPrimitiveGenerator primitiveGenerator(_procFile->entities.back(), _procFile);
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
		const ProcEntity& entity = *_procFile->entities.begin();

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
		ProcEntity& entity = _procFile->entities[i];

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
	for (ProcEntity::Primitives::const_iterator i = primitives.begin();
		i != primitives.end(); ++i)
	{
		if (!i->brush) continue; // skip all patches

		ProcBrush& brush = *i->brush;

		if (!brush.opaque && !(brush.contents & Material::SURF_AREAPORTAL))
		{
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
			//plane[0] = plane[1] = plane[2] = 0.0f;
			//plane[3] = -dist;

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
	BspFaces::const_iterator bestSplit = faces.begin();

	bool havePortals = false;

	for (BspFaces::const_iterator split = faces.begin(); split != faces.end(); ++split)
	{
		(*split)->checked = false;

		if ((*split)->portal)
		{
			havePortals = true;
		}
	}

	for (BspFaces::const_iterator split = faces.begin(); split != faces.end(); ++split)
	{
		if ((*split)->checked) continue;

		// greebo: prefer portals as split planes, if we have some
		if (havePortals != (*split)->portal) continue;

		const Plane3& mapPlane = _procFile->planes.getPlane((*split)->planenum);
		//mapPlane = &dmapGlobals.mapPlanes[ split->planenum ];

		int splits = 0;
		int facing = 0;
		int front = 0;
		int back = 0;

		for (BspFaces::const_iterator check = faces.begin(); check != faces.end(); ++check)
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

		int value =  5*facing - 5*splits; // - abs(front-back);

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

void ProcCompiler::buildFaceTreeRecursively(const BspTreeNodePtr& node, BspFaces& faces)
{
	std::size_t splitPlaneNum = selectSplitPlaneNum(node, faces);
	
	// if we don't have any more faces, this is a node
	if (splitPlaneNum == std::numeric_limits<std::size_t>::max())
	{
		node->planenum = PLANENUM_LEAF;
		_bspTree.numFaceLeafs++;
		return;
	}

	// partition the list
	node->planenum = splitPlaneNum;

	const Plane3& plane = _procFile->planes.getPlane(splitPlaneNum);
	//idPlane &plane = dmapGlobals.mapPlanes[ splitPlaneNum ];

	BspFaces childLists[2];

	BspFaces::iterator next;

	for (BspFaces::iterator split = faces.begin(); split != faces.end(); split = next )
	{
		next = split + 1; // remember the pointer to next

		if ((*split)->planenum == node->planenum)
		{
			split->reset();
			//FreeBspFace( split );
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

			split->reset(); //FreeBspFace( split );
		}
		else if (side == SIDE_FRONT)
		{
			childLists[0].push_back(*split);
			//split->next = childLists[0];
			//childLists[0] = split;
		}
		else if ( side == SIDE_BACK )
		{
			childLists[1].push_back(*split);
			//split->next = childLists[1];
			//childLists[1] = split;
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

			base = 0.5f * (node->children[0]->bounds.origin[i] - node->children[0]->bounds.extents[i]);

			node->children[1]->bounds.origin[i] = base + distHalf;
			node->children[1]->bounds.origin[i] = base - distHalf;
			break;
		}
	}

	for (std::size_t i = 0; i < 2; ++i)
	{
		buildFaceTreeRecursively(node->children[i], childLists[i]);
	}
}

void ProcCompiler::faceBsp(ProcEntity& entity)
{
	globalOutputStream() << "--- FaceBSP: " << _bspFaces.size() << " faces ---" << std::endl;

	_bspTree.bounds = AABB();

	// Accumulate bounds
	for (BspFaces::const_iterator f = _bspFaces.begin(); f != _bspFaces.end(); ++f)
	{
		for (std::size_t i = 0; i < (*f)->w.size(); ++i)
		{
			_bspTree.bounds.includePoint((*f)->w[i].vertex);
		}
	}

	// Allocate the head node and use the total bounds
	_bspTree.head.reset(new BspTreeNode);
	_bspTree.head->bounds = _bspTree.bounds;

	buildFaceTreeRecursively(_bspTree.head, _bspFaces);

	globalOutputStream() << (boost::format("%5i leafs") % _bspTree.numFaceLeafs).str() << std::endl;

	//common->Printf( "%5.1f seconds faceBsp\n", ( end - start ) / 1000.0 );
}

bool ProcCompiler::processModel(ProcEntity& entity, bool floodFill)
{
	_bspFaces.clear();

	// build a bsp tree using all of the sides
	// of all of the structural brushes
	makeStructuralProcFaceList(entity.primitives);

	// Sort all the faces into the tree
	faceBsp(entity);

	/*e->tree = FaceBSP( faces );

	// create portals at every leaf intersection
	// to allow flood filling
	MakeTreePortals( e->tree );

	// classify the leafs as opaque or areaportal
	FilterBrushesIntoTree( e );

	// see if the bsp is completely enclosed
	if ( floodFill && !dmapGlobals.noFlood ) {
		if ( FloodEntities( e->tree ) ) {
			// set the outside leafs to opaque
			FillOutside( e );
		} else {
			common->Printf ( "**********************\n" );
			common->Warning( "******* leaked *******" );
			common->Printf ( "**********************\n" );
			LeakFile( e->tree );
			// bail out here.  If someone really wants to
			// process a map that leaks, they should use
			// -noFlood
			return false;
		}
	}

	// get minimum convex hulls for each visible side
	// this must be done before creating area portals,
	// because the visible hull is used as the portal
	ClipSidesByTree( e );

	// determine areas before clipping tris into the
	// tree, so tris will never cross area boundaries
	FloodAreas( e );

	// we now have a BSP tree with solid and non-solid leafs marked with areas
	// all primitives will now be clipped into this, throwing away
	// fragments in the solid areas
	PutPrimitivesInAreas( e );

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
