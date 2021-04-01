#ifndef PLANEPOINTS_H_
#define PLANEPOINTS_H_

#include "math/Vector3.h"

/* greebo: Three points in space (Vector3) define a plane, hence PlanePoints
 *
 * Note: should probably be moved into libs/math/
 */

typedef Vector3 PlanePoints[3];

inline void planepts_assign(PlanePoints planepts, const PlanePoints other) {
	planepts[0] = other[0];
	planepts[1] = other[1];
	planepts[2] = other[2];
}

inline void planepts_quantise(PlanePoints planepts, double snap) {
	planepts[0].snap(snap);
	planepts[1].snap(snap);
	planepts[2].snap(snap);
}

#endif /*PLANEPOINTS_H_*/
