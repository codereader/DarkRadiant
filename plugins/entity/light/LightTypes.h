#ifndef LIGHTTYPES_H_
#define LIGHTTYPES_H_

// greebo: the Light types.
// Note: Either this can be removed entirely or at least moved into a separate namespace

enum LightType {
	LIGHTTYPE_DEFAULT,
	LIGHTTYPE_RTCW,
	LIGHTTYPE_DOOM3
};

extern LightType g_lightType;

#endif /*LIGHTTYPES_H_*/
