#pragma once

#include "ishaders.h"
#include "math/AABB.h"
#include "math/Plane3.h"
#include "ProcWinding.h"

namespace map
{

class ProcBrush;
typedef boost::weak_ptr<ProcBrush> ProcBrushWeakPtr;

struct ProcFace
{
	std::size_t			planenum;		// serves as index into ProcFile::planes

	MaterialPtr			material;		// the material of this plane
	Vector4				texVec[2];		// the offset value will always be in the 0.0 to 1.0 range

	ProcWinding			winding;		// only clipped to the other sides of the brush
	ProcWinding			visibleHull;	// also clipped to the solid parts of the world

	void calculateTextureVectors(const Plane3& plane, const Matrix4& texMat);
};

#define	PSIDE_FRONT			1
#define	PSIDE_BACK			2
#define	PSIDE_BOTH			(PSIDE_FRONT|PSIDE_BACK)
#define	PSIDE_FACING		4

// A brush structure used during compilation
class ProcBrush
{
public:
	//ProcBrush*			next;
	ProcBrushWeakPtr	original;	// chopped up brushes will reference the originals

	std::size_t			entitynum;			// editor numbering for messages
	std::size_t			brushnum;			// editor numbering for messages

	MaterialPtr			contentShader;	// one face's shader will determine the volume attributes

	int					contents;
	bool				opaque;
	int					outputNumber;		// set when the brush is written to the file list

	AABB				bounds;

	typedef std::vector<ProcFace> ProcFaces;
	ProcFaces			sides;

	// Sets the mins/maxs based on the windings
	// returns false if the brush doesn't enclose a valid volume
	bool bound();

	// returns one of PSIDE_*
	int mostlyOnSide(const Plane3& plane) const;
};
typedef boost::shared_ptr<ProcBrush> ProcBrushPtr;

// ------------------------------------------------------

// WARNING : special case behaviour of atan2(y,x) <-> atan(y/x) might not be the same everywhere when x == 0
// rotation by (0,RotY,RotZ) assigns X to normal
inline void computeAxisBase(const Vector3& normal, Vector3& texS, Vector3& texT)
{
	// do some cleaning
	Vector3 n(
		( fabs(normal[0]) < 1e-6f ) ? 0.0f : normal[0],
		( fabs(normal[1]) < 1e-6f ) ? 0.0f : normal[1],
		( fabs(normal[2]) < 1e-6f ) ? 0.0f : normal[2]
	);

	float RotY = -atan2( n[2], sqrt( n[1] * n[1] + n[0] * n[0]) );
	float RotZ = atan2( n[1], n[0] );

	// rotate (0,1,0) and (0,0,1) to compute texS and texT
	texS[0] = -sin(RotZ);
	texS[1] = cos(RotZ);
	texS[2] = 0;
	// the texT vector is along -Z ( T texture coorinates axis )
	texT[0] = -sin(RotY) * cos(RotZ);
	texT[1] = -sin(RotY) * sin(RotZ);
	texT[2] = -cos(RotY);
}

inline void ProcFace::calculateTextureVectors(const Plane3& plane, const Matrix4& texMat)
{
	Vector3 texX, texY;

	computeAxisBase(plane.normal(), texX, texY);

	texVec[0][0] = texX[0] * texMat.xx() + texY[0] * texMat.yx();
	texVec[0][1] = texX[1] * texMat.xx() + texY[1] * texMat.yx();
	texVec[0][2] = texX[2] * texMat.xx() + texY[2] * texMat.yx();
	texVec[0][3] = texMat.tx() /*+ ( origin * texVec[0].getVector3() )*/; // no origin in DR

	texVec[1][0] = texX[0] * texMat.xy() + texY[0] * texMat.yy();
	texVec[1][1] = texX[1] * texMat.xy() + texY[1] * texMat.yy();
	texVec[1][2] = texX[2] * texMat.xy() + texY[2] * texMat.yy();
	texVec[1][3] = texMat.ty() /*+ ( origin * texVec[1].getVector3() )*/; // no origin in DR;
}

inline bool ProcBrush::bound()
{
	bounds = AABB();

	for (std::size_t i = 0; i < sides.size(); ++i)
	{
		const ProcWinding& winding = sides[i].winding;

		for (std::size_t j = 0; j < winding.size(); ++j)
		{
			bounds.includePoint(winding[j].vertex);
		}
	}

	Vector3 corner1 = bounds.origin + bounds.extents;

	if (corner1[0] < MIN_WORLD_COORD || corner1[1] < MIN_WORLD_COORD || corner1[2] < MIN_WORLD_COORD)
	{
		return false;
	}

	Vector3 corner2 = bounds.origin - bounds.extents;

	if (corner2[0] > MAX_WORLD_COORD || corner2[1] > MAX_WORLD_COORD || corner2[2] > MAX_WORLD_COORD)
	{
		return false;
	}

	return true;
}

inline int ProcBrush::mostlyOnSide(const Plane3& plane) const
{
	float max = 0;
	int side = PSIDE_FRONT;

	for (std::size_t i = 0; i < sides.size(); ++i)
	{
		const ProcWinding& w = sides[i].winding;

		for (std::size_t j = 0; j < w.size(); ++j)
		{
			float d = plane.distanceToPoint(w[j].vertex);

			if (d > max)
			{
				max = d;
				side = PSIDE_FRONT;
			}

			if (-d > max)
			{
				max = -d;
				side = PSIDE_BACK;
			}
		}
	}

	return side;
}

} // namespace
