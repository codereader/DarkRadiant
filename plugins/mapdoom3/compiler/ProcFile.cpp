#include "ProcFile.h"
#include <fstream>
#include <boost/format.hpp>
#include "OptUtils.h"

namespace map
{

const char* const ProcFile::FILE_ID = "mapProcFile003";

namespace
{

std::size_t countUniqueShaders(const ProcArea::OptimizeGroups& groups)
{
	std::size_t count = 0;

	for (ProcArea::OptimizeGroups::const_iterator a = groups.begin(); 
		 a != groups.end(); ++a)
	{
		//globalOutputStream() << (boost::format("A: %d %s: ") % a->planeNum % a->material->getName());

		if (a->triList.empty())
		{	
			//globalOutputStream() << (boost::format("no triangles\n"));
			continue; // ignore groups with no tris
		}

		//globalOutputStream() << (boost::format("\n"));

		ProcArea::OptimizeGroups::const_iterator b;

		for (b = groups.begin(); b != a; ++b)
		{
			if (b->triList.empty())
			{
				//globalOutputStream() << (boost::format("no triangles\n  "));
				continue;
			}

			if (a->material != b->material)
			{
				//globalOutputStream() << (boost::format("material is not the same\n  "));
				continue;
			}

			if (a->mergeGroup != b->mergeGroup && a->mergePatch != b->mergePatch && a->mergeSurf != b->mergeSurf)
			{
				//globalOutputStream() << (boost::format("merge group is not the same\n  "));
				continue;
			}

			//globalOutputStream() << (boost::format("  B: %d %s: ") % b->planeNum % b->material->getName());
			//globalOutputStream() << (boost::format("equal, breaking\n"));

			break;
		}

		//globalOutputStream() << (boost::format("\n"));

		if (a == b)
		{
			count++;
			rMessage() << (boost::format(" Unique: %s, count is %d\n") % a->material->getName() % count);
		}
	}

	return count;
}

struct InteractionTris
{
	ProcTris	triList;
	ProcLight*	light;
};
typedef std::vector<InteractionTris> InteractionTrisList;

bool groupsAreSurfaceCompatible(ProcArea::OptimizeGroups::iterator a, 
								ProcArea::OptimizeGroups::iterator b)
{
	if (a->material != b->material)
	{
		return false;
	}

	if (a->mergeGroup != b->mergeGroup)
	{
		return false;
	}

	return true;
}

// Converts independent triangles to shared vertex triangles
Surface shareMapTriVerts(const ProcTris& tris)
{
	// unique the vertexes
	std::size_t count = tris.size();

	Surface uTri;
	uTri.vertices.reserve(count * 3);
	uTri.indices.reserve(count * 3);

	std::size_t numVerts = 0;
	std::size_t numIndexes = 0;

	for (ProcTris::const_iterator step = tris.begin(); step != tris.end(); ++step)
	{
		for (std::size_t i = 0; i < 3; ++i)
		{
			const ArbitraryMeshVertex& dv = step->v[i];

			// search for a match
			std::size_t j;

			for (j = 0 ; j < uTri.vertices.size(); ++j)
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

inline void writeFloat(std::ostream& str, float v)
{
	if (fabs(v - floorf(v + 0.5f)) < 0.001)
	{
		str << (boost::format("%i ") % (int)floorf(v + 0.5f));
	}
	else 
	{
		str << (boost::format("%f ") % v);
	}
}

// Writes text verts and indexes to procfile
inline std::ostream& writeSurface(std::ostream& str, const Surface& surf)
{
	// emit this chain
	str << (boost::format("/* numVerts = */ %i /* numIndexes = */ %i") % surf.vertices.size() % surf.indices.size()) << std::endl;

	// verts
	std::size_t col = 0;

	for (std::size_t i = 0; i < surf.vertices.size(); ++i)
	{
		const ArbitraryMeshVertex& dv = surf.vertices[i];

		str << "( ";
		writeFloat(str, dv.vertex[0]);
		writeFloat(str, dv.vertex[1]);
		writeFloat(str, dv.vertex[2]);
		writeFloat(str, dv.texcoord[0]);
		writeFloat(str, dv.texcoord[1]);
		writeFloat(str, dv.normal[0]);
		writeFloat(str, dv.normal[1]);
		writeFloat(str, dv.normal[2]);
		str << " ) ";

		if (++col == 3)
		{
			col = 0;
			str << std::endl;
		}
	}

	if (col != 0)
	{
		str << std::endl;
	}

	// indexes
	col = 0;

	for (std::size_t i = 0; i < surf.indices.size(); ++i)
	{
		str << (boost::format("%i ") % surf.indices[i]);

		if (++col == 18)
		{
			col = 0;
			str << std::endl;
		}
	}

	if (col != 0)
	{
		str << std::endl;
	}

	return str;
}

void writeOutputSurfaces(std::ostream& str, ProcEntity& entity, std::size_t areaNum)
{
	ProcArea& area = entity.areas[areaNum];

	rMessage() << (boost::format("==== Entity %d =====\n") % entity.entityNum);

	std::size_t numSurfaces = countUniqueShaders(area.groups);

	if (entity.entityNum == 0)
	{
		str << (boost::format("model { /* name = */ \"_area%i\" /* numSurfaces = */ %i") % areaNum % numSurfaces) 
			<< std::endl << std::endl;
	}
	else
	{
		std::string name = entity.mapEntity->getEntity().getKeyValue("name");

		if (name.empty())
		{
			rError() << (boost::format("Entity %i has surfaces, but no name key") % entity.entityNum) << std::endl;
			return;
		}

		str << (boost::format("model { /* name = */ \"%s\" /* numSurfaces = */ %i") % name % numSurfaces) 
			<< std::endl << std::endl;
	}

	std::size_t surfaceNum = 0;

	for (ProcArea::OptimizeGroups::iterator group = area.groups.begin(); 
		 group != area.groups.end(); ++group)
	{
		if (group->surfaceEmitted)
		{
			continue;
		}

		// combine all groups compatible with this one
		// usually several OptimizeGroups can be combined into a single
		// surface, even though they couldn't be merged together to save
		// vertexes because they had different planes, texture coordinates, or lights.
		// Different mergeGroups will stay in separate surfaces.
		ProcTris ambient;

		// each light that illuminates any of the groups in the surface will
		// get its own list of indexes out of the original surface
		InteractionTrisList interactions;

		for (ProcArea::OptimizeGroups::iterator groupStep = group; 
			 groupStep != area.groups.end(); ++groupStep)
		{
			if (groupStep->surfaceEmitted)
			{
				continue;
			}

			if (!groupsAreSurfaceCompatible(group, groupStep))
			{
				continue;
			}

			// copy it out to the ambient list
			ProcTris copy = groupStep->triList;
			ambient.insert(ambient.end(), copy.begin(), copy.end());
			groupStep->surfaceEmitted = true;

			// duplicate it into an interaction for each groupLight
			for (int i = 0; i < groupStep->numGroupLights; ++i)
			{
				InteractionTrisList::iterator checkInter;

				for (checkInter = interactions.begin(); checkInter != interactions.end(); ++checkInter)
				{
					if (checkInter->light == &groupStep->groupLights[i])
					{
						break;
					}
				}

				if (checkInter == interactions.end())
				{
					// create a new interaction
					InteractionTris newInter;
					newInter.light = &groupStep->groupLights[i];
					
					interactions.push_back(newInter);

					checkInter = interactions.end() - 1;
				}

				checkInter->triList.insert(checkInter->triList.end(), groupStep->triList.begin(), groupStep->triList.end());
			}
		}

		if (ambient.empty())
		{
			continue;
		}

		if (surfaceNum >= numSurfaces)
		{
			rError() << "writeOutputSurfaces: surfaceNum >= numSurfaces" << std::endl;
			return;
		}

		str << (boost::format("/* surface %i */ { ") % surfaceNum);
		
		surfaceNum++;
		
		str << (boost::format("\"%s\" ") % ambient.front().material->getName());

		Surface uTri = shareMapTriVerts(ambient);
		
		ambient.clear();

		uTri.cleanupUTriangles();
		
		writeSurface(str, uTri);

		str << "}" << std::endl << std::endl;
	}

	str << "}" << std::endl << std::endl;
}

int numberNodesRecursively(const BspTreeNodePtr& node, int nextNumber)
{
	if (node->planenum == PLANENUM_LEAF)
	{
		return nextNumber;
	}

	node->nodeNumber = nextNumber;
	nextNumber++;

	nextNumber = numberNodesRecursively(node->children[0], nextNumber);
	nextNumber = numberNodesRecursively(node->children[1], nextNumber);

	return nextNumber;
}

} // namespace

std::ostream& ProcFile::writeOutputPortals(std::ostream& str, ProcEntity& entity)
{
	str << (boost::format("interAreaPortals { /* numAreas = */ %i /* numIAP = */ %i") % entity.numAreas % interAreaPortals.size()) 
		<< std::endl << std::endl;

	str << "/* interAreaPortal format is: numPoints positiveSideArea negativeSideArea ( point) ... */" << std::endl;

	for (std::size_t i = 0; i < interAreaPortals.size(); ++i)
	{
		const ProcInterAreaPortal& iap = interAreaPortals[i];
		const ProcWinding& w = iap.side->winding;

		str << (boost::format("/* iap %i */ %i %i %i ") % i % w.size() % iap.area0 % iap.area1);

		str << "( ";

		for (std::size_t j = 0; j < w.size(); ++j)
		{
			writeFloat(str, w[j].vertex[0]);
			writeFloat(str, w[j].vertex[1]);
			writeFloat(str, w[j].vertex[2]);
		}

		str << ") ";

		str << std::endl;
	}

	str << "}" << std::endl<< std::endl;

	return str;
}

std::ostream& ProcFile::writeOutputNodeRecursively(std::ostream& str, const BspTreeNodePtr& node)
{
	if (node->planenum == PLANENUM_LEAF)
	{
		// we shouldn't get here unless the entire world
		// was a single leaf
		str << "/* node 0 */ ( 0 0 0 0 ) -1 -1" << std::endl;
		return str;
	}

	int child[2];

	for (std::size_t i = 0; i < 2; ++i)
	{
		if (node->children[i]->planenum == PLANENUM_LEAF)
		{
			child[i] = static_cast<int>(-1 - node->children[i]->area);
		}
		else 
		{
			child[i] = node->children[i]->nodeNumber;
		}
	}

	const Plane3& plane = planes.getPlane(node->planenum);

	str << (boost::format("/* node %i */ ") % node->nodeNumber);

	str << "( ";
	writeFloat(str, plane.normal()[0]);
	writeFloat(str, plane.normal()[1]);
	writeFloat(str, plane.normal()[2]);
	writeFloat(str, -plane.dist());
	str << ") ";
	str << (boost::format("%i %i") % child[0] % child[1]) << std::endl;

	if (child[0] > 0)
	{
		writeOutputNodeRecursively(str, node->children[0]);
	}

	if (child[1] > 0)
	{
		writeOutputNodeRecursively(str, node->children[1]);
	}

	return str;
}

std::ostream& ProcFile::writeOutputNodes(std::ostream& str, const BspTreeNodePtr& node)
{
	std::size_t numNodes = numberNodesRecursively(node, 0 );

	// output
	str << (boost::format("nodes { /* numNodes = */ %i") % numNodes) << std::endl << std::endl;

	str << "/* node format is: ( planeVector ) positiveChild negativeChild */" << std::endl;
	str << "/* a child number of 0 is an opaque, solid area */" << std::endl;
	str << "/* negative child numbers are areas: (-1-child) */" << std::endl;

	writeOutputNodeRecursively(str, node);

	str << "}" << std::endl << std::endl;

	return str;
}

std::ostream& ProcFile::writeProcEntity(std::ostream& str, ProcEntity& entity)
{
	if (entity.entityNum != 0)
	{
		// entities may have enclosed, empty areas that we don't need to write out
		if (entity.numAreas > 1)
		{
			entity.numAreas = 1;
		}
	}

	for (std::size_t a = 0; a < entity.numAreas; ++a)
	{
		writeOutputSurfaces(str, entity, a);
	}

	// we will completely skip the portals and nodes if it is a single area
	if (entity.entityNum == 0 && entity.numAreas > 1)
	{
		// output the area portals
		writeOutputPortals(str, entity);

		// output the nodes
		writeOutputNodes(str, entity.tree.head);
	}

	return str;
}

std::ostream& ProcFile::writeShadowTriangles(std::ostream& str, const Surface& tri)
{
	// emit this chain
	str << (boost::format("/* numVerts = */ %i /* noCaps = */ %i /* noFrontCaps = */ %i /* numIndexes = */ %i /* planeBits = */ %i")
		% tri.vertices.size() % tri.numShadowIndicesNoCaps % tri.numShadowIndicesNoFrontCaps % tri.indices.size() % tri.shadowCapPlaneBits);

	str << std::endl;

	// verts
	std::size_t col = 0;

	for (std::size_t i = 0 ; i < tri.vertices.size(); ++i)
	{
		str << "( ";
		writeFloat(str, tri.shadowVertices[i][0]);
		writeFloat(str, tri.shadowVertices[i][1]);
		writeFloat(str, tri.shadowVertices[i][2]);
		str << " )";
		
		if (++col == 5)
		{
			col = 0;
			str << std::endl;
		}
	}

	if (col != 0)
	{
		str << std::endl;
	}

	// indexes
	col = 0;

	for (std::size_t i = 0 ; i < tri.indices.size(); ++i)
	{
		str << (boost::format("%i ") % (str, tri.indices[i]));

		if (++col == 18 )
		{
			col = 0;
			str << std::endl;
		}
	}

	if (col != 0)
	{
		str << std::endl;
	}

	return str;
}

void ProcFile::saveToFile(const std::string& path)
{
	// write the file
	rMessage() << "----- WriteOutputFile -----" << std::endl;

	rMessage() << "writing " << path << std::endl;

	std::ofstream str(path.c_str());

	if (!str.good())
	{
		rMessage() << "error opening " << path << std::endl;
		return;
	}

	str << FILE_ID << std::endl << std::endl;

	// write the entity models and information, writing entities first
	for (ProcEntities::reverse_iterator i = entities.rbegin(); i != entities.rend(); ++i)
	{
		ProcEntity& entity = **i;
	
		if (entity.primitives.empty())
		{
			continue;
		}

		writeProcEntity(str, entity);
	}

	// write the shadow volumes
	for (std::size_t i = 0 ; i < lights.size(); ++i)
	{
		ProcLight& light = lights[i];

		if (light.shadowTris.vertices.empty())
		{
			continue;
		}

		str << (boost::format("shadowModel { /* name = */ \"_prelight_%s\"") % light.name) << std::endl << std::endl;

		writeShadowTriangles(str, light.shadowTris);

		str << "}" << std::endl << std::endl;
	}

	str.flush();
	str.close();
}

} // namespace
