#pragma once

#include <boost/shared_ptr.hpp>

class IRenderEntity;

namespace shaders
{

// Material registers, used to store shader expression results
// The first two slots are always reserved for the constants 0 and 1, see enum ReservedRegisters
typedef std::vector<float> Registers;

// The indices to the constants in the registers array
enum ReservedRegisters
{
	REG_ZERO = 0,
	REG_ONE  = 1,
	NUM_RESERVED_REGISTERS,
};

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
	 * Retrieve the floating point value of this expression. DEPRECATED
	 */
	virtual float getValue(std::size_t time) = 0;

	/** 
	 * Retrieve the floating point value of this expression.
	 */
	virtual float getValue(std::size_t time, const IRenderEntity& entity) = 0;

	/**
	 * Evaluates the value of this expression, writing any results
	 * into the linked material register. DEPRECATED
	 */
	virtual float evaluate(std::size_t time) = 0;

	/**
	 * Evaluates the value of this expression, writing any results
	 * into the linked material register.
	 */
	virtual float evaluate(std::size_t time, const IRenderEntity& entity) = 0;

	/**
	 * Link the expression to the given Registers vector.
	 * Calling evaluate() will cause the result to be saved into the register.
	 *
	 * @returns: the register position the result will be written to.
	 */
	virtual std::size_t linkToRegister(Registers& registers) = 0;
};
typedef boost::shared_ptr<IShaderExpression> IShaderExpressionPtr;

} // namespace
