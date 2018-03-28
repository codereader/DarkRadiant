#include "MD5Model.h"

#include "ivolumetest.h"
#include "ishaders.h"
#include "texturelib.h"
#include "ifilter.h"
#include "string/convert.h"
#include "math/Quaternion.h"
#include "math/Ray.h"
#include "MD5DataStructures.h"

namespace md5 {

MD5Model::MD5Model() :
	_polyCount(0),
	_vertexCount(0),
	_renderableSkeleton(_skeleton)
{}

MD5Model::MD5Model(const MD5Model& other) :
	_joints(other._joints),
	_surfaces(other._surfaces.size()), // resize to fit other
	_aabb_local(other._aabb_local),
	_polyCount(other._polyCount),
	_vertexCount(other._vertexCount),
	_filename(other._filename),
	_modelPath(other._modelPath),
	_renderableSkeleton(_skeleton)
{
	// Copy-construct the other model's surfaces, but not its shaders, revert to default
	for (std::size_t i = 0; i < other._surfaces.size(); ++i)
	{
		_surfaces[i].surface.reset(new MD5Surface(*other._surfaces[i].surface));
		_surfaces[i].activeMaterial = _surfaces[i].surface->getDefaultMaterial();

		// Build the index array - this has to happen at least once
		_surfaces[i].surface->buildIndexArray();
		_surfaces[i].surface->updateToDefaultPose(_joints);
	}

	updateMaterialList();
}

MD5Model::const_iterator MD5Model::begin() const {
	return _surfaces.begin();
}

MD5Model::const_iterator MD5Model::end() const {
	return _surfaces.end();
}

std::size_t MD5Model::size() const {
	return _surfaces.size();
}

MD5Surface& MD5Model::createNewSurface()
{
	_surfaces.push_back(MD5SurfacePtr(new MD5Surface));
	return *(_surfaces.back().surface);
}

void MD5Model::updateAABB()
{
	_aabb_local = AABB();

	for(SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		_aabb_local.includeAABB(i->surface->localAABB());
	}
}

const AABB& MD5Model::localAABB() const
{
	return _aabb_local;
}

void MD5Model::testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld)
{
	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		if (test.getVolume().TestAABB(i->surface->localAABB(), localToWorld) != VOLUME_OUTSIDE)
		{
			i->surface->testSelect(selector, test, localToWorld);
		}
	}
}

bool MD5Model::getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld)
{
	Vector3 bestIntersection = ray.origin;

	// Test each surface and take the nearest point to the ray origin
	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		Vector3 surfaceIntersection;

		if (i->surface->getIntersection(ray, surfaceIntersection, localToWorld))
		{
			// Test if this surface intersection is better than what we currently have
			float oldDistSquared = (bestIntersection - ray.origin).getLengthSquared();
			float newDistSquared = (surfaceIntersection - ray.origin).getLengthSquared();

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
	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		const std::string& defaultMaterial = i->surface->getDefaultMaterial();
		const std::string& activeMaterial = i->activeMaterial;

		// Look up the remap for this surface's material name. If there is a remap
		// change the Shader* to point to the new shader.
		std::string remap = skin.getRemap(defaultMaterial);

		if (!remap.empty() && remap != activeMaterial)
		{
			// Save the remapped shader name
			i->activeMaterial = remap;
		}
		else if (remap.empty() && activeMaterial != defaultMaterial)
		{
			// No remap, so reset our shader to the original unskinned shader
			i->activeMaterial = defaultMaterial;
		}
	}

	captureShaders();
	updateMaterialList();
}

int MD5Model::getSurfaceCount() const
{
	return static_cast<int>(size());
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

	for (SurfaceList::const_iterator i = _surfaces.begin();
		 i != _surfaces.end();
		 ++i)
	{
		_surfaceNames.push_back(i->activeMaterial);
	}
}

const model::StringList& MD5Model::getActiveMaterials() const
{
	return _surfaceNames;
}

void MD5Model::captureShaders()
{
	RenderSystemPtr renderSystem = _renderSystem.lock();

	// Capture or release our shaders
	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		if (renderSystem)
		{
			i->shader = renderSystem->capture(i->activeMaterial);
		}
		else
		{
			i->shader.reset();
		}
	}
}

const model::IModelSurface& MD5Model::getSurface(unsigned surfaceNum) const
{
	assert(surfaceNum >= 0 && surfaceNum < _surfaces.size());
	return *(_surfaces[surfaceNum].surface);
}

void MD5Model::render(const RenderInfo& info) const
{
#if 0 // greebo: No state changes in back-end render methods!
	// Render options
	if (info.checkFlag(RENDER_TEXTURE_2D))
	{
		glEnable(GL_TEXTURE_2D);
	}

	if (info.checkFlag(RENDER_SMOOTH))
	{
		glShadeModel(GL_SMOOTH);
	}
#endif

// greebo: We don't need a back-end render method here, at least not yet (FIXME)
#if 0
	for (SurfaceList::const_iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		// Get the Material to test the shader name against the filter system
		const MaterialPtr& surfaceShader = (*i)->getState()->getMaterial();

		if (surfaceShader->isVisible())
		{
			// Bind the OpenGL texture and render the surface geometry
			TexturePtr tex = surfaceShader->getEditorImage();
			glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());
			(*i)->render(info.getFlags());
		}
	}
#endif
}

void MD5Model::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	_renderSystem = renderSystem;

	captureShaders();
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
	    float lSq = rawRotation.getLengthSquared();

	    float w = -sqrt(1.0f - lSq);
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
		for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
		{
			i->surface->updateToDefaultPose(_joints);
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

	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		i->surface->updateToSkeleton(_skeleton);
	}
}

} // namespace
