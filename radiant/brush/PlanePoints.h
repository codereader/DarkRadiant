#ifndef PLANEPOINTS_H_
#define PLANEPOINTS_H_

#include "math/Vector3.h"

/* greebo: Three points in space (Vector3) define a plane, hence PlanePoints
 * 
 * Note: should probably be moved into libs/math/ 
 */

typedef Vector3 PlanePoints[3];

inline bool planepts_equal(const PlanePoints planepts, const PlanePoints other) {
	return planepts[0] == other[0] && planepts[1] == other[1] && planepts[2] == other[2];
}

inline void planepts_assign(PlanePoints planepts, const PlanePoints other) {
	planepts[0] = other[0];
	planepts[1] = other[1];
	planepts[2] = other[2];
}

inline void planepts_quantise(PlanePoints planepts, double snap) {
	vector3_snap(planepts[0], snap);
	vector3_snap(planepts[1], snap);
	vector3_snap(planepts[2], snap);
}

inline void edge_snap(Vector3& edge, double snap) {
	double scale = ceil(fabs(snap / edge.max()));
	
	if (scale > 0.0f) {
		edge *= scale;
	}
	vector3_snap(edge, snap);
}

inline void planepts_snap(PlanePoints planepts, double snap) {
	Vector3 edge01(planepts[1] - planepts[0]);
	Vector3 edge12(planepts[2] - planepts[1]);
	Vector3 edge20(planepts[0] - planepts[2]);

	double length_squared_01 = edge01.dot(edge01);
	double length_squared_12 = edge12.dot(edge12);
	double length_squared_20 = edge20.dot(edge20);

	vector3_snap(planepts[0], snap);

	if (length_squared_01 < length_squared_12) {
		if (length_squared_12 < length_squared_20) {
			edge_snap(edge01, snap);
			edge_snap(edge12, snap);
			planepts[1] = planepts[0] + edge01;
			planepts[2] = planepts[1] + edge12;
		}
		else {
			edge_snap(edge20, snap);
			edge_snap(edge01, snap);
			planepts[1] = planepts[0] + edge20;
			planepts[2] = planepts[1] + edge01;
		}
	}
	else {
		if (length_squared_01 < length_squared_20) {
			edge_snap(edge01, snap);
			edge_snap(edge12, snap);
			planepts[1] = planepts[0] + edge01;
		}
		else {
			edge_snap(edge12, snap);
			edge_snap(edge20, snap);
			planepts[1] = planepts[0] + edge12;
			planepts[2] = planepts[1] + edge20;
		}
	}
}

#endif /*PLANEPOINTS_H_*/
