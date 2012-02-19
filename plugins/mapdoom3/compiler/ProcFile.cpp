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

	for (ProcArea::OptimizeGroups::const_reverse_iterator a = groups.rbegin(); 
		 a != groups.rend(); ++a)
	{
		if (a->triList.empty())
		{	
			continue; // ignore groups with no tris
		}

		ProcArea::OptimizeGroups::const_reverse_iterator b;

		for (b = groups.rbegin(); b != a; ++b)
		{
			if (b->triList.empty())
			{
				continue;
			}

			if (a->material != b->material)
			{
				continue;
			}

			if (a->mergeGroup != b->mergeGroup && a->mergePatch != b->mergePatch && a->mergeSurf != b->mergeSurf)
			{
				continue;
			}

			break;
		}

		if (a == b)
		{
			count++;
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

bool groupsAreSurfaceCompatible(ProcArea::OptimizeGroups::reverse_iterator a, 
								ProcArea::OptimizeGroups::reverse_iterator b)
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

	uTri.vertices.shrink_to_fit();
	uTri.indices.shrink_to_fit();

	return uTri;
}

inline std::ostream& writeFloat(std::ostream& str, float v)
{
	if (fabs(v - floorf(v + 0.5f)) < 0.001)
	{
		str << (boost::format("%i ") % (int)floorf(v + 0.5f));
	}
	else 
	{
		str << (boost::format("%f ") % v);
	}

	return str;
}

// Writes text verts and indexes to procfile
inline std::ostream& operator<<(std::ostream& str, const Surface& surf)
{
	// emit this chain
	str << (boost::format("/* numVerts = */ %i /* numIndexes = */ %i") % surf.vertices.size() % surf.indices.size()) << std::endl;

	// verts
	std::size_t col = 0;

	for (std::size_t i = 0; i < surf.vertices.size(); ++i)
	{
		const ArbitraryMeshVertex& dv = surf.vertices[i];

		str << "( " << writeFloat(str, dv.vertex[0]) << writeFloat(str, dv.vertex[1]) << writeFloat(str, dv.vertex[2]) 
			<< writeFloat(str, dv.texcoord[0]) << writeFloat(str, dv.texcoord[1])
			<< writeFloat(str, dv.normal[0]) << writeFloat(str, dv.normal[1]) << writeFloat(str, dv.normal[2])
			<< ") ";


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
			globalErrorStream() << (boost::format("Entity %i has surfaces, but no name key") % entity.entityNum) << std::endl;
			return;
		}

		str << (boost::format("model { /* name = */ \"%s\" /* numSurfaces = */ %i") % name % numSurfaces) 
			<< std::endl << std::endl;
	}

	std::size_t surfaceNum = 0;

	for (ProcArea::OptimizeGroups::reverse_iterator group = area.groups.rbegin(); 
		 group != area.groups.rend(); ++group)
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

		for (ProcArea::OptimizeGroups::reverse_iterator groupStep = group; 
			 groupStep != area.groups.rend(); ++groupStep)
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
			globalErrorStream() << "writeOutputSurfaces: surfaceNum >= numSurfaces" << std::endl;
			return;
		}

		str << (boost::format("/* surface %i */ { ") % surfaceNum);
		
		surfaceNum++;
		
		str << (boost::format("\"%s\" ") % ambient[0].material->getName());

		Surface uTri = shareMapTriVerts(ambient);
		
		ambient.clear();

		uTri.cleanupUTriangles();
		
		str << uTri;

		str << "}" << std::endl << std::endl;
	}

	str << "}" << std::endl << std::endl;
}

std::ostream& operator<<(std::ostream& str, ProcEntity& entity)
{
	std::size_t numWriteAreas = entity.numAreas;

	if (entity.entityNum != 0)
	{
		// entities may have enclosed, empty areas that we don't need to write out
		if (entity.numAreas > 1)
		{
			numWriteAreas = 1;
		}
	}

	for (std::size_t a = 0; a < numWriteAreas; ++a)
	{
		writeOutputSurfaces(str, entity, a);
	}

	// we will completely skip the portals and nodes if it is a single area
	if (entity.entityNum == 0 && numWriteAreas > 1)
	{
		// output the area portals
		// WriteOutputPortals( e );

		// output the nodes
		// WriteOutputNodes( e->tree->headnode );
	}

	return str;
}

} // namespace

void ProcFile::saveToFile(const std::string& path)
{
	// write the file
	globalOutputStream() << "----- WriteOutputFile -----" << std::endl;

	globalOutputStream() << "writing " << path << std::endl;

	std::ofstream str(path.c_str());

	if (!str.good())
	{
		globalOutputStream() << "error opening " << path << std::endl;
		return;
	}

	str << FILE_ID << std::endl << std::endl;

	// write the entity models and information, writing entities first
	for (std::size_t i = entities.size() - 1; i >= 0; i--)
	{
		ProcEntity& entity = *entities[i];
	
		if (entity.primitives.empty())
		{
			continue;
		}

		str << entity << std::endl;
	}
#if 0
	// write the shadow volumes
	for ( i = 0 ; i < dmapGlobals.mapLights.Num() ; i++ ) {
		mapLight_t	*light = dmapGlobals.mapLights[i];
		if ( !light->shadowTris ) {
			continue;
		}

		procFile->WriteFloatString( "shadowModel { /* name = */ \"_prelight_%s\"\n\n", light->name );
		WriteShadowTriangles( light->shadowTris );
		procFile->WriteFloatString( "}\n\n" );

		R_FreeStaticTriSurf( light->shadowTris );
		light->shadowTris = NULL;
	}

	fileSystem->CloseFile( procFile );
#endif
}

} // namespace
