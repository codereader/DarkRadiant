#include "TableDefinition.h"

#include "itextstream.h"
#include "parser/DefTokeniser.h"

#include <cmath>
#include <boost/lexical_cast.hpp>

namespace shaders
{

TableDefinition::TableDefinition(const std::string& name, 
								 const std::string& blockContents) :
	_name(name),
	_blockContents(blockContents),
	_snap(false),
	_clamp(false),
	_parsed(false)
{}

float TableDefinition::getValue(float index)
{
	if (!_parsed) parseDefinition();

	// Don't bother if we don't have any values to look up
	if (_values.empty())
	{
		return 0.0f;
	}

	// Ensure that index is within bounds if clamp is active
	if (_clamp)
	{
		if (index < 0) 
		{
			index = 0;
		}

		if (index > _values.size())
		{
			index = _values.size() - 1;
		}
	}
	else if (index > _values.size() - 1)
	{
		// No clamping, wrap around the bounds
		index = std::fmod(index, _values.size() - 1);
	}

	// Calculate the index into our std::vector
	std::size_t lookupIndex = static_cast<std::size_t>(std::floor(index + 0.5f));

	if (_snap)
	{
		// Snap is activated, round to the nearest integer
		return _values[lookupIndex];
	}

	// No snapping, interpolate between this and the next value
	float fraction = index - lookupIndex;
	std::size_t nextIndex = (lookupIndex + 1) % _values.size();
	
	return (1-fraction)*_values[lookupIndex] + fraction*_values[nextIndex];
}

void TableDefinition::parseDefinition()
{
	// consider ourselves parsed from now on
	_parsed = true; 

	try
	{
		// Use a tokeniser to read the values
		parser::BasicDefTokeniser<std::string> tokeniser(_blockContents, " \n\t\r,");

		std::size_t level = 0;

		while (tokeniser.hasMoreTokens())
		{
			std::string token = tokeniser.nextToken();

			if (token == "{")
			{
				++level;

				if (level > 1)
				{
					throw parser::ParseException("Too many opening braces.");
				}
			}
			else if (token == "}")
			{
				if (level == 0)
				{
					throw parser::ParseException("Too many closing braces.");
				}

				--level;
			}
			else if (token == "clamp")
			{
				if (level != 0)
				{
					throw parser::ParseException("The 'clamp' keyword cannot be used at this scope/position.");
				}

				_clamp = true;
			}
			else if (token == "snap")
			{
				if (level != 0)
				{
					throw parser::ParseException("The 'snap' keyword cannot be used at this scope/position.");
				}

				_snap = true;
			}
			else
			{
				// Expect a numeric value at this point
				try
				{
					_values.push_back(boost::lexical_cast<float>(token));
				}
				catch (boost::bad_lexical_cast& ex)
				{
					throw parser::ParseException("Invalid token '" + token + 
						"' encountered: " + ex.what());
				}
			}
		}
	}
	catch (parser::ParseException& ex)
	{
		globalErrorStream() << "[shaders] Error parsing table '" << _name << "': " << ex.what() << std::endl;
	}
}

} // namespace
