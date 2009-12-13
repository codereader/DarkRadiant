#ifndef MAPEXPRESSION_H_
#define MAPEXPRESSION_H_

#include <string>

#include <boost/shared_ptr.hpp>

#include "NamedBindable.h"
#include "parser/DefTokeniser.h"
#include "textures/ImageLoaderManager.h"

using parser::DefTokeniser;

namespace shaders 
{

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
: public NamedBindable
{
public: /* INTERFACE METHODS */

	/**
     * \brief
     * Construct and return the image created from this map expression.
     */
	virtual ImagePtr getImage() const = 0;

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

public:

    /* BindableTexture interface */
    TexturePtr bindTexture(const std::string& name) const
    {
        ImagePtr img = getImage();
        if (img)
            return getImage()->bindTexture(name);
        else
            return TexturePtr();
    }

public: /* STATIC CONSTRUCTION METHODS */

	/** Creates the a MapExpression out of the given token. Nested mapexpressions
	 * 	are recursively passed to child classes.
	 */
	static MapExpressionPtr createForToken(DefTokeniser& token);
	static MapExpressionPtr createForString(std::string str);

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
	static ImagePtr getResampled(const ImagePtr& input, std::size_t width, std::size_t height);
};

// the specific MapExpressions
class HeightMapExpression : public MapExpression {
	MapExpressionPtr heightMapExp;
	float scale;
public:
	HeightMapExpression (DefTokeniser& token);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

class AddNormalsExpression : public MapExpression {
	MapExpressionPtr mapExpOne;
	MapExpressionPtr mapExpTwo;
public:
	AddNormalsExpression (DefTokeniser& token);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

class SmoothNormalsExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	SmoothNormalsExpression (DefTokeniser& token);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

class AddExpression : public MapExpression {
	MapExpressionPtr mapExpOne;
	MapExpressionPtr mapExpTwo;
public:
	AddExpression (DefTokeniser& token);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

class ScaleExpression : public MapExpression {
	MapExpressionPtr mapExp;
	float scaleRed;
	float scaleGreen;
	float scaleBlue;
	float scaleAlpha;
public:
	ScaleExpression (DefTokeniser& token);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

class InvertAlphaExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	InvertAlphaExpression (DefTokeniser& token);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

class InvertColorExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	InvertColorExpression (DefTokeniser& token);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

class MakeIntensityExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	MakeIntensityExpression (DefTokeniser& token);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

class MakeAlphaExpression : public MapExpression {
	MapExpressionPtr mapExp;
public:
	MakeAlphaExpression (DefTokeniser& token);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

/**
 * \brief
 * MapExpression consisting of a single image name. 
 *
 * This is the base for all map expressions.
 */
class ImageExpression 
: public MapExpression 
{
	std::string _imgName;

public:

    /* MapExpression interface */
	ImageExpression(const std::string& imgName);
	ImagePtr getImage() const;
	std::string getIdentifier() const;
};

} // namespace shaders

#endif /*MAPEXPRESSION_H_*/
