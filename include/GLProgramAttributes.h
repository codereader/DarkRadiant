#ifndef GLPROGRAMATTRIBUTES_H_
#define GLPROGRAMATTRIBUTES_H_

/**
 * Enumeration for vertex attributes to be bound to a GLProgram, to avoid using
 * magic numbers.
 */
enum GLProgramAttributes {
	ATTR_TEXCOORD = 8,
	ATTR_TANGENT = 9,
	ATTR_BITANGENT = 10,
	ATTR_NORMAL = 11
};

#endif /*GLPROGRAMATTRIBUTES_H_*/
