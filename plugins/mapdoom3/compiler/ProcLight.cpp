#include "ProcLight.h"

namespace map
{

void ProcLight::parseFromSpawnargs(const Entity& ent)
{
	parms.suppressLightInViewID = false;
	parms.allowLightInViewID = false;
	parms.noShadows = false;
	parms.noSpecular = false;
	parms.pointLight = false;
	parms.parallel = false;

	if (!ent.getKeyValue("light_origin").empty())
	{
		parms.origin = Vector3(ent.getKeyValue("light_origin"));
	}
	else // fall back to "origin" if "light_origin" is empty
	{
		parms.origin = Vector3(ent.getKeyValue("origin"));
	}
	
	std::string target = ent.getKeyValue("light_target");
	std::string up = ent.getKeyValue("light_up");
	std::string right = ent.getKeyValue("light_right");
	std::string start = ent.getKeyValue("light_start");
	std::string end = ent.getKeyValue("light_end");

	bool gotTarget = !target.empty();
	bool gotUp = !up.empty();
	bool gotRight = !right.empty();

	// we should have all of the target/right/up or none of them
	if ((gotTarget || gotUp || gotRight) != (gotTarget && gotUp && gotRight))
	{
		globalErrorStream() << 
			(boost::format("Light at (%f,%f,%f) has bad target info") %
			parms.origin.x() % parms.origin.y() % parms.origin.z()).str() << std::endl;
		return;
	}

	if (!start.empty() && end.empty())
	{
		end = target; // end falls back to target if not set
	}

	if (gotTarget)
	{
		parms.target = Vector3(target);
		parms.up = Vector3(up);
		parms.right = Vector3(right);
		parms.start = Vector3(start);
		parms.end = Vector3(end);
	}

	if (!gotTarget)
	{
		parms.pointLight = true;

		// allow an optional relative center of light and shadow offset
		parms.lightCenter = ent.getKeyValue("light_center");
		
		// create a point light
		if (!ent.getKeyValue("light_radius").empty())
		{
			parms.lightRadius = ent.getKeyValue("light_radius");
		}
		else
		{
			float radius = 300;

			if (!ent.getKeyValue("light").empty())
			{
				radius = strToFloat(ent.getKeyValue("light"));
			}

			parms.lightRadius = Vector3(radius, radius, radius);
		}
	}

	// get the rotation matrix in either full form, or single angle form
	std::string rotation = ent.getKeyValue("light_rotation");

	if (rotation.empty())
	{
		rotation = ent.getKeyValue("rotation");

		if (rotation.empty())
		{
			float angle = strToFloat(ent.getKeyValue("angle"));

			// idMath::AngleNormalize360
			if (angle >= 360.0f || angle < 0.0f)
			{
				angle -= floor(angle / 360.0f) * 360.0f;
			}

			parms.axis = Matrix4::getRotationAboutZDegrees(angle);
		}
		else
		{
			parms.axis = getRotation(rotation);
		}
	}
	else
	{
		parms.axis = getRotation(rotation);
	}

	// fix degenerate identity matrices
	//mat[0].FixDegenerateNormal();
	//mat[1].FixDegenerateNormal();
	//mat[2].FixDegenerateNormal();

	// check for other attributes
	Vector3 color = ent.getKeyValue("_color").empty() ? Vector3(1,1,1) : Vector3(ent.getKeyValue("_color"));

	parms.shaderParms[0] = color[0];
	parms.shaderParms[1] = color[1];
	parms.shaderParms[2] = color[2];
	
	parms.shaderParms[3] = strToFloat(ent.getKeyValue("shaderParm3"), 1.0f);
	parms.shaderParms[4] = strToFloat(ent.getKeyValue("shaderParm4"), 0.0f);

	parms.shaderParms[5] = strToFloat(ent.getKeyValue("shaderParm5"), 0.0f);
	parms.shaderParms[6] = strToFloat(ent.getKeyValue("shaderParm6"), 0.0f);
	parms.shaderParms[7] = strToFloat(ent.getKeyValue("shaderParm7"), 0.0f);
	
	parms.noShadows = strToInt(ent.getKeyValue("noshadows"), 0) != 0;
	parms.noSpecular = strToInt(ent.getKeyValue("nospecular"), 0) != 0;
	parms.parallel = strToInt(ent.getKeyValue("parallel"), 0) != 0;

	std::string shader = ent.getKeyValue("texture");

	if (shader.empty())
	{
		shader = "lights/squarelight1";
	}

	parms.shader = GlobalMaterialManager().getMaterialForName(shader);
}

void ProcLight::deriveLightData()
{
	// decide which light shader we are going to use
	if (parms.shader)
	{
		lightShader = parms.shader;
	}

	if (!lightShader)
	{
		if (parms.pointLight)
		{
			lightShader = GlobalMaterialManager().getMaterialForName("lights/defaultPointLight");
		}
		else
		{
			lightShader = GlobalMaterialManager().getMaterialForName("lights/defaultProjectedLight");
		}
	}

	// get the falloff image
	falloffImage = lightShader->lightFalloffImage();

	if (!falloffImage)
	{
		// use the falloff from the default shader of the correct type
		MaterialPtr defaultShader;

		if (parms.pointLight)
		{
			defaultShader = GlobalMaterialManager().getMaterialForName( "lights/defaultPointLight" );
			falloffImage = defaultShader->lightFalloffImage();
		}
		else 
		{
			// projected lights by default don't diminish with distance
			defaultShader = GlobalMaterialManager().getMaterialForName( "lights/defaultProjectedLight" );
			falloffImage = defaultShader->lightFalloffImage();
		}
	}

	// set the projection
	if (!parms.pointLight)
	{
		// projected light
		// TODO
		//R_SetLightProject( lightProject, vec3_origin /* parms.origin */, parms.target, 
		//	parms.right, parms.up, parms.start, parms.end);
	}
	else
	{
		// point light
		lightProject[0].normal().x() = 0.5f / parms.lightRadius[0];
		lightProject[1].normal().y() = 0.5f / parms.lightRadius[1];
		lightProject[3].normal().z() = 0.5f / parms.lightRadius[2];
		lightProject[0].dist() = 0.5f;
		lightProject[1].dist() = 0.5f;
		lightProject[2].dist() = 1.0f;
		lightProject[3].dist() = 0.5f;
	}

	// set the frustum planes
	setLightFrustum();

	
	// rotate the light planes and projections by the axis
	//R_AxisToModelMatrix( parms.axis, parms.origin, modelMatrix );
	modelMatrix = parms.axis;
	modelMatrix.translateBy(parms.origin);

	for (std::size_t i = 0 ; i < 6 ; ++i)
	{
		frustum[i] = modelMatrix.transform(frustum[i]);
		//idPlane		temp;
		//temp = frustum[i];
		//R_LocalPlaneToGlobal( modelMatrix, temp, frustum[i] );
	}

	for (std::size_t i = 0 ; i < 4 ; ++i)
	{
		lightProject[i] = modelMatrix.transform(lightProject[i]);
		//idPlane		temp;
		//temp = lightProject[i];
		//R_LocalPlaneToGlobal( modelMatrix, temp, lightProject[i] );
	}

	// adjust global light origin for off center projections and parallel projections
	// we are just faking parallel by making it a very far off center for now
	if (parms.parallel)
	{
		Vector3	dir = parms.lightCenter;

		if (dir.getLengthSquared() <= 0)
		{
			// make point straight up if not specified
			dir = Vector3(0,0,1);
		}
		else
		{
			dir.normalise();
		}

		globalLightOrigin = parms.origin + dir * 100000;
	} 
	else
	{
		globalLightOrigin = parms.origin + parms.axis.transformPoint(parms.lightCenter);
	}

	//R_FreeLightDefFrustum( light );

	// TODO frustumTris = R_PolytopeSurface( 6, frustum, frustumWindings );

	/*// a projected light will have one shadowFrustum, a point light will have
	// six unless the light center is outside the box
	R_MakeShadowFrustums( light );
	*/
}

void ProcLight::setLightFrustum()
{
	// we want the planes of s=0, s=q, t=0, and t=q
	frustum[0] = lightProject[0];
	frustum[1] = lightProject[1];
	frustum[2] = lightProject[2] - lightProject[0];
	frustum[3] = lightProject[2] - lightProject[1];

	// we want the planes of s=0 and s=1 for front and rear clipping planes
	frustum[4] = lightProject[3];

	frustum[5] = lightProject[3];
	frustum[5].dist() -= 1.0f;
	frustum[5] = -frustum[5];

	for (std::size_t i = 0 ; i < 6 ; ++i)
	{
		frustum[i] = -frustum[i];
		frustum[i].normalise();
	}
}

Matrix4 ProcLight::getRotation(const std::string& value)
{
	float rotation[9];

	std::stringstream strm(value);
    strm << std::skipws;

	for (int i = 0; i < 9; ++i)
	{
		strm >> rotation[i];
	}

	if (!strm)
	{
		// Parsing failed, fall back to the identity matrix
		return Matrix4::getIdentity();
	}

	return Matrix4::byColumns(
		rotation[0],
		rotation[1],
		rotation[2],
		0,
		rotation[3],
		rotation[4],
		rotation[5],
		0,
		rotation[6],
		rotation[7],
		rotation[8],
		0,
		0,
		0,
		0,
		1
	);
}

} // namespace
