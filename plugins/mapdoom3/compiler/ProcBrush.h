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
	//textureVectors_t	texVec;			// FIXME

	ProcWinding			winding;		// only clipped to the other sides of the brush
	ProcWinding			visibleHull;	// also clipped to the solid parts of the world
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
