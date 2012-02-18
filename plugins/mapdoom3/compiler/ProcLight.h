#pragma once

#include "string/string.h"
#include "itextstream.h"
#include "ientity.h"
#include "ishaders.h"
#include "math/Matrix4.h"
#include "math/AABB.h"
#include "math/Vector3.h"
#include "render/ArbitraryMeshVertex.h"
#include "ProcWinding.h"
#include "Surface.h"

namespace map
{

#define MAX_ENTITY_SHADER_PARMS 12

struct RenderLightParms
{
	Matrix4					axis;				// rotation vectors, must be unit length
	Vector3					origin;

	// if non-zero, the light will not show up in the specific view,
	// which may be used if we want to have slightly different muzzle
	// flash lights for the player and other views
	int						suppressLightInViewID;

	// if non-zero, the light will only show up in the specific view
	// which can allow player gun gui lights and such to not effect everyone
	int						allowLightInViewID;

	// I am sticking the four bools together so there are no unused gaps in
	// the padded structure, which could confuse the memcmp that checks for redundant
	// updates
	bool					noShadows;			// (should we replace this with material parameters on the shader?)
	bool					noSpecular;			// (should we replace this with material parameters on the shader?)

	bool					pointLight;			// otherwise a projection light (should probably invert the sense of this, because points are way more common)
	bool					parallel;			// lightCenter gives the direction to the light at infinity
	Vector3					lightRadius;		// xyz radius for point lights
	Vector3					lightCenter;		// offset the lighting direction for shading and
												// shadows, relative to origin

	// frustum definition for projected lights, all reletive to origin
	// FIXME: we should probably have real plane equations here, and offer
	// a helper function for conversion from this format
	Vector3					target;
	Vector3					right;
	Vector3					up;
	Vector3					start;
	Vector3					end;

	// Dmap will generate an optimized shadow volume named _prelight_<lightName>
	// for the light against all the _area* models in the map.  The renderer will
	// ignore this value if the light has been moved after initial creation
	//idRenderModel *			prelightModel;

	// muzzle flash lights will not cast shadows from player and weapon world models
	int						lightId;

	MaterialPtr				shader;				// NULL = either lights/defaultPointLight or lights/defaultProjectedLight
	float					shaderParms[MAX_ENTITY_SHADER_PARMS];		// can be used in any way by shader
	//idSoundEmitter *		referenceSound;		// for shader sound tables, allowing effects to vary with sounds
};

struct ShadowFrustum
{
	int		numPlanes;		// this is always 6 for now
	Plane3	planes[6];
	// positive sides facing inward
	// plane 5 is always the plane the projection is going to, the
	// other planes are just clip planes
	// all planes are in global coordinates

	bool	makeClippedPlanes;
	// a projected light with a single frustum needs to make sil planes
	// from triangles that clip against side planes, but a point light
	// that has adjacent frustums doesn't need to

	ShadowFrustum() :
		numPlanes(6),
		makeClippedPlanes(false)
	{}
};

// Represents a map light
class ProcLight
{
private:
	// Most of the members of idRenderLightLocal have been moved here
	MaterialPtr	lightShader;
	TexturePtr	falloffImage;

	Plane3		lightProject[4];

	Plane3		frustum[6];				// in global space, positive side facing out, last two are front/back
	ProcWinding	frustumWindings[6];
	Surface		frustumTris;

	Matrix4		modelMatrix;	

	Vector3		globalLightOrigin;		// accounting for lightCenter and parallel

public:
	RenderLightParms	parms;

	// for naming the shadow volume surface and interactions
	std::string		name;

	Surface		shadowTris;

	std::size_t		numShadowFrustums;
	ShadowFrustum	shadowFrustums[6];

	ProcLight() :
		numShadowFrustums(0)
	{
		// Distance value in Plane3 is not initialised
		lightProject[0].dist() = lightProject[1].dist() = 
			lightProject[2].dist() = lightProject[3].dist() = 0;
	}

	void parseFromSpawnargs(const Entity& ent);

	void deriveLightData();

	const MaterialPtr& getLightShader() const 
	{
		return lightShader;
	}

	const Vector3& getGlobalLightOrigin() const
	{
		return globalLightOrigin;
	}

	const Plane3& getFrustumPlane(std::size_t i) const
	{
		return frustum[i];
	}

	const Surface& getFrustumTris() const
	{
		return frustumTris;
	}

private:
	Matrix4 getRotation(const std::string& value);

	// All values are reletive to the origin
	// Assumes that right and up are not normalized
	// This is also called by dmap during map processing.
	void setLightProject(Plane3 lightProject[4], const Vector3& origin, const Vector3& target,
					   const Vector3& rightVector, const Vector3& upVector, const Vector3& start, const Vector3& stop);

	// Creates plane equations from the light projection, positive sides
	// face out of the light (ref: R_SetLightFrustum)
	void setLightFrustum();

	void makeShadowFrustums();

	static Surface generatePolytopeSurface(int numPlanes, const Plane3* planes, ProcWinding* windings);
};

} // namespace
