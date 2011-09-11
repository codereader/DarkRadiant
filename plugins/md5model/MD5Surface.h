#pragma once

#include "irender.h"
#include "render.h"
#include "irenderable.h"
#include "math/AABB.h"
#include "math/Frustum.h"
#include "iselectiontest.h"
#include "modelskin.h"
#include "imodelsurface.h"

#include "MD5DataStructures.h"
#include "parser/DefTokeniser.h"

namespace md5
{

class MD5Skeleton;

class MD5Surface :
	public model::IModelSurface,
	public OpenGLRenderable
{
public:
	typedef VertexBuffer<ArbitraryMeshVertex> Vertices;
	typedef IndexBuffer Indices;

private:
	AABB _aabb_local;

	// Shader name
	std::string _shaderName;
	std::string _originalShaderName;

	// Shader object
	ShaderPtr _shader;

	// The mesh definition - will be baked into renderable vertex arrays
	MD5Mesh _mesh;

	Vertices _vertices;
	Indices _indices;	

	// The GL display lists for this surface's geometry
	GLuint _normalList;
	GLuint _lightingList;

	// We need to keep a reference for skin swapping
	RenderSystemWeakPtr _renderSystem;

private:

	// Capture the named shader
	void captureShader();

	// Create the display lists
	void createDisplayLists();

	// Re-calculate the normal vectors
	void buildVertexNormals();

public:

	/**
	 * Constructor.
	 */
	MD5Surface();

	/**
	 * Destructor.
	 */
	~MD5Surface();

	// Set/get the shader name
	void setShader(const std::string& name);

	/**
	 * Get the Shader object.
	 */
	const ShaderPtr& getState() const;

	/**
	 * Calculate the AABB and build the display lists for rendering.
	 */
	void updateGeometry();

	// Updates the mesh to the pose defined in the .md5mesh file - usually a T-Pose
	// It needs the joints defined in that file as reference
	void updateToDefaultPose(const MD5Joints& joints);

	// Updates this mesh to the state of the given skeleton
	void updateToSkeleton(const MD5Skeleton& skeleton);

	// Applies the given Skin to this surface.
	void applySkin(const ModelSkin& skin);

    // Back-end render function
    void render(const RenderInfo& info) const;

	const AABB& localAABB() const;

	void render(RenderableCollector& collector, const Matrix4& localToWorld, 
				const IRenderEntity& entity) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	// Test for selection
	void testSelect(Selector& selector,
					SelectionTest& test,
					const Matrix4& localToWorld);

	// IModelSurface implementation
	int getNumVertices() const;
	int getNumTriangles() const;

	const ArbitraryMeshVertex& getVertex(int vertexIndex) const;
	model::ModelPolygon getPolygon(int polygonIndex) const;

	const std::string& getDefaultMaterial() const;
	const std::string& getActiveMaterial() const;

	void parseFromTokens(parser::DefTokeniser& tok);

	// Rebuild the render index array - usually needs to be called only once
	void buildIndexArray();
};
typedef boost::shared_ptr<MD5Surface> MD5SurfacePtr;

} // namespace
