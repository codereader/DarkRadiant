#ifndef MAPEXPRESSION_H_
#define MAPEXPRESSION_H_

#include <string>

#include <boost/shared_ptr.hpp>

#include "parser/DefTokeniser.h"
#include "textures/ImageLoaderManager.h"

using parser::DefTokeniser;

namespace shaders {

class IMapExpression;
typedef boost::shared_ptr<IMapExpression> MapExpressionPtr;

// the base class, with the still accessible createForToken function
class IMapExpression {
public:
	/** Creates the a MapExpression out of the given token. Nested mapexpressions
	 * 	are recursively passed to child classes.
	 */
	static MapExpressionPtr createForToken(DefTokeniser& token);
	static MapExpressionPtr createForString(std::string str);
	
	// These have to be implemented by the subclasses
	virtual ImagePtr getImage() = 0;
	virtual std::string getIdentifier() = 0;
	
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
class HeightMapExpression : public IMapExpression {
	MapExpressionPtr heightMapExp;
	float scale;
public:
	HeightMapExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class AddNormalsExpression : public IMapExpression {
	MapExpressionPtr mapExpOne;
	MapExpressionPtr mapExpTwo;
public:
	AddNormalsExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class SmoothNormalsExpression : public IMapExpression {
	MapExpressionPtr mapExp;
public:
	SmoothNormalsExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class AddExpression : public IMapExpression {
	MapExpressionPtr mapExpOne;
	MapExpressionPtr mapExpTwo;
public:
	AddExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class ScaleExpression : public IMapExpression {
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

class InvertAlphaExpression : public IMapExpression {
	MapExpressionPtr mapExp;
public:
	InvertAlphaExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class InvertColorExpression : public IMapExpression {
	MapExpressionPtr mapExp;
public:
	InvertColorExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class MakeIntensityExpression : public IMapExpression {
	MapExpressionPtr mapExp;
public:
	MakeIntensityExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class MakeAlphaExpression : public IMapExpression {
	MapExpressionPtr mapExp;
public:
	MakeAlphaExpression (DefTokeniser& token);
	ImagePtr getImage();
	std::string getIdentifier();
};

class ImageExpression : public IMapExpression {
	std::string _imgName;
	ImageLoaderList _imageLoaders;
public:
	ImageExpression(std::string imgName);
	ImagePtr getImage();
	std::string getIdentifier();
};

} // namespace shaders

#endif /*MAPEXPRESSION_H_*/
