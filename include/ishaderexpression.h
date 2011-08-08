#pragma once

#include <boost/shared_ptr.hpp>

namespace shaders
{

/**
 * A shader expression is something found in a Doom 3 material declaration, 
 * where things like shader parameters, time and constants can be combined
 * by mathematical operators and table-lookups.  A shader expression can be 
 * a constant number in its simplest form, or a complex formula like this:
 *
 * vertexParm 0 0.1 * sintable[time * 0.3], 0.15 * sintable[time * 0.15]
 * 
 * The above makes vertex parameter 0 a time-dependent value which is evaluated 
 * each frame during rendering.
 *
 * A shader expression can be evaluated which results in a single floating point
 * value. In actual materials the shader expression is linked to a material register
 * where the value is written to after evaluation.
 */
class IShaderExpression
{
public:
	/** 
	 * Retrieve the floating point value of this expression.
	 */
	virtual float getValue() = 0;

	/**
	 * Evaluates the value of this expression, writing any results
	 * into the linked material register.
	 */
	virtual float evaluate() = 0;
};
typedef boost::shared_ptr<IShaderExpression> IShaderExpressionPtr;

} // namespace
