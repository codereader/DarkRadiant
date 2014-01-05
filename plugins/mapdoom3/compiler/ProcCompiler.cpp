#include "ProcCompiler.h"

#include "itextstream.h"
#include "math/Plane3.h"
#include "string/convert.h"
#include "ishaders.h"
#include "imodelcache.h"
#include "imodelsurface.h"
#include <limits>
#include <boost/format.hpp>
#include "OptIsland.h"
#include "OptUtils.h"
#include "ProcPatch.h"
#include <stdexcept>

namespace map
{

std::size_t BspTreeNode::nextNodeId = 0;
std::size_t ProcPortal::nextPortalId = 0;

const float CLIP_EPSILON = 0.1f;
const float SPLIT_WINDING_EPSILON = 0.001f;

static std::size_t DEFAULT_OPT_EDGES = 0x40000;
static std::size_t DEFAULT_OPT_VERTICES = 0x10000;

const std::size_t MULTIAREA_CROSS = std::numeric_limits<std::size_t>::max();
const std::size_t AREANUM_DIFFERENT = std::numeric_limits<std::size_t>::max();

static const float LIGHT_CLIP_EPSILON = 0.1f;

#define MAX_SHADOW_INDEXES      0x18000
#define MAX_SHADOW_VERTS        0x18000

// a point that is on the plane is NOT culled
#define POINT_CULLED(p1) ( ( pointCull[p1] & 0xfc0 ) != 0xfc0 )
#define TRIANGLE_CLIPPED(p1,p2,p3) ( ( ( pointCull[p1] & pointCull[p2] & pointCull[p3] ) & 0xfc0 ) != 0xfc0 )

// an edge that is on the plane is NOT culled
#define EDGE_CULLED(p1,p2) ( ( pointCull[p1] ^ 0xfc0 ) & ( pointCull[p2] ^ 0xfc0 ) & 0xfc0 )
#define EDGE_CLIPPED(p1,p2) ( ( pointCull[p1] & pointCull[p2] & 0xfc0 ) != 0xfc0 )

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
    _numAreaFloods(0),
    _overflowed(false),
    _shadowVerts(MAX_SHADOW_VERTS),
    _shadowIndices(MAX_SHADOW_INDEXES)
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
            //  return;
            //}

            _procFile->numPatches++;

            MaterialPtr material = GlobalMaterialManager().getMaterialForName(patch->getShader());

            _entity.primitives.push_back(ProcPrimitive());
            ProcTris& tris = _entity.primitives.back().patch;

            ProcPatch surface(*patch);

            if (patch->subdivionsFixed())
            {
                surface.subdivideExplicit(patch->getSubdivisions(), true);
            }
            else
            {
                surface.subdivide(true);
            }

            for (std::size_t i = 0; i < surface.getNumIndices(); i += 3)
            {
                tris.push_front(ProcTri());
                ProcTri& tri = tris.front();

                tri.v[2] = surface.getVertex(surface.getIndex(i+0));
                tri.v[1] = surface.getVertex(surface.getIndex(i+2));
                tri.v[0] = surface.getVertex(surface.getIndex(i+1));

                tri.material = material;
            }

            /*rMessage() << "Parsed surface from prim #" << (_entityPrimitive-1) << ": " << _entity.primitives.back().patch.size() << " tris, "
                << material->getName() << std::endl;*/

#if 0
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
#endif

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
                    continue;       // back side clipaway
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
                rWarning() << 
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
                rWarning() << 
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
                    rWarning() << 
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
                    rWarning() << 
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
            std::size_t entityNum = _procFile->entities.size();
            _procFile->entities.push_back(ProcEntityPtr(new ProcEntity(entityNode, entityNum)));

            // Traverse this entity's primitives
            ToolPrimitiveGenerator primitiveGenerator(*_procFile->entities.back(), _procFile);
            node->traverseChildren(primitiveGenerator);

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
            rError() <<
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
    _root->traverseChildren(generator);

    generator.buildStats();

    rMessage() << (boost::format("%5i total world brushes") % _procFile->numWorldBrushes).str() << std::endl;
    rMessage() << (boost::format("%5i total world triSurfs") % _procFile->numWorldTriSurfs).str() << std::endl;
    rMessage() << (boost::format("%5i patches") % _procFile->numPatches).str() << std::endl;
    rMessage() << (boost::format("%5i entities") % _procFile->entities.size()).str() << std::endl;
    rMessage() << (boost::format("%5i planes") % _procFile->planes.size()).str() << std::endl;
    rMessage() << (boost::format("%5i areaportals") % _procFile->numPortals).str() << std::endl;

    Vector3 minBounds = _procFile->mapBounds.origin - _procFile->mapBounds.extents;
    Vector3 maxBounds = _procFile->mapBounds.origin + _procFile->mapBounds.extents;

    rMessage() << (boost::format("size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f") % 
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

        rMessage() << "############### entity " << i << " ###############" << std::endl;

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

#define BLOCK_SIZE  1024

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
                (*check)->checked = true;   // won't need to test this plane again
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
            value += 5;     // axial is better
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
    rMessage() << "--- FaceBSP: " << _bspFaces.size() << " faces ---" << std::endl;

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

    rMessage() << (boost::format("%5i leafs") % entity.tree.numFaceLeafs).str() << std::endl;

    //common->Printf( "%5.1f seconds faceBsp\n", ( end - start ) / 1000.0 );

    /*rMessage() << ("After Face BSP\n");

    std::size_t planes = _procFile->planes.size();

    for (std::size_t i = 0; i < planes; ++i)
    {
        const Plane3& plane = _procFile->planes.getPlane(i);
        rMessage() << (boost::format("Plane %d: (%f %f %f %f)\n") % i % plane.normal().x() % plane.normal().y() % plane.normal().z() % plane.dist());
    }*/
}

void ProcCompiler::addPortalToNodes(const ProcPortalPtr& portal, const BspTreeNodePtr& front, const BspTreeNodePtr& back)
{
    if (portal->nodes[0] || portal->nodes[1])
    {
        rError() << "AddPortalToNode: already included" << std::endl;
        return;
    }

    //rMessage() << (boost::format("Adding portal %d to node front = %d and back = %d") % portal->portalId % front->nodeId % back->nodeId) << std::endl;

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

    //rMessage() << (boost::format("Removing portal %d from node %d") % portal->portalId % node->nodeId) << std::endl;
    
    // remove reference to the current portal
    while (true)
    {
        ProcPortalPtr& test = *portalRef;

        if (!test)
        {
            rError() << "RemovePortalFromNode: portal not bounding leaf" << std::endl;
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
            rError() << "removePortalFromNode: portal not in leaf" << std::endl; 
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
        rError() << "removePortalFromNode: mislinked" << std::endl;
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
            rError() << "makeNodePortal: mislinked portal" << std::endl;
            side = 0;   // quiet a compiler warning
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

    //rMessage() << "-- Split node portals on node " << node->nodeId << std::endl;

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
            rError() << "splitNodePortals: mislinked portal" << std::endl;
            side = 0;   // quiet a compiler warning
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
            //rMessage() << " Discarding tiny portal " << portal->portalId << std::endl;

            frontwinding.clear();
            _numTinyPortals++;
        }

        if (!backwinding.empty() && backwinding.isTiny())
        {
            //rMessage() << " Discarding portal " << portal->portalId << std::endl;

            backwinding.clear();
            _numTinyPortals++;
        }

        if (frontwinding.empty() && backwinding.empty())
        {   
            continue; // tiny windings on both sides
        }

        if (frontwinding.empty())
        {
            //rMessage() << " No front winding " << portal->portalId << std::endl;

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
            //rMessage() << " No back winding " << portal->portalId << std::endl;

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

        //rMessage() << " Splitting portal " << portal->portalId << std::endl;
        
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
        rWarning() << "node without a volume" << std::endl;
    }

    Vector3 boundsMin = node->bounds.origin - node->bounds.extents;
    Vector3 boundsMax = node->bounds.origin + node->bounds.extents;

    for (std::size_t i = 0; i < 3; ++i)
    {
        if (boundsMin[i] < MIN_WORLD_COORD || boundsMax[i] > MAX_WORLD_COORD )
        {
            rWarning() << "node with unbounded volume" << std::endl;
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
    rMessage() << "----- MakeTreePortals -----" << std::endl;

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
    {   // only on back
        back.reset(new ProcBrush(*brush)); // copy
        return;
    }

    if (d_back > -0.1) // PLANESIDE_EPSILON)
    {   // only on front
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
        rWarning() << "huge winding" << std::endl;
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
            rMessage() << "split removed brush" << std::endl;
        }
        else
        {
            rMessage() << "split not on both sides" << std::endl;
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
    rMessage() << "----- FilterBrushesIntoTree -----" << std::endl;

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

    rMessage() << (boost::format("%5i total brushes") % _numUniqueBrushes).str() << std::endl;
    rMessage() << (boost::format("%5i cluster references") % _numClusters).str() << std::endl;
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
        rv += string::to_string(p->portalId);

        rv += (s == 0) ? " (f)" : " (b)";
    }

    rv += " [count: " + string::to_string(count) + "]";

    return rv;
}

inline void printBrushCount(const BspTreeNodePtr& node, std::size_t level)
{
    for (std::size_t i = 0; i < level; ++i) rMessage() << " ";
    rMessage() << level << ": node " << node->nodeId << " has " << node->brushlist.size() << " brushes." << std::endl;

    for (std::size_t i = 0; i < level; ++i) rMessage() << " ";
    rMessage() << level << ": node " << node->nodeId << " has portals: " << printPortals(node) << std::endl;

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

    rMessage() << "--- FloodEntities ---" << std::endl;

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

        Vector3 origin = string::convert<Vector3>(originStr);

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
            rError() << "Leak on entity #" << i << std::endl;
            rError() << "Entity classname was " << mapEnt.getKeyValue("classname") << std::endl;
            rError() << "Entity name was " << mapEnt.getKeyValue("name") << std::endl;
            rError() << "Entity origin is " 
                                << string::convert<Vector3>(mapEnt.getKeyValue("origin"))
                                << std::endl;
        }
    }

    rMessage() << (boost::format("%5i flooded leafs") % _numFloodedLeafs).str() << std::endl;

    if (!inside)
    {
        rMessage() << "no entities in open -- no filling" << std::endl;
    }
    else if (tree.outside->occupied > 0)
    {
        rMessage() << "entity reached from outside -- no filling" << std::endl;
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

    rMessage() << "--- FillOutside ---" << std::endl;

    fillOutsideRecursively(entity.tree.head);

    rMessage() << (boost::format("%5i solid leafs") % _numSolidLeafs).str() << std::endl;
    rMessage() << (boost::format("%5i leafs filled") % _numOutsideLeafs).str() << std::endl;
    rMessage() << (boost::format("%5i inside leafs") % _numInsideLeafs).str() << std::endl;
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
    rMessage() << "----- ClipSidesByTree -----" << std::endl;

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
        return false;   // to global outsideleaf
    }

    if (portal.nodes[0]->planenum != PLANENUM_LEAF || portal.nodes[1]->planenum != PLANENUM_LEAF)
    {
        rError() << "ProcCompiler::portalIsPassable: not a leaf" << std::endl;
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

                    rWarning() << "brush has multiple area portal sides at " << center << std::endl;

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
        return;     // already got it
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
        return;     // already got it
    }

    _numAreaFloods = 0;
    floodAreasRecursively(node);

    rMessage() << (boost::format("Area %i has %i leafs") % _numAreas % _numAreaFloods) << std::endl;

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
        rError() << "ProcCompiler::checkAreasRecursively: area = %i" << node->area << std::endl;
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
            rWarning() << "findSideForPortal failed at " << p->winding.getCenter() << std::endl;
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
            continue;   // already emitted
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
    rMessage() << "--- FloodAreas ---" << std::endl;

    // set all areas to -1
    clearAreasRecursively(entity.tree.head);

    // flood fill from non-opaque areas
    _numAreas = 0;

    findAreasRecursively(entity.tree.head);

    rMessage() << (boost::format("%5i areas") % _numAreas) << std::endl;
    entity.numAreas = _numAreas;

    // make sure we got all of them
    checkAreasRecursively(entity.tree.head);

    // identify all portals between areas if this is the world
    if (&entity == _procFile->entities.begin()->get())
    {
        _procFile->interAreaPortals.clear();
        findInterAreaPortalsRecursively(entity.tree.head);

        rMessage() << (boost::format("%5i interarea portals") % _procFile->interAreaPortals.size()) << std::endl;
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
        triList.push_front(ProcTri());
        ProcTri& tri = triList.front();

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
                rError() << "Bad normal in TriListForSide" << std::endl;
                return triList;
            }
        }
    }
#else
    // triangle fan from central point, more verts and tris, but less distended
    // I use this when debugging some tjunction problems
    triList = NULL;
    for ( i = 0 ; i < w->GetNumPoints() ; i++ ) {
        idVec3  midPoint;

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

        std::size_t a1 = front.empty() ? 0 : checkWindingInAreasRecursively(front, node->children[0]);
        std::size_t a2 = back.empty() ? 0 : checkWindingInAreasRecursively(back, node->children[1]);
        
        if (a1 == MULTIAREA_CROSS || a2 == MULTIAREA_CROSS)
        {
            return MULTIAREA_CROSS; // different
        }

        if (a1 == -1)
        {
            return a2;  // one solid
        }

        if (a2 == -1)
        {
            return a1;  // one solid
        }

        if (a1 != a2)
        {
            return MULTIAREA_CROSS; // cross areas
        }

        return a1;
    }

    return node->area;
}

#define  TEXTURE_OFFSET_EQUAL_EPSILON   0.005f
#define  TEXTURE_VECTOR_EQUAL_EPSILON   0.001f

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
        if (group->material == triList.front().material && 
            group->planeNum == planeNum && 
            (group->mergeGroup == triList.front().mergeGroup || 
             group->mergeSurf == triList.front().mergeSurf || 
             group->mergePatch == triList.front().mergePatch))
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
                break;  // exact match
            }
            /*else
            {
                // different texture offsets
                i = 1;  // just for debugger breakpoint
            }*/
        }
    }

    if (group == area.groups.end())
    {
        //std::size_t oldSize = area.groups.size();

        area.groups.push_front(ProcOptimizeGroup());
        group = area.groups.begin();

        //rMessage() << (boost::format("Adding group from plane %d after %d\n") % planeNum % oldSize);

        group->numGroupLights = 0;
        group->smoothed = false;
        group->surfaceEmitted = false;

        group->planeNum = planeNum;
        group->mergeGroup = triList.front().mergeGroup;
        group->mergeSurf = triList.front().mergeSurf;
        group->mergePatch = triList.front().mergePatch;
        group->material = triList.front().material;
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

    Vector3 temp(
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
        return;     // original was degenerate, so it doesn't matter
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
        triList.push_front(originalTri);

        ProcTri& tri = triList.front();

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

        Plane3 plane(originalTri.v[1].vertex, originalTri.v[0].vertex, originalTri.v[2].vertex); // Plane(p1, p0, p2) call convention to match D3

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

            Plane3 plane(tri->v[1].vertex, tri->v[0].vertex, tri->v[2].vertex); // Plane(p1, p0, p2) call convention to match D3

            std::size_t planeNum = _procFile->planes.findOrInsertPlane(plane, EPSILON_NORMAL, EPSILON_DIST);

            Vector4 texVec[2];
            getTexVecForTri(texVec, newTri.front());

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
    rMessage() << "----- PutPrimitivesInAreas -----" << std::endl;

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
            //rMessage() << "--- Primitive: " << primCount << " (patch) " << std::endl;

            // add curve triangles
            addMapTrisToAreas(prim->patch, entity);

            /*for (std::size_t i = 0; i < _procFile->planes.size(); ++i)
            {
                const Plane3& plane = _procFile->planes.getPlane(i);
                rMessage() << (boost::format("Plane %d: %f %f %f %f") % i % plane.normal().x() % plane.normal().y() % plane.normal().z() % plane.dist()) << std::endl;
            }*/
            
            /*// Print result
            for (std::size_t a = 0; a < entity.areas.size(); ++a)
            {
                rMessage() << " Area " << a << ": ";
        
                rMessage() << entity.areas[a].groups.size() << " groups" << std::endl;

                for (ProcArea::OptimizeGroups::const_iterator g = entity.areas[a].groups.begin(); 
                    g != entity.areas[a].groups.end(); ++g)
                {
                    rMessage() << "  plane " << g->planeNum << ", " <<
                        g->triList.size() << " tris" << std::endl;
                }
            }*/

            continue;
        }

        //rMessage() << "--- Primitive: " << primCount << " (brush " << brush->brushnum << ") " << std::endl;

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
            rMessage() << " Area " << a << ": ";
        
            rMessage() << entity.areas[a].groups.size() << " groups" << std::endl;

            for (ProcArea::OptimizeGroups::const_iterator g = entity.areas[a].groups.begin(); 
                g != entity.areas[a].groups.end(); ++g)
            {
                rMessage() << "  plane " << g->planeNum << ", " <<
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
                rWarning() << "Cannot inline entity " << mapEnt.getKeyValue("name") <<
                    " since the model cannot be loaded: " << modelName << std::endl;
                continue;
            }

            rMessage() << "inlining " << mapEnt.getKeyValue("name") << std::endl;
            
            // get the rotation matrix in either full form, or single angle form
            std::string rotation = mapEnt.getKeyValue("rotation");

            Matrix4 axis;

            if (rotation.empty())
            {
                float angle = string::convert<float>(mapEnt.getKeyValue("angle"));

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

            Vector3 origin = string::convert<Vector3>(mapEnt.getKeyValue("origin"));

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

    /*rMessage() << ("After Put Primitives in Areas\n");

    std::size_t planes = _procFile->planes.size();

    for (std::size_t i = 0; i < planes; ++i)
    {
        const Plane3& plane = _procFile->planes.getPlane(i);
        rMessage() << (boost::format("Plane %d: (%f %f %f %f)\n") % i % plane.normal().x() % plane.normal().y() % plane.normal().z() % plane.dist());
    }

    rMessage() << (boost::format("PPIA: Entity %s\n") % entity.mapEntity->getEntity().getKeyValue("name"));

    // Print result
    for (std::size_t a = 0; a < entity.areas.size(); ++a)
    {
        rMessage() << " Area " << a << ": ";
        
        rMessage() << entity.areas[a].groups.size() << " groups" << std::endl;

        std::size_t count = 0;
        for (ProcArea::OptimizeGroups::const_iterator g = entity.areas[a].groups.begin(); g != entity.areas[a].groups.end(); ++g, ++count)
        {
            rMessage() << (boost::format("  Group %d, plane %d, %d tris\n") % count % g->planeNum % g->triList.size());

            std::size_t triCount = 0;
            for (ProcTris::const_iterator t = g->triList.begin(); t != g->triList.end(); ++t, ++triCount)
            {
                rMessage() << (boost::format("  Tri %d: (%f %f %f) (%f %f %f) (%f %f %f)\n") % triCount % 
                    t->v[0].vertex.x() % t->v[0].vertex.y() % t->v[0].vertex.z() %
                    t->v[1].vertex.x() % t->v[1].vertex.y() % t->v[1].vertex.z() %
                    t->v[2].vertex.x() % t->v[2].vertex.y() % t->v[2].vertex.z());
            }
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
    _triangleHash->_hashBounds = AABB();
    _triangleHash->calculateBounds(groups);

    _triangleHash->spreadHashBounds();
    _triangleHash->hashTriangles(groups);
}

void ProcCompiler::fixAreaGroupsTjunctions(ProcArea::OptimizeGroups& groups)
{
    if (false/*dmapGlobals.noTJunc*/) return; // FIXME

    if (groups.empty()) return;

    if (true/*dmapGlobals.verbose*/) // FIXME
    {
        std::size_t startCount = countGroupListTris(groups);
        rMessage() << "----- FixAreaGroupsTjunctions -----" << std::endl;
        rMessage() << (boost::format("%6i triangles in") % startCount) << std::endl;
    }

    hashTriangles(groups);

    for (ProcArea::OptimizeGroups::iterator group = groups.begin();
         group != groups.end(); ++group)
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

    if (true/*dmapGlobals.verbose*/) // FIXME
    {
        std::size_t endCount = countGroupListTris(groups);
        rMessage() << (boost::format("%6i triangles out") % endCount) << std::endl;
    }
}

inline void calcNormalVectors(const Vector3& self, Vector3& left, Vector3& down)
{
    float d = self.x() * self.x() + self.y() * self.y();

    if (!d)
    {
        left[0] = 1;
        left[1] = 0;
        left[2] = 0;
    } 
    else 
    {
        d = 1 / sqrt(d);

        left[0] = -self.y() * d;
        left[1] = self.x() * d;
        left[2] = 0;
    }

    down = left.crossProduct(self);
}

OptVertex* ProcCompiler::findOptVertex(const ArbitraryMeshVertex& v, ProcOptimizeGroup& group)
{
    // deal with everything strictly as 2D
    float x = v.vertex.dot(group.axis[0]);
    float y = v.vertex.dot(group.axis[1]);

    // should we match based on the t-junction fixing hash verts?
    for (std::size_t i = 0; i < _optVerts.size(); ++i)
    {
        if (_optVerts[i].pv[0] == x && _optVerts[i].pv[1] == y)
        {
            return &_optVerts[i];
        }
    }

    // not found, insert a new one
    _optVerts.push_back(OptVertex());

    OptVertex* vert = &_optVerts.back(); // TODO: greebo: instead of OptVertex* we might as well use array indices?
    
    vert->v = v;
    vert->pv[0] = x;
    vert->pv[1] = y;
    vert->pv[2] = 0;

    _optBounds.includePoint(vert->pv);

    return vert;
}

namespace
{

bool vertexIsBetween(const OptVertex* p1, const OptVertex* v1, const OptVertex* v2)
{
    Vector3 d1 = p1->pv - v1->pv;
    Vector3 d2 = p1->pv - v2->pv;
    float d = d1.dot(d2);

    return (d < 0);
}

} // namespace

void ProcCompiler::addOriginalTriangle(OptVertex* v[3])
{
    // if this triangle is backwards (possible with epsilon issues)
    // ignore it completely
    if (!OptUtils::IsTriangleValid(v[0], v[1], v[2]))
    {
        rWarning() << "WARNING: backwards triangle in input!" << std::endl;
        return;
    }

    for (std::size_t i = 0; i < 3; ++i)
    {
        OptVertex* v1 = v[i];
        OptVertex* v2 = v[(i+1) % 3];

        if (v1 == v2)
        {
            // this probably shouldn't happen, because the
            // tri would be degenerate
            continue;
        }

        std::size_t j = 0;

        // see if there is an existing one
        for ( ; j < _originalEdges.size(); ++j)
        {
            if (_originalEdges[j].v1 == v1 && _originalEdges[j].v2 == v2)
            {
                break;
            }

            if (_originalEdges[j].v2 == v1 && _originalEdges[j].v1 == v2)
            {
                break;
            }
        }

        if (j == _originalEdges.size())
        {
            // add it
            _originalEdges.push_back(OriginalEdge(v1, v2));
        }
    }
}

void ProcCompiler::addOriginalEdges(ProcOptimizeGroup& group)
{
    if (false/* dmapGlobals.verbose */) // FIXME
    {
        rMessage() <<  "----" << std::endl;
        rMessage() << (boost::format("%6i original tris") % group.triList.size()) << std::endl;
    }

    _optBounds = AABB();

    // allocate space for max possible edges
    std::size_t numTris = group.triList.size();

    _originalEdges.clear();
    _originalEdges.reserve(numTris * 3);

    // add all unique triangle edges
    _optEdges.clear();
    _optEdges.reserve(DEFAULT_OPT_EDGES);

    _optVerts.clear();
    _optVerts.reserve(DEFAULT_OPT_VERTICES);

    OptVertex*  v[3];

    for (ProcTris::iterator tri = group.triList.begin(); tri != group.triList.end(); ++tri)
    {
        v[0] = tri->optVert[0] = findOptVertex(tri->v[0], group);
        v[1] = tri->optVert[1] = findOptVertex(tri->v[1], group);
        v[2] = tri->optVert[2] = findOptVertex(tri->v[2], group);

        addOriginalTriangle(v);
    }
}

OptVertex* ProcCompiler::getEdgeIntersection(const OptVertex* p1, const OptVertex* p2,
                                const OptVertex* l1, const OptVertex* l2, ProcOptimizeGroup& group)
{
    Vector3 dir1 = p1->pv - l1->pv;
    Vector3 dir2 = p1->pv - l2->pv;
    Vector3 cross1 = dir1.crossProduct(dir2);

    dir1 = p2->pv - l1->pv;
    dir2 = p2->pv - l2->pv;
    Vector3 cross2 = dir1.crossProduct(dir2);

    if (cross1[2] - cross2[2] == 0)
    {
        return NULL;
    }

    float f = cross1[2] / (cross1[2] - cross2[2]);

    ArbitraryMeshVertex v;

    v.vertex = p1->v.vertex * (1.0f - f) + p2->v.vertex * f;
    v.normal = p1->v.normal * (1.0f - f) + p2->v.normal * f;
    v.normal.normalise();
    v.texcoord[0] = p1->v.texcoord[0] * (1.0f - f) + p2->v.texcoord[0] * f;
    v.texcoord[1] = p1->v.texcoord[1] * (1.0f - f) + p2->v.texcoord[1] * f;

    return findOptVertex(v, group);
}

void ProcCompiler::addEdgeIfNotAlready(OptVertex* v1, OptVertex* v2)
{
    // make sure that there isn't an identical edge already added
    for (OptEdge* e = v1->edges; e ; )
    {
        if ((e->v1 == v1 && e->v2 == v2) || (e->v1 == v2 && e->v2 == v1))
        {
            return;     // already added
        }

        if (e->v1 == v1)
        {
            e = e->v1link;
        } 
        else if (e->v2 == v1)
        {
            e = e->v2link;
        } 
        else 
        {
            rError() << "addEdgeIfNotAlready: bad edge link" << std::endl;
            return;
        }
    }

    // this edge is a keeper
    _optEdges.push_back(OptEdge());

    OptEdge* newEdge = &_optEdges.back();
    newEdge->v1 = v1;
    newEdge->v2 = v2;

    newEdge->islandLink = NULL;

    // link the edge to its verts
    newEdge->linkToVertices();
}

void ProcCompiler::splitOriginalEdgesAtCrossings(ProcOptimizeGroup& group)
{
    std::size_t numOriginalVerts = _optVerts.size();

    // now split any crossing edges and create optEdges
    // linked to the vertexes

#if 0
    // debug drawing bounds
    dmapGlobals.drawBounds = optBounds;

    dmapGlobals.drawBounds[0][0] -= 2;
    dmapGlobals.drawBounds[0][1] -= 2;
    dmapGlobals.drawBounds[1][0] += 2;
    dmapGlobals.drawBounds[1][1] += 2;
#endif

    // generate crossing points between all the original edges
    EdgeCrossingsList crossings(_originalEdges.size());

    for (std::size_t i = 0; i < _originalEdges.size(); ++i)
    {
#if 0
        if ( dmapGlobals.drawflag ) {
            DrawOriginalEdges( numOriginalEdges, originalEdges );
            qglBegin( GL_LINES );
            qglColor3f( 0, 1, 0 );
            qglVertex3fv( originalEdges[i].v1->pv.ToFloatPtr() );
            qglColor3f( 0, 0, 1 );
            qglVertex3fv( originalEdges[i].v2->pv.ToFloatPtr() );
            qglEnd();
            qglFlush();
        }
#endif
        for (std::size_t j = i + 1; j < _originalEdges.size(); ++j)
        {
            OptVertex* v1 = _originalEdges[i].v1;
            OptVertex* v2 = _originalEdges[i].v2;
            OptVertex* v3 = _originalEdges[j].v1;
            OptVertex* v4 = _originalEdges[j].v2;

            if (!OptUtils::EdgesCross(v1, v2, v3, v4))
            {
                continue;
            }

            // this is the only point in optimization where
            // completely new points are created, and it only
            // happens if there is overlapping coplanar
            // geometry in the source triangles
            OptVertex* newVert = getEdgeIntersection(v1, v2, v3, v4, group);

            if (!newVert)
            {
                // colinear, so add both verts of each edge to opposite
                if (vertexIsBetween(v3, v1, v2)) 
                {
                    crossings[i].push_back(EdgeCrossing(v3));
                }

                if (vertexIsBetween(v4, v1, v2)) 
                {
                    crossings[i].push_back(EdgeCrossing(v4));
                }

                if (vertexIsBetween(v1, v3, v4)) 
                {
                    crossings[j].push_back(EdgeCrossing(v1));
                }

                if (vertexIsBetween(v2, v3, v4)) 
                {
                    crossings[j].push_back(EdgeCrossing(v2));
                }

                continue;
            }

            if (newVert != v1 && newVert != v2)
            {
                crossings[i].push_back(EdgeCrossing(newVert));
            }

            if (newVert != v3 && newVert != v4)
            {
                crossings[j].push_back(EdgeCrossing(newVert));
            }
        }
    }

    // now split each edge by its crossing points
    // colinear edges will have duplicated edges added, but it won't hurt anything
    for (std::size_t i = 0; i < _originalEdges.size(); ++i)
    {
        std::size_t numCross = crossings[i].size();
        numCross += 2;  // account for originals

        std::vector<OptVertex*> sorted(numCross);
        memset(&sorted[0], 0, sorted.size());

        sorted[0] = _originalEdges[i].v1;
        sorted[1] = _originalEdges[i].v2;

        std::size_t j = 2;

        for (EdgeCrossings::const_iterator cross = crossings[i].begin(); cross != crossings[i].end(); ++cross)
        {
            sorted[j] = cross->ov;
            j++;
        }

        // add all possible fragment combinations that aren't divided by another point
        for (std::size_t j = 0; j < numCross; ++j)
        {
            for (std::size_t k = j+1; k < numCross; ++k)
            {
                std::size_t l = 0;

                for (; l < numCross; ++l)
                {
                    if (sorted[l] == sorted[j] || sorted[l] == sorted[k])
                    {
                        continue;
                    }

                    if (sorted[j] == sorted[k])
                    {
                        continue;
                    }
                    
                    if (vertexIsBetween(sorted[l], sorted[j], sorted[k]))
                    {
                        break;
                    }
                }

                if (l == numCross)
                {
                    //common->Printf( "line %i fragment from point %i to %i\n", i, sorted[j] - optVerts, sorted[k] - optVerts );
                    addEdgeIfNotAlready(sorted[j], sorted[k]);
                }
            }
        }
    }


    crossings.clear();
    _originalEdges.clear();

    // check for duplicated edges
    for (std::size_t i = 0 ; i < _optEdges.size(); ++i)
    {
        for (std::size_t j = i + 1; j < _optEdges.size(); ++j)
        {
            if ((_optEdges[i].v1 == _optEdges[j].v1 && _optEdges[i].v2 == _optEdges[j].v2) ||
                (_optEdges[i].v1 == _optEdges[j].v2 && _optEdges[i].v2 == _optEdges[j].v1))
            {
                rMessage() << "duplicated optEdge" << std::endl;
            }
        }
    }

    if (false/* dmapGlobals.verbose*/)
    {
        rMessage() << (boost::format("%6i original edges") % _originalEdges.size()) << std::endl;
        rMessage() << (boost::format("%6i edges after splits") % _optEdges.size()) << std::endl;
        rMessage() << (boost::format("%6i original vertexes") % numOriginalVerts) << std::endl;
        rMessage() << (boost::format("%6i vertexes after splits") % _optVerts.size()) << std::endl;
    }
}

void ProcCompiler::dontSeparateIslands(ProcOptimizeGroup& group)
{
    OptIsland island(group, _optVerts, _optEdges, _procFile);

    island.optimise();
}

void ProcCompiler::optimizeOptList(ProcOptimizeGroup& group)
{
    ProcArea::OptimizeGroups tempList(1, group);

    // fix the t junctions among this single list
    // so we can match edges
    // can we avoid doing this if colinear vertexes break edges?
    fixAreaGroupsTjunctions(tempList);
    group = tempList.front();
    
    // create the 2D vectors
    calcNormalVectors(_procFile->planes.getPlane(group.planeNum).normal(), group.axis[0], group.axis[1]);

    addOriginalEdges(group);
    splitOriginalEdgesAtCrossings(group);

#if 0
    // seperate any discontinuous areas for individual optimization
    // to reduce the scope of the problem
    SeparateIslands( opt );
#else
    dontSeparateIslands(group);
#endif

    // now free the hash verts
    _triangleHash.reset();

    // free the original list and use the new one
    group.triList.swap(group.regeneratedTris);
    group.regeneratedTris.clear();
}

void ProcCompiler::setGroupTriPlaneNums(ProcArea::OptimizeGroups& groupList)
{
    for (ProcArea::OptimizeGroups::iterator group = groupList.begin(); 
         group != groupList.end(); ++group)
    {
        for (ProcTris::iterator tri = group->triList.begin(); tri != group->triList.end(); ++tri)
        {
            tri->planeNum = group->planeNum;
        }
    }
}

void ProcCompiler::optimizeGroupList(ProcArea::OptimizeGroups& groupList)
{
    if (groupList.empty()) return;

    std::size_t numIn = countGroupListTris(groupList);

    // optimize and remove colinear edges, which will
    // re-introduce some t junctions
    for (ProcArea::OptimizeGroups::iterator group = groupList.begin(); 
         group != groupList.end(); ++group)
    {
        optimizeOptList(*group);
    }

    std::size_t numEdge = countGroupListTris(groupList);

    // fix t junctions again
    fixAreaGroupsTjunctions(groupList);
    _triangleHash.reset();

    std::size_t numTjunc2 = countGroupListTris(groupList);

    setGroupTriPlaneNums(groupList);

    rMessage() << "----- OptimizeAreaGroups Results -----" << std::endl;
    rMessage() << (boost::format("%6i tris in") % numIn) << std::endl;
    rMessage() << (boost::format("%6i tris after edge removal optimization") % numEdge) << std::endl;
    rMessage() << (boost::format("%6i tris after final t junction fixing") % numTjunc2) << std::endl;
}

Surface ProcCompiler::shareMapTriVerts(const ProcTris& tris)
{
    // unique the vertexes
    std::size_t count = tris.size();

    Surface uTri;

    uTri.vertices.reserve(count * 3);
    uTri.indices.reserve(count * 3);

    for (ProcTris::const_iterator step = tris.begin(); step != tris.end(); ++step)
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            const ArbitraryMeshVertex& dv = step->v[i];

            // search for a match
            std::size_t j = 0;

            for (j = 0; j < uTri.vertices.size(); ++j)
            {
                if (OptUtils::MatchVert(uTri.vertices[j], dv))
                {
                    break;
                }
            }

            if (j == uTri.vertices.size())
            {
                uTri.vertices.push_back(dv);
            }

            uTri.indices.push_back(static_cast<int>(j));
        }
    }

    return uTri;
}

Surface ProcCompiler::createVertexProgramTurboShadowVolume(const Matrix4& transform, const Surface& tri, 
        const ProcLight& light, Surface::CullInfo& cullInfo)
{
    throw std::runtime_error("createVertexProgramTurboShadowVolume not implemented yet.");
}

Surface ProcCompiler::createTurboShadowVolume(const Matrix4& transform, const Surface& tri, 
        const ProcLight& light, Surface::CullInfo& cullInfo)
{
    throw std::runtime_error("createTurboShadowVolume not implemented yet.");
}

namespace
{

inline Vector3 globalPointToLocal(const Matrix4& transform, const Vector3& in)
{
    Vector3 temp = in - transform.translation();
    //VectorSubtract( in, &modelMatrix[12], temp );

    Vector3 out(
        temp.dot(transform.x().getVector3()),   //DotProduct( temp, &modelMatrix[0] );
        temp.dot(transform.y().getVector3()),
        temp.dot(transform.z().getVector3())
    );

    return temp;
}

inline float planeDistanceToBounds(const AABB& bounds, const Plane3& plane)
{
    Vector3 mins = bounds.origin - bounds.extents;
    Vector3 maxs = bounds.origin + bounds.extents;

    float d1 = plane.distanceToPoint(bounds.origin);
    float d2 = fabs((maxs[0] - bounds.origin[0]) * plane.normal()[0]) +
               fabs((maxs[1] - bounds.origin[1]) * plane.normal()[1]) +
               fabs((maxs[2] - bounds.origin[2]) * plane.normal()[2]);

    if (d1 - d2 > 0.0f)
    {
        return d1 - d2;
    }

    if (d1 + d2 < 0.0f)
    {
        return d1 + d2;
    }

    return 0.0f;
}

}

void ProcCompiler::calcInteractionFacing(const Matrix4& transform, const Surface& tri, const ProcLight& light,
                             Surface::CullInfo& cullInfo)
{
    if (!cullInfo.facing.empty())
    {
        return;
    }

    Vector3 localLightOrigin = globalPointToLocal(transform, light.getGlobalLightOrigin());

    std::size_t numFaces = tri.indices.size() / 3;

    if (tri.facePlanes.empty() || !tri.facePlanesCalculated)
    {
        const_cast<Surface&>(tri).deriveFacePlanes();
    }

    cullInfo.facing.resize(numFaces + 1);

    // calculate back face culling
    // exact geometric cull against face
    for (std::size_t i = 0; i < numFaces; ++i) 
    {
        float planeSide = localLightOrigin.dot(tri.facePlanes[i].normal()) - tri.facePlanes[i].dist();
        cullInfo.facing[i] = planeSide >= 0.0f;
    }

    cullInfo.facing[numFaces] = 1;  // for dangling edges to reference
}

void ProcCompiler::calcPointCull(const Surface& tri, const Plane3 frustum[6], unsigned short* pointCull, int* remap)
{
    memset(remap, -1, tri.vertices.size() * sizeof(remap[0]));

    int frontBits = 0;
    std::size_t i = 0;

    for (frontBits = 0, i = 0; i < 6; ++i)
    {
        // get front bits for the whole surface
        if (planeDistanceToBounds(tri.bounds, frustum[i]) >= LIGHT_CLIP_EPSILON)
        {
            frontBits |= 1 << (i + 6);
        }
    }

    // initialize point cull
    for (i = 0; i < tri.vertices.size(); ++i)
    {
        pointCull[i] = frontBits;
    }

    // if the surface is not completely inside the light frustum
    if (frontBits == ( ( ( 1 << 6 ) - 1 ) ) << 6)
    {
        return;
    }

    unsigned char* side1 = (unsigned char*)alloca(tri.vertices.size() * sizeof(unsigned char));
    unsigned char* side2 = (unsigned char*)alloca(tri.vertices.size() * sizeof(unsigned char));

    memset(side1, 0, tri.vertices.size() * sizeof(unsigned char));
    memset(side2, 0, tri.vertices.size() * sizeof(unsigned char));

    for (i = 0; i < 6; ++i)
    {
        if (frontBits & (1<<(i+6)))
        {
            continue;
        }

        for (std::size_t c = 0; c < tri.vertices.size(); ++c)
        {
            float planeSide = frustum[i].normal().dot(tri.vertices[c].vertex) - frustum[i].dist();
            side1[c] |= (planeSide < LIGHT_CLIP_EPSILON) << i;
            side2[c] |= (planeSide > -LIGHT_CLIP_EPSILON) << i;
        }
    }

    for (i = 0; i < tri.vertices.size(); ++i)
    {
        pointCull[i] |= side1[i] | (side2[i] << 6);
    }
}

int ProcCompiler::chopWinding(ClipTri clipTris[2], int inNum, const Plane3& plane)
{
    float   dists[MAX_CLIPPED_POINTS];
    int     sides[MAX_CLIPPED_POINTS];
        
    ClipTri& in = clipTris[inNum];
    ClipTri& out = clipTris[inNum^1];

    int counts[3] = { 0, 0, 0 };
    
    // determine sides for each point
    int i = 0;

    for (i = 0 ; i < in.numVerts; i++)
    {
        float dot = plane.distanceToPoint(in.verts[i]);
        dists[i] = dot;
        if (dot < -LIGHT_CLIP_EPSILON)
        {
            sides[i] = SIDE_BACK;
        }
        else if (dot > LIGHT_CLIP_EPSILON)
        {
            sides[i] = SIDE_FRONT;
        }
        else
        {
            sides[i] = SIDE_ON;
        }

        counts[sides[i]]++;
    }

    // if none in front, it is completely clipped away
    if (!counts[SIDE_FRONT])
    {
        in.numVerts = 0;
        return inNum;
    }

    if (!counts[SIDE_BACK])
    {
        return inNum;       // inout stays the same
    }

    // avoid wrapping checks by duplicating first value to end
    sides[i] = sides[0];
    dists[i] = dists[0];

    in.verts[in.numVerts] = in.verts[0];
    in.edgeFlags[in.numVerts] = in.edgeFlags[0];

    out.numVerts = 0;

    for (i = 0; i < in.numVerts; ++i)
    {
        Vector3& p1 = in.verts[i];

        if (sides[i] != SIDE_BACK)
        {
            out.verts[out.numVerts] = p1;

            if (sides[i] == SIDE_ON && sides[i+1] == SIDE_BACK)
            {
                out.edgeFlags[out.numVerts] = 1;
            } 
            else
            {
                out.edgeFlags[out.numVerts] = in.edgeFlags[i];
            }

            out.numVerts++;
        }

        if ((sides[i] == SIDE_FRONT && sides[i+1] == SIDE_BACK) || 
            (sides[i] == SIDE_BACK && sides[i+1] == SIDE_FRONT))
        {
            // generate a split point
            Vector3& p2 = in.verts[i+1];
            
            float dot = dists[i] / (dists[i]-dists[i+1]);

            Vector3 mid;

            for (int j = 0; j < 3; ++j)
            {
                mid[j] = p1[j] + dot*(p2[j] - p1[j]);
            }
                
            out.verts[out.numVerts] = mid;

            // set the edge flag
            if (sides[i+1] != SIDE_FRONT)
            {
                out.edgeFlags[out.numVerts] = 1;
            } 
            else 
            {
                out.edgeFlags[out.numVerts] = in.edgeFlags[i];
            }

            out.numVerts++;
        }
    }

    return inNum ^ 1;
}

bool ProcCompiler::clipTriangleToLight(const Vector3& a, const Vector3& b, const Vector3& c, int planeBits, const Plane3 frustum[6])
{
    ClipTri pingPong[2];

    pingPong[0].numVerts = 3;
    pingPong[0].edgeFlags[0] = 0;
    pingPong[0].edgeFlags[1] = 0;
    pingPong[0].edgeFlags[2] = 0;
    pingPong[0].verts[0] = a;
    pingPong[0].verts[1] = b;
    pingPong[0].verts[2] = c;

    int p = 0;

    for (int i = 0 ; i < 6 ; ++i)
    {
        if (planeBits & ( 1 << i ))
        {
            p = chopWinding(pingPong, p, frustum[i]);

            if (pingPong[p].numVerts < 1)
            {
                return false;
            }
        }
    }

    ClipTri& ct = pingPong[p];

    // copy the clipped points out to shadowVerts
    if (_numShadowVerts + ct.numVerts * 2 > MAX_SHADOW_VERTS)
    {
        _overflowed = true;
        return false;
    }

    int base = static_cast<int>(_numShadowVerts);

    for (std::size_t i = 0; i < ct.numVerts; ++i)
    {
        _shadowVerts[base + i*2].getVector3() = ct.verts[i];
    }
    _numShadowVerts += ct.numVerts * 2;

    if (_numShadowIndices + 3 * (ct.numVerts - 2) > MAX_SHADOW_INDEXES) 
    {
        _overflowed = true;
        return false;
    }

    for (int i = 2; i < ct.numVerts; i++)
    {
        _shadowIndices[_numShadowIndices++] = base + i * 2;
        _shadowIndices[_numShadowIndices++] = base + ( i - 1 ) * 2;
        _shadowIndices[_numShadowIndices++] = base;
    }

    // any edges that were created by the clipping process will
    // have a silhouette quad created for it, because it is one
    // of the exterior bounds of the shadow volume
    for (int i = 0; i < ct.numVerts; i++)
    {
        if (ct.edgeFlags[i])
        {
            if (_numClipSilEdges == MAX_CLIP_SIL_EDGES)
            {
                break;
            }

            _clipSilEdges[_numClipSilEdges][0] = base + i * 2;

            if (i == ct.numVerts - 1)
            {
                _clipSilEdges[_numClipSilEdges][1] = base;
            } 
            else 
            {
                _clipSilEdges[_numClipSilEdges][1] = base + (i + 1) * 2;
            }

            _numClipSilEdges++;
        }
    }

    return true;
}

namespace
{

/* 
To make sure the triangulations of the sil edges is consistant,
we need to be able to order two points.  We don't care about how
they compare with any other points, just that when the same two
points are passed in (in either order), they will always specify
the same one as leading.

Currently we need to have separate faces in different surfaces
order the same way, so we must look at the actual coordinates.
If surfaces are ever guaranteed to not have to edge match with
other surfaces, we could just compare indexes.
===============
*/
static bool pointsOrdered(const Vector3& a, const Vector3& b)
{
    // vectors that wind up getting an equal hash value will
    // potentially cause a misorder, which can show as a couple
    // crack pixels in a shadow

    // scale by some odd numbers so -8, 8, 8 will not be equal
    // to 8, -8, 8

    // in the very rare case that these might be equal, all that would
    // happen is an oportunity for a tiny rasterization shadow crack
    float i = a[0] + a[1]*127 + a[2]*1023;
    float j = b[0] + b[1]*127 + b[2]*1023;

    return i < j;
}

}

void ProcCompiler::addClipSilEdges()
{
    // don't allow it to overflow
    if (_numShadowIndices + _numClipSilEdges * 6 > MAX_SHADOW_INDEXES)
    {
        _overflowed = true;
        return;
    }

    for (std::size_t i = 0; i < _numClipSilEdges; i++)
    {
        int v1 = _clipSilEdges[i][0];
        int v2 = _clipSilEdges[i][1];
        int v1_back = v1 + 1;
        int v2_back = v2 + 1;

        if (pointsOrdered(_shadowVerts[v1].getVector3(), _shadowVerts[v2].getVector3()))
        {
            _shadowIndices[_numShadowIndices++] = v1;
            _shadowIndices[_numShadowIndices++] = v2;
            _shadowIndices[_numShadowIndices++] = v1_back;
            _shadowIndices[_numShadowIndices++] = v2;
            _shadowIndices[_numShadowIndices++] = v2_back;
            _shadowIndices[_numShadowIndices++] = v1_back;
        } 
        else
        {
            _shadowIndices[_numShadowIndices++] = v1;
            _shadowIndices[_numShadowIndices++] = v2;
            _shadowIndices[_numShadowIndices++] = v2_back;
            _shadowIndices[_numShadowIndices++] = v1;
            _shadowIndices[_numShadowIndices++] = v2_back;
            _shadowIndices[_numShadowIndices++] = v1_back;
        }
    }
}

bool ProcCompiler::clipLineToLight(const Vector3& a, const Vector3& b, const Plane3 frustum[4], Vector3& p1, Vector3& p2)
{
    p1 = a;
    p2 = b;

    // clip it
    for (int j = 0; j < 6 ; ++j)
    {
        float d1 = frustum[j].distanceToPoint(p1);
        float d2 = frustum[j].distanceToPoint(p2);

        // if both on or in front, not clipped to this plane
        if (d1 > -LIGHT_CLIP_EPSILON && d2 > -LIGHT_CLIP_EPSILON)
        {
            continue;
        }

        // if one is behind and the other isn't clearly in front, the edge is clipped off
        if (d1 <= -LIGHT_CLIP_EPSILON && d2 < LIGHT_CLIP_EPSILON)
        {
            return false;
        }

        if (d2 <= -LIGHT_CLIP_EPSILON && d1 < LIGHT_CLIP_EPSILON) 
        {
            return false;
        }

        // clip it, keeping the negative side
        Vector3& clip = (d1 < 0) ? p1 : p2;

#if 0
        if ( idMath::Fabs(d1 - d2) < 0.001 ) {
            d2 = d1 - 0.1;
        }
#endif

        float f = d1 / (d1 - d2);

        clip[0] = p1[0] + f * (p2[0] - p1[0]);
        clip[1] = p1[1] + f * (p2[1] - p1[1]);
        clip[2] = p1[2] + f * (p2[2] - p1[2]);
    }

    return true;    // retain a fragment
}

void ProcCompiler::addSilEdges(const Surface& tri, unsigned short* pointCull, const Plane3 frustum[6], 
    int* remap, unsigned char* faceCastsShadow)
{
    std::size_t numPlanes = tri.indices.size() / 3;

    // add sil edges for any true silhouette boundaries on the surface
    for (std::size_t i = 0; i < tri.silEdges.size(); ++i)
    {
        const Surface::SilEdge& sil = tri.silEdges[i];

        if (sil.p1 < 0 || sil.p1 > numPlanes || sil.p2 < 0 || sil.p2 > numPlanes)
        {
            rError() << "Bad sil planes" << std::endl;
            return;
        }

        // an edge will be a silhouette edge if the face on one side
        // casts a shadow, but the face on the other side doesn't.
        // "casts a shadow" means that it has some surface in the projection,
        // not just that it has the correct facing direction
        // This will cause edges that are exactly on the frustum plane
        // to be considered sil edges if the face inside casts a shadow.
        if (!(faceCastsShadow[sil.p1] ^ faceCastsShadow[sil.p2]))
        {
            continue;
        }

        // if the edge is completely off the negative side of
        // a frustum plane, don't add it at all.  This can still
        // happen even if the face is visible and casting a shadow
        // if it is partially clipped
        if (EDGE_CULLED(sil.v1, sil.v2))
        {
            continue;
        }

        std::size_t v1 = 0;
        std::size_t v2 = 0;

        // see if the edge needs to be clipped
        if (EDGE_CLIPPED(sil.v1, sil.v2))
        {
            if (_numShadowVerts + 4 > MAX_SHADOW_VERTS)
            {
                _overflowed = true;
                return;
            }

            v1 = _numShadowVerts;
            v2 = v1 + 2;

            if (!clipLineToLight(tri.vertices[sil.v1].vertex, tri.vertices[sil.v2].vertex, 
                frustum, _shadowVerts[v1].getVector3(), _shadowVerts[v2].getVector3()))
            {
                continue;   // clipped away
            }

            _numShadowVerts += 4;
        } 
        else 
        {
            // use the entire edge
            v1 = remap[sil.v1];
            v2 = remap[sil.v2];
            if ( v1 < 0 || v2 < 0 )
            {
                rError() << "addSilEdges: bad remap[]" << std::endl;
                return;
            }
        }

        // don't overflow
        if (_numShadowIndices + 6 > MAX_SHADOW_INDEXES)
        {
            _overflowed = true;
            return;
        }

        // we need to choose the correct way of triangulating the silhouette quad
        // consistantly between any two points, no matter which order they are specified.
        // If this wasn't done, slight rasterization cracks would show in the shadow
        // volume when two sil edges were exactly coincident
        if (faceCastsShadow[sil.p2])
        {
            if (pointsOrdered(_shadowVerts[v1].getVector3(), _shadowVerts[v2].getVector3()))
            {
                _shadowIndices[_numShadowIndices++] = v1;
                _shadowIndices[_numShadowIndices++] = v1+1;
                _shadowIndices[_numShadowIndices++] = v2;
                _shadowIndices[_numShadowIndices++] = v2;
                _shadowIndices[_numShadowIndices++] = v1+1;
                _shadowIndices[_numShadowIndices++] = v2+1;
            } 
            else
            {
                _shadowIndices[_numShadowIndices++] = v1;
                _shadowIndices[_numShadowIndices++] = v2+1;
                _shadowIndices[_numShadowIndices++] = v2;
                _shadowIndices[_numShadowIndices++] = v1;
                _shadowIndices[_numShadowIndices++] = v1+1;
                _shadowIndices[_numShadowIndices++] = v2+1;
            }
        }
        else
        { 
            if (pointsOrdered(_shadowVerts[v1].getVector3(), _shadowVerts[v2].getVector3()))
            {
                _shadowIndices[_numShadowIndices++] = v1;
                _shadowIndices[_numShadowIndices++] = v2;
                _shadowIndices[_numShadowIndices++] = v1+1;
                _shadowIndices[_numShadowIndices++] = v2;
                _shadowIndices[_numShadowIndices++] = v2+1;
                _shadowIndices[_numShadowIndices++] = v1+1;
            } 
            else
            {
                _shadowIndices[_numShadowIndices++] = v1;
                _shadowIndices[_numShadowIndices++] = v2;
                _shadowIndices[_numShadowIndices++] = v2+1;
                _shadowIndices[_numShadowIndices++] = v1;
                _shadowIndices[_numShadowIndices++] = v2+1;
                _shadowIndices[_numShadowIndices++] = v1+1;
            }
        }
    }
}

namespace
{

inline void getLightProjectionMatrix(const Vector3& origin, const Plane3& rearPlane, Vector4 mat[4])
{
    // calculate the homogeneous light vector
    Vector4 lv(origin, 1);

    float lg = Vector4(rearPlane.normal(), -rearPlane.dist()).dot(lv);

    // outer product
    mat[0][0] = lg - rearPlane.normal()[0] * lv[0];
    mat[0][1] = -rearPlane.normal()[1] * lv[0];
    mat[0][2] = -rearPlane.normal()[2] * lv[0];
    mat[0][3] = rearPlane.dist() * lv[0];

    mat[1][0] = -rearPlane.normal()[0] * lv[1];
    mat[1][1] = lg - rearPlane.normal()[1] * lv[1];
    mat[1][2] = -rearPlane.normal()[2] * lv[1];
    mat[1][3] = rearPlane.dist() * lv[1];

    mat[2][0] = -rearPlane.normal()[0] * lv[2];
    mat[2][1] = -rearPlane.normal()[1] * lv[2];
    mat[2][2] = lg - rearPlane.normal()[2] * lv[2];
    mat[2][3] = rearPlane.dist() * lv[2];

    mat[3][0] = -rearPlane.normal()[0] * lv[3];
    mat[3][1] = -rearPlane.normal()[1] * lv[3];
    mat[3][2] = -rearPlane.normal()[2] * lv[3];
    mat[3][3] = lg - (-rearPlane.dist() * lv[3]);
}

}

void ProcCompiler::projectPointsToFarPlane(const Matrix4& transform, const ProcLight& light, 
    const Plane3& lightPlaneLocal, std::size_t firstShadowVert, std::size_t numShadowVerts)
{
    Vector3 lv = transform.transformPoint(light.getGlobalLightOrigin());

    Vector4 mat[4];
    getLightProjectionMatrix(lv, lightPlaneLocal, mat);

    // make a projected copy of the even verts into the odd spots
    Vector4* in = &_shadowVerts[firstShadowVert];

    for (std::size_t i = firstShadowVert; i < numShadowVerts; i+= 2, in += 2)
    {
        in[0].w() = 1;

        float w = in->getVector3().dot(mat[3].getVector3()) + mat[3][3];
        
        if (w == 0)
        {
            in[1] = in[0];
            continue;
        }

        float oow = 1.0f / w;

        in[1].x() = (in->getVector3().dot(mat[0].getVector3()) + mat[0][3]) * oow;
        in[1].y() = (in->getVector3().dot(mat[1].getVector3()) + mat[1][3]) * oow;
        in[1].z() = (in->getVector3().dot(mat[2].getVector3()) + mat[2][3]) * oow;
        in[1].w() = 1;
    }
}

void ProcCompiler::createShadowVolumeInFrustum(const Matrix4& transform, const Surface& tri,
    const ProcLight& light, const Vector3& lightOrigin, const Plane3 frustum[6],
    const Plane3 &farPlane, bool makeClippedPlanes, int* remap, unsigned char* faceCastsShadow,
    std::vector<unsigned char>& globalFacing)
{
#if 0
    int     cullBits;
#endif

    unsigned short* pointCull = (unsigned short*)alloca(tri.vertices.size() * sizeof(unsigned short));

    // test the vertexes for inside the light frustum, which will allow
    // us to completely cull away some triangles from consideration.
    calcPointCull(tri, frustum, pointCull, remap);

    // this may not be the first frustum added to the volume
    std::size_t firstShadowIndex = _numShadowIndices;
    std::size_t firstShadowVert = _numShadowVerts;

    // decide which triangles front shadow volumes, clipping as needed
    _numClipSilEdges = 0;

    std::size_t numTris = tri.indices.size() / 3;

    for (std::size_t i = 0; i < numTris; ++i)
    {
        faceCastsShadow[i] = 0; // until shown otherwise

        // if it isn't facing the right way, don't add it
        // to the shadow volume
        if (globalFacing[i])
        {
            continue;
        }

        int i1 = tri.silIndexes[i*3 + 0];
        int i2 = tri.silIndexes[i*3 + 1];
        int i3 = tri.silIndexes[i*3 + 2];

        // if all the verts are off one side of the frustum,
        // don't add any of them
        if (pointCull[i1] & pointCull[i2] & pointCull[i3] & 0x3f)
        {
            continue;
        }

        // make sure the verts that are not on the negative sides
        // of the frustum are copied over.
        // we need to get the original verts even from clipped triangles
        // so the edges reference correctly, because an edge may be unclipped
        // even when a triangle is clipped.
        if (_numShadowVerts + 6 > MAX_SHADOW_VERTS)
        {
            _overflowed = true;
            return;
        }

        if (!POINT_CULLED(i1) && remap[i1] == -1)
        {
            remap[i1] = static_cast<int>(_numShadowVerts);
            _shadowVerts[_numShadowVerts].getVector3() = tri.vertices[i1].vertex;
            _numShadowVerts += 2;
        }

        if (!POINT_CULLED(i2) && remap[i2] == -1)
        {
            remap[i2] = static_cast<int>(_numShadowVerts);
            _shadowVerts[_numShadowVerts].getVector3() = tri.vertices[i2].vertex;
            _numShadowVerts += 2;
        }

        if (!POINT_CULLED(i3) && remap[i3] == -1)
        {
            remap[i3] = static_cast<int>(_numShadowVerts);
            _shadowVerts[_numShadowVerts].getVector3() = tri.vertices[i3].vertex;
            _numShadowVerts += 2;
        }

        // clip the triangle if any points are on the negative sides
        if ( TRIANGLE_CLIPPED( i1, i2, i3 ) )
        {
            int cullBits = ( ( pointCull[ i1 ] ^ 0xfc0 ) | ( pointCull[ i2 ] ^ 0xfc0 ) | ( pointCull[ i3 ] ^ 0xfc0 ) ) >> 6;

            // this will also define clip edges that will become silhouette planes
            if (clipTriangleToLight(tri.vertices[i1].vertex, tri.vertices[i2].vertex, tri.vertices[i3].vertex, cullBits, frustum))
            {
                faceCastsShadow[i] = 1;
            }
        } 
        else
        {
            // instead of overflowing or drawing a streamer shadow, don't draw a shadow at all
            if (_numShadowIndices + 3 > MAX_SHADOW_INDEXES)
            {
                _overflowed = true;
                return;
            }

            if (remap[i1] == -1 || remap[i2] == -1 || remap[i3] == -1)
            {
                rError() << "createShadowVolumeInFrustum: bad remap[]" << std::endl;
                return;
            }

            _shadowIndices[_numShadowIndices++] = remap[i3];
            _shadowIndices[_numShadowIndices++] = remap[i2];
            _shadowIndices[_numShadowIndices++] = remap[i1];
            faceCastsShadow[i] = 1;
        }
    }

    // add indexes for the back caps, which will just be reversals of the
    // front caps using the back vertexes
    std::size_t numCapIndexes = _numShadowIndices - firstShadowIndex;

    // if no faces have been defined for the shadow volume,
    // there won't be anything at all
    if (numCapIndexes == 0)
    {
        return;
    }

    //--------------- off-line processing ------------------

    // if we are running from dmap, perform the (very) expensive shadow optimizations
    // to remove internal sil edges and optimize the caps
    if (false/*callOptimizer*/) // greebo: defaults to false for the moment being
    {
#if 0
        optimizedShadow_t opt;
        
        // project all of the vertexes to the shadow plane, generating
        // an equal number of back vertexes
//      R_ProjectPointsToFarPlane( ent, light, farPlane, firstShadowVert, numShadowVerts );

        opt = SuperOptimizeOccluders( shadowVerts, shadowIndexes + firstShadowIndex, numCapIndexes, farPlane, lightOrigin );

        // pull off the non-optimized data
        numShadowIndexes = firstShadowIndex;
        numShadowVerts = firstShadowVert;

        // add the optimized data
        if ( numShadowIndexes + opt.totalIndexes > MAX_SHADOW_INDEXES 
            || numShadowVerts + opt.numVerts > MAX_SHADOW_VERTS ) {
            overflowed = true;
            common->Printf( "WARNING: overflowed MAX_SHADOW tables, shadow discarded\n" );
            Mem_Free( opt.verts );
            Mem_Free( opt.indexes );
            return;
        }

        for ( i = 0 ; i < opt.numVerts ; i++ ) {
            shadowVerts[numShadowVerts+i][0] = opt.verts[i][0];
            shadowVerts[numShadowVerts+i][1] = opt.verts[i][1];
            shadowVerts[numShadowVerts+i][2] = opt.verts[i][2];
            shadowVerts[numShadowVerts+i][3] = 1;
        }
        for ( i = 0 ; i < opt.totalIndexes ; i++ ) {
            int index = opt.indexes[i];
            if ( index < 0 || index > opt.numVerts ) {
                common->Error( "optimized shadow index out of range" );
            }
            shadowIndexes[numShadowIndexes+i] = index + numShadowVerts;
        }

        numShadowVerts += opt.numVerts;
        numShadowIndexes += opt.totalIndexes;

        // note the index distribution so we can sort all the caps after all the sils
        indexRef[indexFrustumNumber].frontCapStart = firstShadowIndex;
        indexRef[indexFrustumNumber].rearCapStart = firstShadowIndex+opt.numFrontCapIndexes;
        indexRef[indexFrustumNumber].silStart = firstShadowIndex+opt.numFrontCapIndexes+opt.numRearCapIndexes;
        indexRef[indexFrustumNumber].end = numShadowIndexes;
        indexFrustumNumber++;

        Mem_Free( opt.verts );
        Mem_Free( opt.indexes );
#endif
        return;
    }

    //--------------- real-time processing ------------------

    // the dangling edge "face" is never considered to cast a shadow,
    // so any face with dangling edges that casts a shadow will have
    // it's dangling sil edge trigger a sil plane
    faceCastsShadow[numTris] = 0;

    // instead of overflowing or drawing a streamer shadow, don't draw a shadow at all
    // if we ran out of space
    if (_numShadowIndices + numCapIndexes > MAX_SHADOW_INDEXES)
    {
        _overflowed = true;
        return;
    }

    for (std::size_t i = 0; i < numCapIndexes; i += 3)
    {
        _shadowIndices[_numShadowIndices + i + 0] = _shadowIndices[firstShadowIndex + i + 2] + 1;
        _shadowIndices[_numShadowIndices + i + 1] = _shadowIndices[firstShadowIndex + i + 1] + 1;
        _shadowIndices[_numShadowIndices + i + 2] = _shadowIndices[firstShadowIndex + i + 0] + 1;
    }

    _numShadowIndices += numCapIndexes;

    // c_caps += numCapIndexes * 2;

    std::size_t preSilIndexes = _numShadowIndices;

    // if any triangles were clipped, we will have a list of edges
    // on the frustum which must now become sil edges
    if (makeClippedPlanes)
    {
        addClipSilEdges();
    }

    // any edges that are a transition between a shadowing and
    // non-shadowing triangle will cast a silhouette edge
    addSilEdges(tri, pointCull, frustum, remap, faceCastsShadow);

    // c_sils += numShadowIndexes - preSilIndexes;

    // project all of the vertexes to the shadow plane, generating
    // an equal number of back vertexes
    projectPointsToFarPlane(transform, light, farPlane, firstShadowVert, _numShadowVerts);

    // note the index distribution so we can sort all the caps after all the sils
    _indexRef[_indexFrustumNumber].frontCapStart = firstShadowIndex;
    _indexRef[_indexFrustumNumber].rearCapStart = firstShadowIndex+numCapIndexes;
    _indexRef[_indexFrustumNumber].silStart = preSilIndexes;
    _indexRef[_indexFrustumNumber].end = _numShadowIndices;
    _indexFrustumNumber++;
}

Surface ProcCompiler::createShadowVolume(const Matrix4& transform, const Surface& tri, const ProcLight& light,
                             ShadowGenType optimize, Surface::CullInfo& cullInfo)
{
#if 0
    if ( !r_shadows.GetBool() ) {
        return NULL;
    }
#endif

    if (tri.silEdges.empty() || tri.indices.empty() || tri.vertices.empty())
    {
        return Surface();
    }

    //tr.pc.c_createShadowVolumes++;

    // use the fast infinite projection in dynamic situations, which
    // trades somewhat more overdraw and no cap optimizations for
    // a very simple generation process
    if (optimize == SG_DYNAMIC && true /*r_useTurboShadow.GetBool()*/)
    {
        // greebo: With the current settings this code won't be reached

        if (true /*tr.backEndRendererHasVertexPrograms*/ && true/*r_useShadowVertexProgram.GetBool()*/)
        {
             return createVertexProgramTurboShadowVolume(transform, tri, light, cullInfo);
        } 
        else
        {
            return createTurboShadowVolume(transform, tri, light, cullInfo);
        }
    }

    Surface newTri;

    calcInteractionFacing(transform, tri, light, cullInfo);

    std::size_t numFaces = tri.indices.size() / 3;
    
    unsigned char allFront = 1;

    for (std::size_t i = 0; i < numFaces && allFront; ++i)
    {
        allFront &= cullInfo.facing[i];
    }

    if (allFront)
    {
        // if no faces are the right direction, don't make a shadow at all
        return Surface();
    }

    // clear the shadow volume
    _numShadowIndices = 0;
    _numShadowVerts = 0;
    bool overflowed = false;
    _indexFrustumNumber = 0;
    int capPlaneBits = 0;
    bool callOptimizer = (optimize == SG_OFFLINE);

    // the facing information will be the same for all six projections
    // from a point light, as well as for any directed lights
    std::vector<unsigned char>& globalFacing = cullInfo.facing;

    unsigned char* faceCastsShadow = (unsigned char*)alloca(tri.indices.size() / 3 + 1);    // + 1 for fake dangling edge face
    int* remap = (int*)alloca(tri.vertices.size() * sizeof(int));

    Vector3 lightOrigin = globalPointToLocal(transform, light.getGlobalLightOrigin());
    
    // run through all the shadow frustums, which is one for a projected light,
    // and usually six for a point light, but point lights with centers outside
    // the box may have less
    for (std::size_t frustumNum = 0; frustumNum < light.numShadowFrustums; ++frustumNum)
    {
        const ShadowFrustum& frust = light.shadowFrustums[frustumNum];
        Plane3 frustum[6];

        // transform the planes into entity space
        // we could share and reverse some of the planes between frustums for a minor
        // speed increase

        // the cull test is redundant for a single shadow frustum projected light, because
        // the surface has already been checked against the main light frustums
        std::size_t j = 0;

        for (j = 0; j < frust.numPlanes; ++j)
        {
            frustum[j] = OptUtils::TransformPlane(frust.planes[j], transform);
            //R_GlobalPlaneToLocal( ent->modelMatrix, frust->planes[j], frustum[j] );

            // try to cull the entire surface against this frustum
            float d = planeDistanceToBounds(tri.bounds, frustum[j]);

            if (d < -LIGHT_CLIP_EPSILON)
            {
                break;
            }
        }

        if (j != frust.numPlanes)
        {
            continue;
        }

        // we need to check all the triangles
        std::size_t oldFrustumNumber = _indexFrustumNumber;

        createShadowVolumeInFrustum(transform, tri, light, lightOrigin, frustum, frustum[5], frust.makeClippedPlanes, remap, faceCastsShadow, globalFacing);

        // if we couldn't make a complete shadow volume, it is better to
        // not draw one at all, avoiding streamer problems
        if (overflowed)
        {
            return Surface();
        }

        if (_indexFrustumNumber != oldFrustumNumber)
        {
            // note that we have caps projected against this frustum,
            // which may allow us to skip drawing the caps if all projected
            // planes face away from the viewer and the viewer is outside the light volume
            capPlaneBits |= 1<<frustumNum;
        }
    }

    // if no faces have been defined for the shadow volume,
    // there won't be anything at all
    if (_numShadowIndices == 0) 
    {
        return Surface();
    }

    // this should have been prevented by the overflowed flag, so if it ever happens,
    // it is a code error
    if (_numShadowVerts > MAX_SHADOW_VERTS || _numShadowIndices > MAX_SHADOW_INDEXES)
    {
        throw std::runtime_error("Shadow volume exceeded allocation");
    }

    // allocate a new surface for the shadow volume
    // newTri = R_AllocStaticTriSurf();

    // we might consider setting this, but it would only help for
    // large lights that are partially off screen
    //newTri.bounds = AABB();

    // copy off the verts and indexes
    newTri.shadowVertices.assign(_shadowVerts.begin(), _shadowVerts.begin() + _numShadowVerts);

    newTri.indices.resize(_numShadowIndices);

    // the shadow verts will go into a main memory buffer as well as a vertex
    // cache buffer, so they can be copied back if they are purged
    //R_AllocStaticTriSurfShadowVerts( newTri, newTri->numVerts );
    //SIMDProcessor->Memcpy( newTri->shadowVertexes, shadowVerts, newTri->numVerts * sizeof( newTri->shadowVertexes[0] ) );

    //R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );

    if ( 1 /* sortCapIndexes */ )
    {
        newTri.shadowCapPlaneBits = capPlaneBits;

        // copy the sil indexes first
        newTri.numShadowIndicesNoCaps = 0;

        for (std::size_t i = 0; i < _indexFrustumNumber; ++i)
        {
            std::size_t c = _indexRef[i].end - _indexRef[i].silStart;

            memcpy(&newTri.indices[newTri.numShadowIndicesNoCaps], &_shadowIndices[_indexRef[i].silStart], c * sizeof(newTri.indices[0]));

            newTri.numShadowIndicesNoCaps += c;
        }

        // copy rear cap indexes next
        newTri.numShadowIndicesNoFrontCaps = newTri.numShadowIndicesNoCaps;

        for (std::size_t i = 0; i < _indexFrustumNumber; ++i)
        {
            std::size_t c = _indexRef[i].silStart - _indexRef[i].rearCapStart;

            memcpy(&newTri.indices[newTri.numShadowIndicesNoFrontCaps], &_shadowIndices[_indexRef[i].rearCapStart], c * sizeof(newTri.indices[0]));

            newTri.numShadowIndicesNoFrontCaps += c;
        }

        // copy front cap indexes last
        std::size_t numIndices = newTri.numShadowIndicesNoFrontCaps;

        for (std::size_t i = 0; i < _indexFrustumNumber; ++i)
        {
            std::size_t c = _indexRef[i].rearCapStart - _indexRef[i].frontCapStart;

            memcpy(&newTri.indices[numIndices], &_shadowIndices[_indexRef[i].frontCapStart], c * sizeof(newTri.indices[0]));

            numIndices += c;
        }
    }
//  else 
//  {
//      newTri->shadowCapPlaneBits = 63;    // we don't have optimized index lists
//      SIMDProcessor->Memcpy( newTri->indexes, shadowIndexes, newTri->numIndexes * sizeof( newTri->indexes[0] ) );
//  }

    if (false /*optimize == SG_OFFLINE*/) // greebo: cannot be true at the moment
    {
        //CleanupOptimizedShadowTris( newTri );
    }

    return newTri;
}

Surface ProcCompiler::createLightShadow(ProcArea::OptimizeGroups& shadowerGroups, const ProcLight& light)
{
    rMessage() << (boost::format("----- CreateLightShadow %s -----") % light.name) << std::endl;

    // optimize all the groups
    optimizeGroupList(shadowerGroups);

    Surface shadowTris;
    
    // combine all the triangles into one list
    ProcTris combined;

    for (ProcArea::OptimizeGroups::iterator group = shadowerGroups.begin(); 
         group != shadowerGroups.end(); ++group)
    {
        combined.insert(combined.end(), group->triList.begin(), group->triList.end());
    }

    /*rMessage() << "Combined: " << std::endl;

    std::size_t triCount = 0;
    for (ProcTris::const_iterator t = combined.begin(); t != combined.end(); ++t, ++triCount)
    {
        rMessage() << (boost::format("  Tri %d: (%f %f %f) (%f %f %f) (%f %f %f)\n") % triCount % 
            t->v[0].vertex.x() % t->v[0].vertex.y() % t->v[0].vertex.z() %
            t->v[1].vertex.x() % t->v[1].vertex.y() % t->v[1].vertex.z() %
            t->v[2].vertex.x() % t->v[2].vertex.y() % t->v[2].vertex.z());
    }*/

    if (combined.empty())
    {
        return shadowTris;
    }

    // find uniqued vertexes
    Surface occluders = shareMapTriVerts(combined);

    combined.clear();
    
    // find silhouette information for the triSurf
    occluders.cleanupTriangles(false, true, false);

    //rMessage() << (boost::format("Occluders: \n"));
    //rMessage() << occluders << std::endl;

    // let the renderer build the shadow volume normally
    Matrix4 transform = Matrix4::getIdentity();

#if 0
    idRenderEntityLocal     space;

    space.modelMatrix[0] = 1;
    space.modelMatrix[5] = 1;
    space.modelMatrix[10] = 1;
    space.modelMatrix[15] = 1;
#endif

    Surface::CullInfo cullInfo;

    // call the normal shadow creation, but with the superOptimize flag set, which will
    // call back to SuperOptimizeOccluders after clipping the triangles to each frustum
    if (true /*dmapGlobals.shadowOptLevel == SO_MERGE_SURFACES*/) // default is merge_surfaces
    {
        shadowTris = createShadowVolume(transform, occluders, light, SG_STATIC, cullInfo);
    }
    else
    {
        shadowTris = createShadowVolume(transform, occluders, light, SG_OFFLINE, cullInfo);
    }

    /*R_FreeStaticTriSurf( occluders );

    R_FreeInteractionCullInfo( cullInfo );

    if ( shadowTris ) {
        dmapGlobals.totalShadowTriangles += shadowTris->numIndexes / 3;
        dmapGlobals.totalShadowVerts += shadowTris->numVerts / 3;
    }*/

    //rMessage() << (boost::format("shadowTris: \n"));
    //rMessage() << shadowTris << std::endl;

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
        rMessage() << (boost::format("--- Light %s is casting shadows") % light.name) << std::endl;

        for (std::size_t i = 0; i < entity.numAreas; ++i)
        {
            //rMessage() << (boost::format("Prelighting area %d") % i) << std::endl;

            const ProcArea& area = entity.areas[i];

            int groupNum = 0;

            for (ProcArea::OptimizeGroups::const_iterator group = area.groups.begin(); 
                 group != area.groups.end(); ++group, ++groupNum)
            {
                //rMessage() << (boost::format("Group %d: %d tris (%s) Plane %d ") % groupNum % group->triList.size() % group->material->getName() % group->planeNum);

                // if the surface doesn't cast shadows, skip it
                if (!group->material->surfaceCastsShadow())
                {
                    //rMessage() << " doesn't cast a shadow\n";
                    continue;
                }

                // if the group doesn't face away from the light, it
                // won't contribute to the shadow volume
                if (_procFile->planes.getPlane(group->planeNum).distanceToPoint(lightOrigin) > 0)
                {
                    //rMessage() << " is not facing away\n";
                    continue;
                }

                // if the group bounds doesn't intersect the light bounds,
                // skip it
                if (!group->bounds.intersects(light.getFrustumTris().bounds))
                {
                    //rMessage() << " doesn't intersect bounds\n";
                    continue;
                }

                //rMessage() << " build shadower...\n";

                // build up a list of the triangle fragments inside the
                // light frustum
                ProcTris shadowers;

                for (ProcTris::const_iterator tri = group->triList.begin();
                     tri != group->triList.end(); ++tri)
                {
                    // clip it to the light frustum
                    ProcTris in;
                    ProcTris out;

                    clipTriByLight(light, *tri, in, out);

                    /*rMessage() << "in: ";

                    for (ProcTris::const_iterator t = in.begin(); t != in.end(); ++t)
                    {
                        rMessage() << (boost::format("\t<%f, %f, %f> | <%f, %f, %f> | <%f, %f, %f>\n") % 
                            t->v[0].vertex[0] % t->v[0].vertex[1] % t->v[0].vertex[2] %
                            t->v[1].vertex[0] % t->v[1].vertex[1] % t->v[1].vertex[2] %
                            t->v[2].vertex[0] % t->v[2].vertex[1] % t->v[2].vertex[2]);
                    }

                    rMessage() << std::endl;*/

                    shadowers.insert(shadowers.end(), in.begin(), in.end()); 
                }

                // if we didn't get any out of this group, we don't
                // need to create a new group in the shadower list
                if (shadowers.empty())
                {
                    continue;
                }

                //rMessage() << "has " << shadowers.size() << " shadowers now, ";

                // find a group in shadowerGroups to add these to
                // we will ignore everything but planenum, and we
                // can merge across areas
                ProcArea::OptimizeGroups::iterator check = shadowerGroups.begin();

                for ( ; check != shadowerGroups.end(); ++check)
                {
                    if (check->planeNum == group->planeNum)
                    {
                        //rMessage() << "sorting into existing group with plane " << check->planeNum << ", ";
                        break;
                    }
                }

                if (check == shadowerGroups.end())
                {
                    shadowerGroups.push_front(*group);
                    check = shadowerGroups.begin();
                    check->triList.clear(); // don't inherit the triangles of the other group
                    //rMessage() << "sorting into new group with plane " << check->planeNum << ", ";
                }

                // if any surface is a shadow-casting perforated or translucent surface, we
                // can't use the face removal optimizations because we can see through
                // some of the faces
                if (group->material->getCoverage() != Material::MC_OPAQUE)
                {
                    hasPerforatedSurface = true;
                }

                check->triList.insert(check->triList.end(), shadowers.begin(), shadowers.end());

                /*rMessage() << "has " << check->triList.size() << " tris now" << std::endl;

                rMessage() << "check->triList: ";

                for (ProcTris::const_iterator t = check->triList.begin(); t != check->triList.end(); ++t)
                {
                    rMessage() << (boost::format("\t<%f, %f, %f> | <%f, %f, %f> | <%f, %f, %f>\n") % 
                        t->v[0].vertex[0] % t->v[0].vertex[1] % t->v[0].vertex[2] %
                        t->v[1].vertex[0] % t->v[1].vertex[1] % t->v[1].vertex[2] %
                        t->v[2].vertex[0] % t->v[2].vertex[1] % t->v[2].vertex[2]);
                }

                rMessage() << std::endl;*/
            }
        }
    }

    /*rMessage() << "Before createLightShadow" << std::endl;

    std::size_t count = 0;

    for (ProcArea::OptimizeGroups::const_iterator g = shadowerGroups.begin(); g != shadowerGroups.end(); ++g, ++count)
    {
        rMessage() << (boost::format("  Group %d, plane %d, %d tris\n") % count % g->planeNum % g->triList.size());

        std::size_t triCount = 0;
        for (ProcTris::const_iterator t = g->triList.begin(); t != g->triList.end(); ++t, ++triCount)
        {
            rMessage() << (boost::format("  Tri %d: (%f %f %f) (%f %f %f) (%f %f %f)\n") % triCount % 
                t->v[0].vertex.x() % t->v[0].vertex.y() % t->v[0].vertex.z() %
                t->v[1].vertex.x() % t->v[1].vertex.y() % t->v[1].vertex.z() %
                t->v[2].vertex.x() % t->v[2].vertex.y() % t->v[2].vertex.z());
        }
    }*/

    // take the shadower group list and create a beam tree and shadow volume
    light.shadowTris = createLightShadow(shadowerGroups, light);

    //rMessage() << (boost::format("light->shadowTris: %d verts") % light.shadowTris.vertices.size());

    /*for (int i = 0; idDrawVert* t = light->shadowTris->verts; t; t = t->->next)
    {
        common->Printf("\t<%f, %f, %f> | <%f, %f, %f> | <%f, %f, %f>\n", t->v[0].xyz[0], t->v[0].xyz[1], t->v[0].xyz[2],
            t->v[1].xyz[0], t->v[1].xyz[1], t->v[1].xyz[2],
            t->v[2].xyz[0], t->v[2].xyz[1], t->v[2].xyz[2]);
    }*/

    //rMessage() << std::endl;

    if (!light.shadowTris.vertices.empty() && hasPerforatedSurface)
    {
        // can't ever remove front faces, because we can see through some of them
        light.shadowTris.numShadowIndicesNoCaps = light.shadowTris.numShadowIndicesNoFrontCaps = light.shadowTris.indices.size();
    }

    // we don't need the original shadower triangles for anything else
    //FreeOptimizeGroupList( shadowerGroups );
}

void ProcCompiler::preLight(ProcEntity& entity)
{
    // don't prelight anything but the world entity
    if (&entity != _procFile->entities[0].get())
    {
        return;
    }
    
    if (1 > 0 /*dmapGlobals.shadowOptLevel > 0*/) // FIXME: shadowopt level is 1 by default
    {
        rMessage() << "----- BuildLightShadows -----" << std::endl;
        
        // calc bounds for all the groups to speed things up
        for (std::size_t i = 0; i < entity.numAreas; ++i)
        {
            ProcArea& area = entity.areas[i];

            for (ProcArea::OptimizeGroups::iterator group = area.groups.begin();
                 group != area.groups.end(); ++group)
            {
                boundOptimizeGroup(*group);

                /*Vector3 mins = group->bounds.origin - group->bounds.extents;
                Vector3 maxs = group->bounds.origin + group->bounds.extents;

                rMessage() << (boost::format("Bounds: %f %f %f - %f %f %f\n") % mins[0] % mins[1] % mins[2] % maxs[0] % maxs[1] % maxs[2]);*/
            }
        }

        for (std::size_t i = 0; i < _procFile->lights.size(); ++i)
        {
            ProcLight& light = _procFile->lights[i];

            buildLightShadows(entity, light);
        }
    }

    if (false/* !dmapGlobals.noLightCarve */) // greebo: noLightCarve defaults to true
    {
        rMessage() << "----- CarveGroupsByLight -----" << std::endl;

        // now subdivide the optimize groups into additional groups for
        // each light that illuminates them
        for (std::size_t i = 0; i < _procFile->lights.size(); ++i)
        {
            ProcLight& light = _procFile->lights[i];
            // TODO CarveGroupsByLight( e, light );
        }

        //common->Printf( "%5.1f seconds for CarveGroupsByLight\n", ( end - start ) / 1000.0 );
    }
}

void ProcCompiler::optimizeEntity(ProcEntity& entity)
{
    rMessage() << "----- OptimizeEntity -----" << std::endl;

    for (std::size_t i = 0; i < entity.areas.size(); ++i)
    {
        optimizeGroupList(entity.areas[i].groups);
    }
}

void ProcCompiler::fixGlobalTjunctions(ProcEntity& entity)
{
    rMessage() << "----- FixGlobalTjunctions -----" << std::endl;

    _triangleHash.reset(new TriangleHash);

    // bound all the triangles to determine the bucket size
    _triangleHash->_hashBounds = AABB();

    for (std::size_t a = 0; a < entity.areas.size(); ++a)
    {
        _triangleHash->calculateBounds(entity.areas[a].groups);
    }

    // spread the bounds so it will never have a zero size
    _triangleHash->spreadHashBounds();

    for (std::size_t a = 0; a < entity.areas.size(); ++a)
    {
        _triangleHash->hashTriangles(entity.areas[a].groups);
    }

    // add all the func_static model vertexes to the hash buckets
    // optionally inline some of the func_static models
    if (&entity == _procFile->entities[0].get())
    {
        for (std::size_t eNum = 1; eNum < _procFile->entities.size(); ++eNum)
        {
            ProcEntity& otherEntity = *_procFile->entities[eNum];
            Entity& mapEnt = otherEntity.mapEntity->getEntity();

            if (mapEnt.getKeyValue("classname") != "func_static")
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
                rWarning() << "Cannot fix global t-junctions on entity " << mapEnt.getKeyValue("name") <<
                    " since the model cannot be loaded: " << modelName << std::endl;
                continue;
            }

            // get the rotation matrix in either full form, or single angle form
            std::string rotation = mapEnt.getKeyValue("rotation");

            Matrix4 axis;

            if (rotation.empty())
            {
                float angle = string::convert<float>(mapEnt.getKeyValue("angle"));

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

            Vector3 origin = string::convert<Vector3>(
                mapEnt.getKeyValue("origin")
            );

            for (int i = 0; i < model->getSurfaceCount(); ++i)
            {
                const model::IModelSurface& surface = model->getSurface(i);

                MaterialPtr material = GlobalMaterialManager().getMaterialForName(surface.getDefaultMaterial());

                for (int v = 0; v < surface.getNumVertices(); v += 3) // greebo: += 3 => is this ok with DR's model structure?
                {
                    const ArbitraryMeshVertex& vertex = surface.getVertex(v);

                    Vector3 transformed = axis.transformPoint(vertex.vertex) + origin;
                    _triangleHash->getHashVert(transformed);
                }
            }
        }
    }

    // now fix each area
    for (std::size_t a = 0; a < entity.areas.size(); ++a)
    {
        for (ProcArea::OptimizeGroups::iterator group = entity.areas[a].groups.begin();
             group != entity.areas[a].groups.end(); ++group)
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
    }
    
    // done
    _triangleHash.reset();
}

void ProcCompiler::freeTreePortalsRecursively(const BspTreeNodePtr& node)
{
    // free children
    if (node->planenum != PLANENUM_LEAF)
    {
        freeTreePortalsRecursively(node->children[0]);
        freeTreePortalsRecursively(node->children[1]);
    }
    
    // free portals
    ProcPortalPtr nextp;
    for (ProcPortalPtr p = node->portals; p; p = nextp)
    {
        int s = (p->nodes[1] == node);
        nextp = p->next[s];

        removePortalFromNode(p, p->nodes[!s]);
    }

    node->portals.reset();
}

std::size_t ProcCompiler::pruneNodesRecursively(const BspTreeNodePtr& node)
{
    if (node->planenum == PLANENUM_LEAF)
    {
        return node->area;
    }

    std::size_t a1 = pruneNodesRecursively(node->children[0]);
    std::size_t a2 = pruneNodesRecursively(node->children[1]);

    if (a1 != a2 || a1 == AREANUM_DIFFERENT)
    {
        return AREANUM_DIFFERENT;
    }

    // free all the nodes below this point
    freeTreePortalsRecursively(node->children[0]);
    freeTreePortalsRecursively( node->children[1]);
    
    node->children[0].reset();
    node->children[1].reset();
    
    // change this node to a leaf
    node->planenum = PLANENUM_LEAF;
    node->area = a1;

    return a1;
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
    if (floodFill/* && !dmapGlobals.noFlood*/)  // TODO: noflood option
    {
        if (floodEntities(entity.tree))
        {
            // set the outside leafs to opaque
            fillOutside(entity);
        }
        else
        {
            rMessage() <<  "**********************" << std::endl;
            rWarning() << "******* leaked *******" << std::endl;
            rMessage() <<  "**********************" << std::endl;
            
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

    /*rMessage() << "--- Planelist before PutPrimitivesInAreas --- " << std::endl;

    for (std::size_t i = 0; i < _procFile->planes.size(); ++i)
    {
        const Plane3& plane = _procFile->planes.getPlane(i);
        rMessage() << (boost::format("Plane %d: %f %f %f %f") % i % plane.normal().x() % plane.normal().y() % plane.normal().z() % plane.dist()) << std::endl;
    }*/

    // we now have a BSP tree with solid and non-solid leafs marked with areas
    // all primitives will now be clipped into this, throwing away
    // fragments in the solid areas
    putPrimitivesInAreas(entity);

    /*for (std::size_t i = 0; i < _procFile->planes.size(); ++i)
    {
        const Plane3& plane = _procFile->planes.getPlane(i);
        rMessage() << (boost::format("Plane %d: %f %f %f %f") % i % plane.normal().x() % plane.normal().y() % plane.normal().z() % plane.dist()) << std::endl;
    }*/

    // now build shadow volumes for the lights and split
    // the optimize lists by the light beam trees
    // so there won't be unneeded overdraw in the static
    // case
    preLight(entity);

    // optimizing is a superset of fixing tjunctions
    if (true/*!dmapGlobals.noOptimize*/) // greebo: noOptimize is false by default
    {
        optimizeEntity(entity);
    }
    else if (false/*!dmapGlobals.noTJunc*/)
    {
        // TODO FixEntityTjunctions( e );
    }

    // now fix t junctions across areas
    fixGlobalTjunctions(entity);

    // greebo: This was done by the proc output writer before, but it makes sense to 
    // do that before returning
    // prune unneeded nodes and count
    pruneNodesRecursively(entity.tree.head);

    return true;
}

} // namespace
