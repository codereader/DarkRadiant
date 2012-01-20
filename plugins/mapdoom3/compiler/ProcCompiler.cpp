#include "ProcCompiler.h"

#include "itextstream.h"
#include "math/Plane3.h"
#include "ishaders.h"
#include <boost/format.hpp>

namespace map
{

ProcCompiler::ProcCompiler(const scene::INodePtr& root) :
	_root(root)
{}

ProcFilePtr ProcCompiler::generateProcFile()
{
	_procFile.reset(new ProcFile);

	// Load all entities into proc entities
	generateBrushData();

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
				_buildBrush.sides.push_back(BspFace());
				BspFace& side = _buildBrush.sides.back();

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
					
					tris[triIdx].v[0].vertex = v1.vertex;
					tris[triIdx].v[1].vertex = v2.vertex;
					tris[triIdx].v[2].vertex = v3.vertex;
					tris[triIdx].material = material;

					tris[triIdx+1].v[0].vertex = v4.vertex;
					tris[triIdx+1].v[1].vertex = v3.vertex;
					tris[triIdx+1].v[2].vertex = v2.vertex;
					tris[triIdx+1].material = material;
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
			BspFace& side = _buildBrush.sides[i];

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

		const BspFace& firstSide = _buildBrush.sides[0];
		int contents = firstSide.material->getSurfaceFlags();

		_buildBrush.contentShader = firstSide.material;
		
		bool mixed = false;

		// a brush is only opaque if all sides are opaque
		_buildBrush.opaque = true;

		for (std::size_t i = 1; i < _buildBrush.sides.size(); i++)
		{
			const BspFace& side = _buildBrush.sides[i];

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
			
			return false; // processed => stop traversal here
		}

		return true;
	}
};

}

void ProcCompiler::generateBrushData()
{
	ToolDataGenerator generator(_procFile);
	_root->traverse(generator);
}

} // namespace
