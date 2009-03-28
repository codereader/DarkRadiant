#ifndef MAPEXPRESSION_H_
#define MAPEXPRESSION_H_

#include <string>

#include <boost/shared_ptr.hpp>

#include "parser/DefTokeniser.h"
#include "textures/ImageLoaderManager.h"

using parser::DefTokeniser;

namespace shaders {

class MapExpression;
typedef boost::shared_ptr<MapExpression> MapExpressionPtr;

/**
 * \brief
 * Abstract base class for map expressions.
 *
 * Map expression are recursive expressions that generate an image, such as
 * "heightmap(addnormals(blah, bleh), 1).
 */
class MapExpression 
{
public: /* TYPES */

    /**
     * \brief
     * Vector of ImagePtrs.
     */
    typedef std::vector<ImagePtr> ImageVector;

public: /* INTERFACE METHODS */

	/**
     * \brief
     * Construct and return the image created from this map expression.
     */
	virtual ImagePtr getImage() = 0;

    /**
     * \brief
     * Return a text string that uniquely identifies this map expression.
     *
     * Each map expression must generate a unique string that identifies it, so
     * that it can be cached for performance by the GL texture manager.
     */
	virtual std::string getIdentifier() = 0;

    /**
     * \brief
     * Return whether this map expression creates a cube map.
     *
     * \return
     * true if this map expression creates a cube map, false if it is a single
     * image.
     */
    virtual bool isCubeMap() const
    {
        return false;
    }

    /**
     * \brief
     * Return the vector of six cube map images for this cube map expression.
     *
     * If this is not a cube map expression, the vector will be empty.
     */
    virtual ImageVector getCubeMapImages() const
    {
        return ImageVector();
    }
	
public: /* STATIC CONSTRUCTION METHODS */

	/** Creates the a MapExpression out of the given token. Nested mapexpressions
	 * 	are recursively passed to child classes.
	 */
	static MapExpressionPtr createForToken(DefTokeniser& token);
	static MapExpressionPtr createForString(std::string str);

    /**
     * \brief
     * Create a cube map expression from the given texture path, e.g.
     * "env/skyboxes/skybox1".
     */
    static MapExpressionPtr createCubeMapForString(const std::string& str);
	
protected:
	/** greebo: Assures that the image is matching the desired dimensions.
	 * 
	 * @input: The image to be rescaled. If it doesn't match <width x height>
	 * 			it is rescaled and the resampled image is returned.
	 * 			
	 * Note: The input image is removed from the heap after resampling
	 * 		 and a new one is allocated and returned.
	 *  
	 * @returns: the resampled image, this might as well be input.  
	 */
	virtual ImagePtr getResampled(ImagePtr input, unsigned int width, unsigned int height);
};

// the specific MapExpressions
class HeightMapExpression : public MapExpression {
	MapExpressionPtr heightMapExp;
	float scale;
public:
	HeightMapExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class AddNormalsExpression : public MapExpression {
	MapExpressionPtr mapExpOne;
	MapExpressionPtr mapExpTwo;
public:
	AddNormalsExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class SmoothNormalsExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	SmoothNormalsExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class AddExpression : public MapExpression {
	MapExpressionPtr mapExpOne;
	MapExpressionPtr mapExpTwo;
public:
	AddExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class ScaleExpression : public MapExpression {
	MapExpressionPtr mapExp;
	float scaleRed;
	float scaleGreen;
	float scaleBlue;
	float scaleAlpha;
public:
	ScaleExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class InvertAlphaExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	InvertAlphaExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class InvertColorExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	InvertColorExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class MakeIntensityExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	MakeIntensityExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class MakeAlphaExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	MakeAlphaExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class ImageExpression : public MapExpression {
	std::string _imgName;
	ImageLoaderList _imageLoaders;
public:
	ImageExpression(std::string imgName);
	ImagePtr getImage();
	std::string getIdentifier();
};

/**
 * \brief
 * Map expression that creates a cube map out of six images in a directory.
 */
class CubeMapExpression
: public MapExpression
{
    // The source prefix. The actual images will be (_prefix + "_up") etc.
    std::string _prefix;

public:

    // Constructor
    CubeMapExpression(const std::string& prefix)
    : _prefix(prefix)
    { }

    /* MapExpression interface */
    ImagePtr getImage();
    std::string getIdentifier();
    bool isCubeMap() const;
    ImageVector getCubeMapImages() const;
};

} // namespace shaders

#endif /*MAPEXPRESSION_H_*/
