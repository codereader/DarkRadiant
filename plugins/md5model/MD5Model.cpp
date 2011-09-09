#include "MD5Model.h"

#include "ivolumetest.h"
#include "ishaders.h"
#include "texturelib.h"
#include "ifilter.h"
#include "string/string.h"
#include "math/Quaternion.h"
#include "MD5DataStructures.h"

namespace md5 {

MD5Model::MD5Model() :
	_polyCount(0),
	_vertexCount(0),
	_renderableSkeleton(_skeleton)
{}

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
	return *_surfaces.back();
}

void MD5Model::updateAABB() {
	_aabb_local = AABB();
	for(SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i) {
		_aabb_local.includeAABB((*i)->localAABB());
	}
}

const AABB& MD5Model::localAABB() const {
	return _aabb_local;
}

void MD5Model::testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld)
{
	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		if (test.getVolume().TestAABB((*i)->localAABB(), localToWorld) != VOLUME_OUTSIDE)
		{
			(*i)->testSelect(selector, test, localToWorld);
		}
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
	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		(*i)->applySkin(skin);
	}

	updateMaterialList();
}

int MD5Model::getSurfaceCount() const {
	return static_cast<int>(size());
}

int MD5Model::getVertexCount() const {
	return static_cast<int>(_vertexCount);
}

int MD5Model::getPolyCount() const {
	return static_cast<int>(_polyCount);
}

void MD5Model::updateMaterialList() {
	_surfaceNames.clear();

	for (SurfaceList::const_iterator i = _surfaces.begin();
		 i != _surfaces.end();
		 ++i)
	{
		_surfaceNames.push_back((*i)->getActiveMaterial());
	}
}

const model::MaterialList& MD5Model::getActiveMaterials() const {
	return _surfaceNames;
}

const model::IModelSurface& MD5Model::getSurface(int surfaceNum) const
{
	assert(surfaceNum >= 0 && surfaceNum < _surfaces.size());
	return *_surfaces[surfaceNum];
}

void MD5Model::render(const RenderInfo& info) const
{
	// Render options
	if (info.checkFlag(RENDER_TEXTURE_2D))
	{
		glEnable(GL_TEXTURE_2D);
	}

	if (info.checkFlag(RENDER_SMOOTH))
	{
		glShadeModel(GL_SMOOTH);
	}

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
}

void MD5Model::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	for (SurfaceList::const_iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		(*i)->setRenderSystem(renderSystem);
	}
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
	std::size_t numJoints = strToSizet(tok.nextToken());
	tok.assertNextToken("numMeshes");
	std::size_t numMeshes = strToSizet(tok.nextToken());

	// ------ JOINTS  ------

	// Start of joints datablock
	tok.assertNextToken("joints");
	tok.assertNextToken("{");

	// Initialise the Joints vector with the specified number of objects
	_joints.resize(numJoints);

	// Iterate over the vector of Joints, filling in each one with parsed
	// values
	for(MD5Joints::iterator i = _joints.begin(); i != _joints.end(); ++i)
	{
		// Skip the joint name
		tok.skipTokens(1);

		// Index of parent joint
		i->parent = strToInt(tok.nextToken());

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
		surface.updateToDefaultPose(_joints);

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

	float x = strToFloat(tok.nextToken());
	float y = strToFloat(tok.nextToken());
	float z = strToFloat(tok.nextToken());

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
			(*i)->updateToDefaultPose(_joints);
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
		(*i)->updateToSkeleton(_skeleton);
	}
}

} // namespace
