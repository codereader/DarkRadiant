#include "PicoModelLoader.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "imodelcache.h"
#include "ishaders.h"

#include "lib/picomodel.h"
#include "gamelib.h"

#include "os/path.h"

#include "idatastream.h"
#include "string/case_conv.h"
#include "../StaticModel.h"
#include "../StaticModelSurface.h"

namespace model {

namespace
{
	size_t picoInputStreamReam(void* inputStream, unsigned char* buffer, size_t length)
    {
		return reinterpret_cast<InputStream*>(inputStream)->read(buffer, length);
	}

    // Convert byte pointers to colour vector
    inline Vector4 getColourVector(unsigned char* array)
    {
        if (array)
        {
            return Vector4(array[0] / 255.0f, array[1] / 255.0f, array[2] / 255.0f, array[3] / 255.0f);
        }
        
        return Vector4(1.0, 1.0, 1.0, 1.0); // white
    }
} // namespace

PicoModelLoader::PicoModelLoader(const picoModule_t* module, const std::string& extension) :
    ModelImporterBase(extension),
	_module(module)
{}

// Load the given model from the VFS path
IModelPtr PicoModelLoader::loadModelFromPath(const std::string& path)
{
	// Open an ArchiveFile to load
	auto file = path_is_absolute(path.c_str()) ?
        GlobalFileSystem().openFileInAbsolutePath(path) :
        GlobalFileSystem().openFile(path);

	if (!file)
	{
		rError() << "Failed to load model " << path << std::endl;
		return IModelPtr();
	}

	// Determine the file extension (ASE or LWO) to pass down to the PicoModel
	std::string fName = file->getName();
	string::to_lower(fName);
	std::string fExt = fName.substr(fName.size() - 3, 3);

	picoModel_t* model = PicoModuleLoadModelStream(
		_module,
		&file->getInputStream(),
		picoInputStreamReam,
		file->size(),
		0
	);

	// greebo: Check if the model load was successful
	if (!model || model->numSurfaces == 0)
	{
		// Model is either NULL or has no surfaces, this must've failed
		return IModelPtr();
	}

    // Convert the pico model surfaces to StaticModelSurfaces
    auto surfaces = CreateSurfaces(model, fExt);

	auto modelObj = std::make_shared<StaticModel>(surfaces);

	// Set the filename
	modelObj->setFilename(os::getFilename(file->getName()));
	modelObj->setModelPath(path);

	PicoFreeModel(model);

	return modelObj;
}

std::vector<StaticModelSurfacePtr> PicoModelLoader::CreateSurfaces(picoModel_t* picoModel, const std::string& extension)
{
    // Convert the pico model surfaces to StaticModelSurfaces
    std::vector<StaticModelSurfacePtr> surfaces;

    // Get the number of surfaces to create
    int nSurf = PicoGetModelNumSurfaces(picoModel);

    // Create a StaticModelSurface for each surface in the structure
    for (int n = 0; n < nSurf; ++n)
    {
        // Retrieve the surface, discarding it if it is null or non-triangulated (?)
        picoSurface_t* surf = PicoGetModelSurface(picoModel, n);

        auto rSurf = CreateSurface(surf, extension);

        if (!rSurf) continue;

        surfaces.emplace_back(rSurf);
    }

    return surfaces;
}

std::string PicoModelLoader::CleanupShaderName(const std::string& inName)
{
    const std::string baseFolder = "base";	//FIXME: should be from game.xml
    std::size_t basePos;

    std::string mapName = string::replace_all_copy(inName, "\\", "/");

    // for paths given relative, start from the beginning
    if (mapName.substr(0, 6) == "models" || mapName.substr(0, 8) == "textures")
    {
        basePos = 0;
    }
    else
    {
        // Take off the everything before "base/", and everything after
        // the first period if it exists (i.e. strip off ".tga")
        basePos = mapName.find(baseFolder);
        if (basePos == std::string::npos)
        {
            // Unrecognised shader path, no base folder.
            // Try the original incase it was already given relative.
            basePos = 0;
        }
        else
        {
            // Increment for for the length of "base/", the / is the +1
            basePos += (baseFolder.size() + 1);
        }
    }

    std::size_t dotPos = mapName.find(".");

    if (dotPos != std::string::npos)
    {
        return mapName.substr(basePos, dotPos - basePos);
    }
    else
    {
        return mapName.substr(basePos);
    }
}

std::string PicoModelLoader::DetermineDefaultMaterial(picoSurface_t* picoSurface, const std::string& extension)
{
    // Get the shader from the picomodel struct. If this is a LWO model, use
    // the material name to select the shader, while for an ASE model the
    // bitmap path should be used.
    picoShader_t* shader = PicoGetSurfaceShader(picoSurface);
    std::string rawName = "";
    std::string defaultMaterial;

    if (shader != 0)
    {
        if (extension == "lwo")
        {
            defaultMaterial = PicoGetShaderName(shader);
        }
        else if (extension == "ase")
        {
            rawName = PicoGetShaderName(shader);
            std::string rawMapName = PicoGetShaderMapName(shader);
            defaultMaterial = CleanupShaderName(rawMapName);
        }
        else // if extension is not handled explicitly, use at least something
        {
            defaultMaterial = PicoGetShaderName(shader);
        }
    }

    // #4644: Doom3 / TDM don't use the *MATERIAL_NAME in ASE models, only *BITMAP is used
    // Use the fallback (introduced in #2499) only when the game allows it
    if (game::current::getValue<bool>("/modelFormat/ase/useMaterialNameIfNoBitmapFound"))
    {
        // If shader not found, fallback to alternative if available
        // _defaultMaterial is empty if the ase material has no BITMAP
        // materialIsValid is false if _defaultMaterial is not an existing shader
        if ((defaultMaterial.empty() || !GlobalMaterialManager().materialExists(defaultMaterial)) &&
            !rawName.empty())
        {
            defaultMaterial = CleanupShaderName(rawName);
        }
    }

    return defaultMaterial;
}

StaticModelSurfacePtr PicoModelLoader::CreateSurface(picoSurface_t* picoSurface, const std::string& extension)
{
    if (picoSurface == 0 || PicoGetSurfaceType(picoSurface) != PICO_TRIANGLES)
    {
        return StaticModelSurfacePtr();
    }

    // Fix the normals of the surface (?)
    PicoFixSurfaceNormals(picoSurface);

    // Convert the pico vertex data to the types we need to construct a StaticModelSurface

    // Get the number of vertices and indices, and reserve capacity in our
    // vectors in advance by populating them with empty structs.
    auto numVertices = PicoGetSurfaceNumVertexes(picoSurface);
    unsigned int numIndices = PicoGetSurfaceNumIndexes(picoSurface);
    
    std::shared_ptr<StaticModelSurface> staticSurface;

    {
        // Allocate the vectors that will be moved to the surface at end of scope
        std::vector<ArbitraryMeshVertex> vertices(numVertices);
        std::vector<unsigned int> indices(numIndices);

        // Stream in the vertex data from the raw struct, expanding the local AABB
        // to include each vertex.
        for (int vNum = 0; vNum < numVertices; ++vNum) {

            // Get the vertex position and colour
            Vertex3f vertex(PicoGetSurfaceXYZ(picoSurface, vNum));

            Normal3f normal = PicoGetSurfaceNormal(picoSurface, vNum);

            vertices[vNum].vertex = vertex;
            vertices[vNum].normal = normal;
            vertices[vNum].texcoord = TexCoord2f(PicoGetSurfaceST(picoSurface, 0, vNum));
            vertices[vNum].colour = getColourVector(PicoGetSurfaceColor(picoSurface, 0, vNum));
        }

        // Stream in the index data
        picoIndex_t* ind = PicoGetSurfaceIndexes(picoSurface, 0);

        for (unsigned int i = 0; i < numIndices; i++)
        {
            indices[i] = ind[i];
        }

        staticSurface = std::make_shared<StaticModelSurface>(std::move(vertices), std::move(indices));
    }

    staticSurface->setDefaultMaterial(DetermineDefaultMaterial(picoSurface, extension));

    return staticSurface;
}

} // namespace model
