#include "MD5Model.h"

#include "ivolumetest.h"
#include "texturelib.h"
#include "ifilter.h"
#include "string/convert.h"
#include "math/Quaternion.h"
#include "math/Ray.h"
#include "MD5DataStructures.h"

namespace md5
{

MD5Model::MD5Model() :
	_polyCount(0),
	_vertexCount(0)
{}

MD5Model::MD5Model(const MD5Model& other) :
	_joints(other._joints),
	_surfaces(other._surfaces.size()), // resize to fit other
	_aabb_local(other._aabb_local),
	_polyCount(other._polyCount),
	_vertexCount(other._vertexCount),
	_filename(other._filename),
	_modelPath(other._modelPath)
{
	// Copy-construct the other model's surfaces, but not its shaders, revert to default
	for (std::size_t i = 0; i < other._surfaces.size(); ++i)
	{
		_surfaces[i].reset(new MD5Surface(*other._surfaces[i]));
		_surfaces[i]->setActiveMaterial(_surfaces[i]->getDefaultMaterial());

		// Build the index array - this has to happen at least once
		_surfaces[i]->buildIndexArray();
		_surfaces[i]->updateToDefaultPose(_joints);
	}

	updateMaterialList();
}

void MD5Model::foreachSurface(const std::function<void(const MD5Surface&)>& functor) const
{
    for (const auto& surface : _surfaces)
    {
        functor(*surface);
    }
}

MD5Surface& MD5Model::createNewSurface()
{
	_surfaces.push_back(std::make_shared<MD5Surface>());
	return *(_surfaces.back());
}

void MD5Model::updateAABB()
{
	_aabb_local = AABB();

	for (const auto& surface : _surfaces)
	{
		_aabb_local.includeAABB(surface->localAABB());
	}
}

const AABB& MD5Model::localAABB() const
{
	return _aabb_local;
}

void MD5Model::testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld)
{
    for (const auto& surface : _surfaces)
	{
		if (test.getVolume().TestAABB(surface->localAABB(), localToWorld) != VOLUME_OUTSIDE)
		{
			surface->testSelect(selector, test, localToWorld);
		}
	}
}

bool MD5Model::getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld)
{
	Vector3 bestIntersection = ray.origin;

	// Test each surface and take the nearest point to the ray origin
    for (const auto& surface : _surfaces)
	{
		Vector3 surfaceIntersection;

		if (surface->getIntersection(ray, surfaceIntersection, localToWorld))
		{
			// Test if this surface intersection is better than what we currently have
			auto oldDistSquared = (bestIntersection - ray.origin).getLengthSquared();
			auto newDistSquared = (surfaceIntersection - ray.origin).getLengthSquared();

			if ((oldDistSquared == 0 && newDistSquared > 0) || newDistSquared < oldDistSquared)
			{
				bestIntersection = surfaceIntersection;
			}
		}
	}

	if ((bestIntersection - ray.origin).getLengthSquared() > 0)
	{
		intersection = bestIntersection;
		return true;
	}
	else
	{
		return false;
	}
}

std::string MD5Model::getFilename() const {
	return _filename;
}

void MD5Model::setFilename(const std::string& name) {
	_filename = name;
}

std::string MD5Model::getModelPath() const {
	return _modelPath;
}

void MD5Model::setModelPath(const std::string& modelPath) {
	_modelPath = modelPath;
}

void MD5Model::applySkin(const ModelSkin& skin)
{
	// Apply the skin to each surface, then try to capture shaders
    for (const auto& surface : _surfaces)
	{
		const std::string& defaultMaterial = surface->getDefaultMaterial();
		const std::string& activeMaterial = surface->getActiveMaterial();

		// Look up the remap for this surface's material name. If there is a remap
		// change the Shader* to point to the new shader.
		std::string remap = skin.getRemap(defaultMaterial);

		if (!remap.empty() && remap != activeMaterial)
		{
			// Save the remapped shader name
			surface->setActiveMaterial(remap);
		}
		else if (remap.empty() && activeMaterial != defaultMaterial)
		{
			// No remap, so reset our shader to the original unskinned shader
			surface->setActiveMaterial(defaultMaterial);
		}
	}

	updateMaterialList();
}

int MD5Model::getSurfaceCount() const
{
	return static_cast<int>(_surfaces.size());
}

int MD5Model::getVertexCount() const
{
	return static_cast<int>(_vertexCount);
}

int MD5Model::getPolyCount() const
{
	return static_cast<int>(_polyCount);
}

void MD5Model::updateMaterialList()
{
	_surfaceNames.clear();

    for (const auto& surface : _surfaces)
	{
		_surfaceNames.push_back(surface->getActiveMaterial());
	}
}

const model::StringList& MD5Model::getActiveMaterials() const
{
	return _surfaceNames;
}

const model::IIndexedModelSurface& MD5Model::getSurface(unsigned surfaceNum) const
{
	assert(surfaceNum >= 0 && surfaceNum < _surfaces.size());
	return *_surfaces[surfaceNum];
}

void MD5Model::parseFromTokens(parser::DefTokeniser& tok)
{
	_vertexCount = 0;
	_polyCount = 0;

	// Check the version number
	tok.assertNextToken("MD5Version");
	tok.assertNextToken("10");

	// Commandline
	tok.assertNextToken("commandline");
	tok.skipTokens(1); // quoted command string

	// Number of joints and meshes
	tok.assertNextToken("numJoints");
	std::size_t numJoints = string::convert<std::size_t>(tok.nextToken());
	tok.assertNextToken("numMeshes");
	std::size_t numMeshes = string::convert<std::size_t>(tok.nextToken());

	// ------ JOINTS  ------

	// Start of joints datablock
	tok.assertNextToken("joints");
	tok.assertNextToken("{");

	// Initialise the Joints vector with the specified number of objects
	MD5Joints& joints = _joints;

	joints.resize(numJoints);

	// Iterate over the vector of Joints, filling in each one with parsed
	// values
	for(MD5Joints::iterator i = joints.begin(); i != joints.end(); ++i)
	{
		// Skip the joint name
		tok.skipTokens(1);

		// Index of parent joint
		i->parent = string::convert<int>(tok.nextToken());

		// Joint's position vector
		i->position = parseVector3(tok);

	    // Parse joint's rotation
		Vector3 rawRotation = parseVector3(tok);

	    // Calculate the W value. If it is NaN (due to underflow in the sqrt),
	    // set it to 0.
	    auto lSq = rawRotation.getLengthSquared();

	    auto w = -sqrt(1.0 - lSq);
	    if (isNaN(w)) {
	    	w = 0;
	    }

		// Set the Vector4 rotation on the joint
	    i->rotation = Quaternion(rawRotation, w);
	}

	// End of joints datablock
	tok.assertNextToken("}");

	// ------ MESHES ------

	// For each mesh, there should be a mesh datablock
	for (std::size_t i = 0; i < numMeshes; ++i)
	{
		// Construct the surface for this mesh
		MD5Surface& surface = createNewSurface();

		surface.parseFromTokens(tok);

		// Build the index array - this has to happen at least once
		surface.buildIndexArray();

		// Build the default vertex array
		surface.updateToDefaultPose(joints);

		// Update the vertexcount
		_vertexCount += surface.getNumVertices();

		// Update the polycount
		_polyCount += surface.getNumTriangles();
	}

	updateAABB();
	updateMaterialList();
}

Vector3 MD5Model::parseVector3(parser::DefTokeniser& tok) {
	tok.assertNextToken("(");

	float x = string::convert<float>(tok.nextToken());
	float y = string::convert<float>(tok.nextToken());
	float z = string::convert<float>(tok.nextToken());

	tok.assertNextToken(")");

	return Vector3(x, y, z);
}

void MD5Model::setAnim(const IMD5AnimPtr& anim)
{
	_anim = anim;

	if (!_anim)
	{
        for (const auto& surface : _surfaces)
		{
			surface->updateToDefaultPose(_joints);
		}
	}
}

const IMD5AnimPtr& MD5Model::getAnim() const
{
	return _anim;
}

void MD5Model::updateAnim(std::size_t time)
{
	if (!_anim) return; // nothing to do

	// Update our joint hierarchy first
	_skeleton.update(_anim, time);

    for (const auto& surface : _surfaces)
	{
		surface->updateToSkeleton(_skeleton);
	}

    signal_ModelAnimationUpdated().emit();
}

sigc::signal<void>& MD5Model::signal_ModelAnimationUpdated()
{
    return _sigModelAnimationUpdated;
}

} // namespace
