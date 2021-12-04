#pragma once

#include "math/Plane3.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "texturelib.h"
#include "ibrush.h"

namespace map
{

namespace quake3
{

// Code ported from GtkRadiant to calculate the texture projection matrix as done in Q3
// without the ComputeAxisBase preprocessing that is happening in idTech4 before applying texcoords
// https://github.com/TTimo/GtkRadiant/blob/a1ae77798f434bf8fb31a7d91cd137d1ce554e33/radiant/brush.cpp#L85
inline void getTextureAxisFromPlane(const Vector3& normal, Vector3& xv, Vector3& yv)
{
    static const Vector3 baseaxis[18] =
    {
        {0,0,1}, {1,0,0}, {0,-1,0},     // floor
        {0,0,-1}, {1,0,0}, {0,-1,0},    // ceiling
        {1,0,0}, {0,1,0}, {0,0,-1},     // west wall
        {-1,0,0}, {0,1,0}, {0,0,-1},    // east wall
        {0,1,0}, {1,0,0}, {0,0,-1},     // south wall
        {0,-1,0}, {1,0,0}, {0,0,-1}     // north wall
    };

    Vector3::ElementType best = 0;
    int bestaxis = 0;

    for (int i = 0; i < 6; i++)
    {
        auto dot = normal.dot(baseaxis[i * 3]);

        if (dot > best)
        {
            best = dot;
            bestaxis = i;
        }
    }

    xv = baseaxis[bestaxis * 3 + 1];
    yv = baseaxis[bestaxis * 3 + 2];
}

// Code ported from GtkRadiant 
// https://github.com/TTimo/GtkRadiant/blob/a1ae77798f434bf8fb31a7d91cd137d1ce554e33/radiant/brush.cpp#L424
inline double HighestImpactSign(double a, double b)
{
	// returns the sign of the value with larger abs
    return a + b > 0 ? +1 : -1;
}

inline ShiftScaleRotation calculateTexDefFromTransform(const IFace& face, const Matrix4& transform, float imageWidth, float imageHeight)
{
    // Copy the matrix components into the 8-float-structure the GtkRadiant algorithm is based on
    double STfromXYZ[2][4];

    STfromXYZ[0][0] = transform.xx();
    STfromXYZ[0][1] = transform.yx();
    STfromXYZ[0][2] = transform.zx();

    STfromXYZ[1][0] = transform.xy();
    STfromXYZ[1][1] = transform.yy();
    STfromXYZ[1][2] = transform.zy();

    STfromXYZ[0][3] = transform.tx();
    STfromXYZ[1][3] = transform.ty();

    int sv;
    int tv;

    Vector3 pvecs[2];
    getTextureAxisFromPlane(face.getPlane3().normal(), pvecs[0], pvecs[1]);

    if (pvecs[0][0])
    {
        sv = 0;
    }
    else if (pvecs[0][1])
    {
        sv = 1;
    }
    else
    {
        sv = 2;
    }

    if (pvecs[1][0])
    {
        tv = 0;
    }
    else if (pvecs[1][1])
    {
        tv = 1;
    }
    else
    {
        tv = 2;
    }

	// undo the texture transform
	for (int j = 0 ; j < 4 ; j++ ) 
    {
		STfromXYZ[0][j] *= imageWidth;
		STfromXYZ[1][j] *= imageHeight;
	}

    ShiftScaleRotation td;

	// shift
	td.shift[0] = STfromXYZ[0][3];
	td.shift[1] = STfromXYZ[1][3];

	td.scale[0] = sqrt( STfromXYZ[0][sv] * STfromXYZ[0][sv] + STfromXYZ[0][tv] * STfromXYZ[0][tv] );
	td.scale[1] = sqrt( STfromXYZ[1][sv] * STfromXYZ[1][sv] + STfromXYZ[1][tv] * STfromXYZ[1][tv] );

	if (td.scale[0])
    {
		td.scale[0] = 1 / td.scale[0]; // avoid NaNs
	}

	if (td.scale[1])
    {
		td.scale[1] = 1 / td.scale[1];
	}

	double sign0tv = ( STfromXYZ[0][tv] > 0 ) ? +1 : -1;
	double ang = atan2( sign0tv * STfromXYZ[0][tv], sign0tv * STfromXYZ[0][sv] ); // atan2(y, x) with y positive is in [0, PI[

	td.scale[0] *= HighestImpactSign( STfromXYZ[0][tv] * +sin( ang ), STfromXYZ[0][sv] * cos( ang ) ) * pvecs[0][sv];
	td.scale[1] *= HighestImpactSign( STfromXYZ[1][sv] * -sin( ang ), STfromXYZ[1][tv] * cos( ang ) ) * pvecs[1][tv];

	td.rotate = ang * 180 / math::PI; // FIXME possibly snap this to 0/90/180 (270 can't happen)?

    return td;
}

// Originally ported from GtkRadiant, made to work with DarkRadiant's data structures
// https://github.com/TTimo/GtkRadiant/blob/a1ae77798f434bf8fb31a7d91cd137d1ce554e33/radiant/brush.cpp#L331
inline void getTextureVectorsForFace(const Vector3& normal, const ShiftScaleRotation& ssr, float texWidth, float texHeight, double STfromXYZ[2][4])
{
    memset(STfromXYZ, 0, 8 * sizeof(double));

    // get natural texture axis
    Vector3 pvecs[2];
    getTextureAxisFromPlane(normal, pvecs[0], pvecs[1]);

    Vector3::ElementType sinv, cosv;

    // rotate axis
    if (ssr.rotate == 0)
    {
        sinv = 0; cosv = 1;
    }
    else if (ssr.rotate == 90)
    {
        sinv = 1; cosv = 0;
    }
    else if (ssr.rotate == 180)
    {
        sinv = 0; cosv = -1;
    }
    else if (ssr.rotate == 270)
    {
        sinv = -1; cosv = 0;
    }
    else
    {
        auto angle = ssr.rotate / 180 * math::PI;
        sinv = sin(angle);
        cosv = cos(angle);
    }

    int sv, tv;

    if (pvecs[0][0])
    {
        sv = 0;
    }
    else if (pvecs[0][1])
    {
        sv = 1;
    }
    else
    {
        sv = 2;
    }

    if (pvecs[1][0])
    {
        tv = 0;
    }
    else if (pvecs[1][1])
    {
        tv = 1;
    }
    else
    {
        tv = 2;
    }

    for (int i = 0; i < 2; i++)
    {
        auto ns = cosv * pvecs[i][sv] - sinv * pvecs[i][tv];
        auto nt = sinv * pvecs[i][sv] + cosv * pvecs[i][tv];
        STfromXYZ[i][sv] = ns;
        STfromXYZ[i][tv] = nt;
    }

    // scale
    for (int j = 0; j < 3; j++)
    {
        STfromXYZ[0][j] /= ssr.scale[0];
        STfromXYZ[1][j] /= ssr.scale[1];
    }

    // shift
    STfromXYZ[0][3] = ssr.shift[0];
    STfromXYZ[1][3] = ssr.shift[1];

    for (int j = 0; j < 4; j++)
    {
        STfromXYZ[0][j] /= texWidth;
        STfromXYZ[1][j] /= texHeight;
    }
}

inline Matrix4 calculateTextureMatrix(const Vector3& normal, const ShiftScaleRotation& ssr, float imageWidth, float imageHeight)
{
    auto transform = Matrix4::getIdentity();

    // Call the Q3 texture matrix calculation code as used in GtkRadiant
    double STfromXYZ[2][4];
    quake3::getTextureVectorsForFace(normal, ssr, imageWidth, imageHeight, STfromXYZ);

    transform.xx() = STfromXYZ[0][0];
    transform.yx() = STfromXYZ[0][1];
    transform.zx() = STfromXYZ[0][2];

    transform.xy() = STfromXYZ[1][0];
    transform.yy() = STfromXYZ[1][1];
    transform.zy() = STfromXYZ[1][2];

    transform.tx() = STfromXYZ[0][3];
    transform.ty() = STfromXYZ[1][3];

    return transform;
}

}

}
