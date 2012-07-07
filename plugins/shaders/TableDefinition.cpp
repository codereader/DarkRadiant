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

	if (_values.size() == 1)
	{
		return _values[0];
	}

	if (_clamp)
	{
		if (index > 1.0f) 
		{
			index = 1.0f - 1.0f / _values.size();
		}
		else if (index < 0.0f) 
		{
			index = 0.0f;
		}

		// Map the index to the [0..N-1] interval
		index *= _values.size() - 1;
	}
	else
	{
		// Only take the fractional part of the index
		index = std::fmod(index, 1.0f);

		// Map the index to the [0..N] interval
		index *= _values.size();
	}

	// If snap is active, round the values to the nearest integer
	if (_snap)
	{
		index = std::floor(index + 0.5f);

		return _values[static_cast<std::size_t>(index) % _values.size()];
	}
	else
	{
		// No snapping, pick the interpolation values
		std::size_t leftIdx = static_cast<std::size_t>(std::floor(index)) % _values.size();
		std::size_t rightIdx = (leftIdx + 1) % _values.size();

		float fraction = index - leftIdx;

		return (1-fraction)*_values[leftIdx] + fraction*_values[rightIdx];
	}
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
		rError() << "[shaders] Error parsing table '" << _name << "': " << ex.what() << std::endl;
	}
}

} // namespace
