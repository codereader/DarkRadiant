#include "ProcLight.h"

#include "ProcWinding.h"
#include "OptUtils.h"
#include "string/convert.h"

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
        parms.origin = string::convert<Vector3>(ent.getKeyValue("light_origin"));
    }
    else // fall back to "origin" if "light_origin" is empty
    {
        parms.origin = string::convert<Vector3>(ent.getKeyValue("origin"));
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
        rError() << 
            (boost::format("Light at (%f,%f,%f) has bad target info") %
            parms.origin.x() % parms.origin.y() % parms.origin.z()).str() << std::endl;
        return;
    }

    if (gotTarget)
    {
        parms.target = string::convert<Vector3>(target);
        parms.up = string::convert<Vector3>(up);
        parms.right = string::convert<Vector3>(right);
        parms.start = string::convert<Vector3>(start);
        parms.end = string::convert<Vector3>(end);
    }

    if (end.empty())
    {
        parms.end = parms.target;
    }

    if (!gotTarget)
    {
        parms.pointLight = true;

        // allow an optional relative center of light and shadow offset
        parms.lightCenter = string::convert<Vector3>(
            ent.getKeyValue("light_center")
        );
        
        // create a point light
        if (!ent.getKeyValue("light_radius").empty())
        {
            parms.lightRadius = string::convert<Vector3>(
                ent.getKeyValue("light_radius")
            );
        }
        else
        {
            float radius = 300;

            if (!ent.getKeyValue("light").empty())
            {
                radius = string::convert<float>(ent.getKeyValue("light"));
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
            float angle = string::convert<float>(ent.getKeyValue("angle"));

            // idMath::AngleNormalize360
            if (angle >= 360.0f || angle < 0.0f)
            {
                angle -= floor(angle / 360.0f) * 360.0f;
            }

            parms.axis = Matrix4::getRotationAboutZDegrees(angle);
        }
        else
        {
            parms.axis = Matrix4::getRotation(rotation);
        }
    }
    else
    {
        parms.axis = Matrix4::getRotation(rotation);
    }

    // fix degenerate identity matrices
    //mat[0].FixDegenerateNormal();
    //mat[1].FixDegenerateNormal();
    //mat[2].FixDegenerateNormal();

    // check for other attributes
    Vector3 color = ent.getKeyValue("_color").empty() 
                  ? Vector3(1,1,1)
                  : string::convert<Vector3>(ent.getKeyValue("_color"));

    parms.shaderParms[0] = color[0];
    parms.shaderParms[1] = color[1];
    parms.shaderParms[2] = color[2];
    
    parms.shaderParms[3] = string::convert<float>(ent.getKeyValue("shaderParm3"), 1.0f);
    parms.shaderParms[4] = string::convert<float>(ent.getKeyValue("shaderParm4"), 0.0f);

    parms.shaderParms[5] = string::convert<float>(ent.getKeyValue("shaderParm5"), 0.0f);
    parms.shaderParms[6] = string::convert<float>(ent.getKeyValue("shaderParm6"), 0.0f);
    parms.shaderParms[7] = string::convert<float>(ent.getKeyValue("shaderParm7"), 0.0f);
    
    parms.noShadows = string::convert<int>(ent.getKeyValue("noshadows"), 0) != 0;
    parms.noSpecular = string::convert<int>(ent.getKeyValue("nospecular"), 0) != 0;
    parms.parallel = string::convert<int>(ent.getKeyValue("parallel"), 0) != 0;

    std::string shader = ent.getKeyValue("texture");

    if (shader.empty())
    {
        shader = "lights/squarelight1";
    }

    parms.shader = GlobalMaterialManager().getMaterialForName(shader);
}

// greebo: might be similar to the section in Light::projection() in entity/light/Light.cpp
void ProcLight::setLightProject(Plane3 lightProject[4], const Vector3& origin, const Vector3& target,
                       const Vector3& rightVector, const Vector3& upVector, const Vector3& start, const Vector3& stop)
{
    Vector3 right = rightVector;
    float rLen = right.normalise();

    Vector3 up = upVector;
    float uLen = up.normalise();

    Vector3 normal = up.crossProduct(right);
//normal = right.Cross( up );
    normal.normalise();

    float dist = target.dot(normal); //  - ( origin * normal );
    
    if (dist < 0)
    {
        dist = -dist;
        normal = -normal;
    }

    float scale = (0.5f * dist) / rLen;
    right *= scale;

    scale = -(0.5f * dist) / uLen;
    up *= scale;

    lightProject[2].normal() = normal;
    lightProject[2].dist() = origin.dot(lightProject[2].normal());

    lightProject[0].normal() = right;
    lightProject[0].dist() = origin.dot(lightProject[0].normal());

    lightProject[1].normal() = up;
    lightProject[1].dist() = origin.dot(lightProject[1].normal());

    // now offset to center
    Vector4 targetGlobal(target + origin, 1);

    Vector4 lightProject2Vec4(lightProject[2].normal(), -lightProject[2].dist());

    float ofs = 0.5f - targetGlobal.dot(Vector4(lightProject[0].normal(), -lightProject[0].dist())) / targetGlobal.dot(lightProject2Vec4);
    
    lightProject[0].normal() += lightProject[2].normal() * ofs;
    lightProject[0].dist() += lightProject[2].dist() * ofs;

    Vector4 lightProject1Vec4(lightProject[1].normal(), -lightProject[1].dist());

    ofs = 0.5f - targetGlobal.dot(lightProject1Vec4) / targetGlobal.dot(lightProject2Vec4);

    lightProject[1].normal() += lightProject[2].normal() * ofs;
    lightProject[1].dist() += lightProject[2].dist() * ofs;

    // set the falloff vector
    normal = stop - start;
    dist = normal.normalise();
    
    if (dist <= 0)
    {
        dist = 1;
    }

    lightProject[3].normal() = normal * (1.0f / dist);

    Vector3 startGlobal = start + origin;

    lightProject[3].dist() = startGlobal.dot(lightProject[3].normal());
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
        setLightProject(lightProject, Vector3(0,0,0) /* parms.origin */, parms.target, 
            parms.right, parms.up, parms.start, parms.end);
    }
    else
    {
        // point light
        lightProject[0].normal().x() = 0.5f / parms.lightRadius[0];
        lightProject[1].normal().y() = 0.5f / parms.lightRadius[1];
        lightProject[3].normal().z() = 0.5f / parms.lightRadius[2];
        lightProject[0].dist() = -0.5f;
        lightProject[1].dist() = -0.5f;
        lightProject[2].dist() = -1.0f;
        lightProject[3].dist() = -0.5f;
    }

    // set the frustum planes
    setLightFrustum();
    
    // rotate the light planes and projections by the axis
    modelMatrix = parms.axis;
    modelMatrix.translateBy(parms.origin);

    for (std::size_t i = 0 ; i < 6 ; ++i)
    {
        frustum[i] = OptUtils::TransformPlane(frustum[i], modelMatrix);
    }

    for (std::size_t i = 0 ; i < 4 ; ++i)
    {
        lightProject[i] = OptUtils::TransformPlane(lightProject[i], modelMatrix);
    }

    // adjust global light origin for off center projections and parallel projections
    // we are just faking parallel by making it a very far off center for now
    if (parms.parallel)
    {
        Vector3 dir = parms.lightCenter;

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

    frustumTris = generatePolytopeSurface(6, frustum, frustumWindings);

    // a projected light will have one shadowFrustum, a point light will have
    // six unless the light center is outside the box
    makeShadowFrustums();
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
    frustum[5].dist() += 1.0f;
    frustum[5] = -frustum[5];

    for (std::size_t i = 0 ; i < 6 ; ++i)
    {
        frustum[i] = -frustum[i];
        frustum[i].normalise();
    }
}

#define MAX_POLYTOPE_PLANES 6

Surface ProcLight::generatePolytopeSurface(int numPlanes, const Plane3* planes, ProcWinding* windings)
{
    ProcWinding planeWindings[MAX_POLYTOPE_PLANES];

    if (numPlanes > MAX_POLYTOPE_PLANES)
    {
        rError() << "generatePolytopeSurface: more than " << 
            MAX_POLYTOPE_PLANES << " planes." << std::endl;
        return Surface();
    }

    std::size_t numVerts = 0;
    std::size_t numIndices = 0;

    for (std::size_t i = 0; i < numPlanes; ++i)
    {
        const Plane3& plane = planes[i];
        ProcWinding& w = planeWindings[i];

        w.setFromPlane(plane);

        for (std::size_t j = 0; j < numPlanes; ++j)
        {
            const Plane3& plane2 = planes[j];

            if (j == i)
            {
                continue;
            }

            if (!w.clip(-plane2, ON_EPSILON))
            {
                break;
            }
        }

        if (w.size() <= 2)
        {
            continue;
        }

        numVerts += w.size();
        numIndices += (w.size() - 2) * 3;
    }

    Surface tri;
    tri.vertices.reserve(numVerts);
    tri.indices.reserve(numIndices);

    // copy the data from the windings
    for (std::size_t i = 0; i < numPlanes; ++i)
    {
        ProcWinding& w = planeWindings[i];

        if (w.empty())
        {
            continue;
        }

        std::size_t startVert = tri.vertices.size();

        for (std::size_t j = 0; j < w.size(); ++j)
        {
            tri.vertices.push_back(ArbitraryMeshVertex(w[j].vertex, Vector3(0,0,0), TexCoord2f(0,0)));
        }

        for (std::size_t j = 1; j < w.size() - 1; ++j)
        {
            tri.indices.push_back(static_cast<int>(startVert));
            tri.indices.push_back(static_cast<int>(startVert + j));
            tri.indices.push_back(static_cast<int>(startVert + j + 1));
        }

        // optionally save the winding
        if (windings)
        {
            windings[i].swap(w);
        }
    }

    tri.calcBounds();
    
    return tri;
}

void ProcLight::makeShadowFrustums()
{
    if (parms.pointLight)
    {
        // exact projection,taking into account asymetric frustums when 
        // globalLightOrigin isn't centered

        static int  faceCorners[6][4] = {
            { 7, 5, 1, 3 },     // positive X side
            { 4, 6, 2, 0 },     // negative X side
            { 6, 7, 3, 2 },     // positive Y side
            { 5, 4, 0, 1 },     // negative Y side
            { 6, 4, 5, 7 },     // positive Z side
            { 3, 1, 0, 2 }      // negative Z side
        };
        static int  faceEdgeAdjacent[6][4] = {
            { 4, 4, 2, 2 },     // positive X side
            { 7, 7, 1, 1 },     // negative X side
            { 5, 5, 0, 0 },     // positive Y side
            { 6, 6, 3, 3 },     // negative Y side
            { 0, 0, 3, 3 },     // positive Z side
            { 5, 5, 6, 6 }      // negative Z side
        };

        bool centerOutside = false;

        // if the light center of projection is outside the light bounds,
        // we will need to build the planes a little differently
        if (fabs(parms.lightCenter[0]) > parms.lightRadius[0] || 
            fabs(parms.lightCenter[1]) > parms.lightRadius[1] || 
            fabs(parms.lightCenter[2]) > parms.lightRadius[2])
        {
            centerOutside = true;
        }

        // make the corners
        Vector3 corners[8];

        for (int i = 0 ; i < 8 ; ++i)
        {
            Vector3 temp;

            for (int j = 0 ; j < 3 ; ++j)
            {
                if (i & (1 << j))
                {
                    temp[j] = parms.lightRadius[j];
                } 
                else
                {
                    temp[j] = -parms.lightRadius[j];
                }
            }

            // transform to global space
            corners[i] = parms.origin + parms.axis.transformPoint(temp);
        }

        numShadowFrustums = 0;

        for (std::size_t side = 0; side < 6; ++side) 
        {
            ShadowFrustum& frust = shadowFrustums[numShadowFrustums];

            Vector3& p1 = corners[faceCorners[side][0]];
            Vector3& p2 = corners[faceCorners[side][1]];
            Vector3& p3 = corners[faceCorners[side][2]];
            
            // plane will have positive side inward
            Plane3 backPlane(p1, p2, p3);

            // if center of projection is on the wrong side, skip
            float d = backPlane.distanceToPoint(globalLightOrigin);

            if (d < 0)
            {
                continue;
            }

            frust.numPlanes = 6;
            frust.planes[5] = backPlane;
            frust.planes[4] = backPlane;    // we don't really need the extra plane

            // make planes with positive side facing inwards in light local coordinates
            for (std::size_t edge = 0; edge < 4; ++edge)
            {
                Vector3& p1 = corners[faceCorners[side][edge]];
                Vector3& p2 = corners[faceCorners[side][(edge+1)&3]];

                // create a plane that goes through the center of projection
                frust.planes[edge] = Plane3(p1, p2, globalLightOrigin); // Plane(p1, p0, p2) call convention to match D3

                // see if we should use an adjacent plane instead
                if (centerOutside)
                {
                    Vector3& p3 = corners[faceEdgeAdjacent[side][edge]];
                    Plane3 sidePlane(p1, p2, p3); // Plane(p1, p0, p2) call convention to match D3

                    d = sidePlane.distanceToPoint(globalLightOrigin);

                    if (d < 0)
                    {
                        // use this plane instead of the edged plane
                        frust.planes[edge] = sidePlane;
                    }

                    // we can't guarantee a neighbor, so add sill planes at edge
                    shadowFrustums[numShadowFrustums].makeClippedPlanes = true;
                }
            }

            numShadowFrustums++;
        }

        return;
    }
    
    // projected light

    numShadowFrustums = 1;
    ShadowFrustum& frust = shadowFrustums[0];

    // flip and transform the frustum planes so the positive side faces
    // inward in local coordinates

    // it is important to clip against even the near clip plane, because
    // many projected lights that are faking area lights will have their
    // origin behind solid surfaces.
    for (std::size_t i = 0; i < 6; ++i)
    {
        Plane3& plane = frust.planes[i];

        plane.normal() = -frustum[i].normal();
        plane.dist() = -frustum[i].dist();
    }
    
    frust.numPlanes = 6;
    frust.makeClippedPlanes = true;

    // projected lights don't have shared frustums, so any clipped edges
    // right on the planes must have a sil plane created for them
}

} // namespace
