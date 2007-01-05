#ifndef MAPEXPRESSION_H_
#define MAPEXPRESSION_H_

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

/* FORWARD DECLS */
namespace parser { class DefTokeniser; }

namespace shaders
{

/**
 * MapExpression shared pointer type.
 */
class MapExpression;
typedef boost::shared_ptr<MapExpression> MapExpressionPtr;

/**
 * Representation of a map expression in a Doom 3 material decl. Most map
 * expressions are a simple texture name, but they may make use of multiply-
 * recursive invocations of addnormals(), heightmap() and other modification
 * functions. For this reason the map expression must be parsed recursively
 * and stored in a tree; MapExpression is the representation of a node in this
 * tree.
 * 
 * A MapExpression is constructed by passing a DefTokeniser, whose next token
 * must be the first token in the map expression (the introductory keyword --
 * "map", "diffusemap", "bumpmap" or "specularmap" -- must already have been
 * consumed by the calling function). The MapExpression will read as many
 * tokens as necessary to construct a well-formed expression tree.
 * 
 * For simplicity, a MapExpression can also represent a constant float value,
 * for functions which take one (such as "heightmap(<map>, <float>"). This 
 * allows the MapExpression to hold a list of MapExpressions as children, 
 * rather than needing a separate "FloatExpression" or other class.
 */
class MapExpression
{
	// Type of this expression node
	enum {
		LEAF,				// leafnode (either texture or float)
		ADD,				// add(map, map)
		SCALE,				// scale(map, float [,float] [,float] [,float] )
		ADDNORMALS,			// addnormals(map, map)
		HEIGHTMAP,			// heightmap(map, float)
		INVERTALPHA,		// invertalpha(map)
		INVERTCOLOR,		// invertcolor(map)
		MAKEINTENSITY,		// makeintensity(map)
		MAKEALPHA			// makealpha(map)
	} _nodeType;
		
	// Vector of child nodes (N-tree)
	typedef std::vector<MapExpression> MapExpressionList; 
	MapExpressionList _children;
	
	// String value of this node, if there is one (image or float)
	std::string _value;
	
private:
	
	/*
	 * Construct a MapExpression by parsing tokens from the provided 
	 * DefTokeniser.
	 */
	MapExpression(parser::DefTokeniser&);

	/*
	 * Construct an empty MapExpression.
	 */
	MapExpression() {}

public:

	/**
	 * Public named constructor to create a MapExpression and return a shared
	 * pointer to it.
	 */
	static MapExpressionPtr construct(parser::DefTokeniser& tok) {
		return MapExpressionPtr(new MapExpression(tok));	
	}

	/**
	 * Construct a null MapExpression that will evaluate to the empty texture.
	 */
	static MapExpressionPtr constructNull() {
		return MapExpressionPtr(new MapExpression());
	}

	/**
	 * Reduce this MapExpression to a string containing the name of the texture
	 * to use for rendering.
	 * 
	 * TODO: This is a hack to accomodate the current textures system -- in 
	 * reality the MapExpression should flatten to an image, not the name of
	 * a pre-existing image (which almost certainly won't reflect the
	 * expression's result). Accomplishing this will require the textures
	 * system to be upgraded to allow the insertion of newly-generated images
	 * in addition to those loaded from disk.
	 */
	std::string getTextureName() const;

};

}

#endif /*MAPEXPRESSION_H_*/
