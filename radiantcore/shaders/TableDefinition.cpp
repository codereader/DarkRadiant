#include "TableDefinition.h"

#include "itextstream.h"
#include "parser/DefTokeniser.h"

#include <cmath>

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

    auto numValues = _values.size();

	if (numValues == 1)
	{
		return _values[0];
	}

	if (_clamp)
	{
		if (index > 1.0f) 
		{
			index = 1.0f - 1.0f / numValues;
		}
		else if (index < 0.0f) 
		{
			index = 0.0f;
		}

		// Map the index to the [0..N-1] interval
		index *= numValues - 1;
	}
	else
	{
		// Only take the fractional part of the index
		index = std::fmod(index, 1.0f);

        // Mirror negative indices to the positive range (catch negative -0.0f)
        if (index < 0 && index != 0.0f)
        {
            index += 1.0f;
        }

		// Map the index to the [0..N) interval
		index *= numValues;
	}

	// If snap is active, round the values to the nearest integer
	if (_snap)
	{
		index = std::floor(index + 0.5f);

		return _values[static_cast<std::size_t>(index) % numValues];
	}

	// No snapping, pick the interpolation values
	auto leftIdx = static_cast<std::size_t>(std::floor(index)) % numValues;
	auto rightIdx = (leftIdx + 1) % numValues;

	float fraction = index - leftIdx;

	return (1-fraction)*_values[leftIdx] + fraction*_values[rightIdx];
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
					_values.push_back(std::stof(token));
				}
				catch (std::invalid_argument& ex)
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
