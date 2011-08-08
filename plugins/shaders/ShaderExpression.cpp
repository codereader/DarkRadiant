#include "ShaderExpression.h"

#include "itextstream.h"

#include "string/string.h"
#include <boost/algorithm/string/predicate.hpp>

namespace shaders
{

namespace
{
	const int MAX_SHADERPARM_INDEX = 11;
}

IShaderExpressionPtr ShaderExpression::createFromTokens(parser::StringTokeniser& tokeniser)
{
	std::string token = tokeniser.nextToken();

	// The first expression
	IShaderExpressionPtr first;

	if (token == "(")
	{
		// A paranthesis always suggests a new token, enter recursion to retrieve the first
		first = createFromTokens(tokeniser);
	}                                                                                        
	else if (boost::algorithm::istarts_with(token, "parm"))
	{
		// This is a shaderparm, get the number
		int shaderParmNum = strToInt(token.substr(4));

		if (shaderParmNum >= 0 && shaderParmNum <= MAX_SHADERPARM_INDEX)
		{
			first.reset(new ShaderParmExpression(shaderParmNum));
		}
		else
		{
			throw new parser::ParseException("Shaderparm index out of bounds");
		}
	}
	else if (token == "time")
	{
		first.reset(new TimeExpression);
	}
	else
	{
		// TODO Check if this keyword is a material lookup table

		// Attempt to convert the token into a floating point value
		try
		{
			float value = boost::lexical_cast<float>(token);
			first.reset(new ConstantExpression(value));
		}
		catch (boost::bad_lexical_cast& ex)
		{
			throw new parser::ParseException(
				"Expected time, parm<N> , table name or floating point value, found: " +
				token + " (" + ex.what() + ")");
		}
	}

	// The first expression has been parsed, are there any more tokens?
	while (tokeniser.hasMoreTokens())
	{
		// TODO
		tokeniser.nextToken();
	}

	return IShaderExpressionPtr();
}

IShaderExpressionPtr ShaderExpression::createFromString(const std::string& exprStr)
{
	parser::BasicStringTokeniser tokeniser(exprStr);

	try
	{
		return createFromTokens(tokeniser);
	}
	catch (parser::ParseException& ex)
	{
		globalWarningStream() << "[shaders] " << ex.what() << std::endl;
		return IShaderExpressionPtr();
	}
}

} // namespace
