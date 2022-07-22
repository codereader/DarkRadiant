#include "TableDefinition.h"

#include "parser/DefTokeniser.h"
#include <cmath>

namespace shaders
{

TableDefinition::TableDefinition(const std::string& name) :
    DeclarationBase<ITableDefinition>(decl::Type::Table, name),
	_snap(false),
	_clamp(false)
{}

float TableDefinition::getValue(float index)
{
    ensureParsed();

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
			return _values[numValues - 1];
		}
		else if (index < 0.0f) 
		{
			return _values[0];
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

    auto leftIdx = static_cast<std::size_t>(std::floor(index)) % numValues;

	if (_snap)
	{
	    // If snap is active, just use the left-bound index
		return _values[leftIdx];
	}

	// No snapping, pick the next value to the right to interpolate
	auto rightIdx = (leftIdx + 1) % numValues;

	float fraction = index - leftIdx;

	return (1-fraction)*_values[leftIdx] + fraction*_values[rightIdx];
}

void TableDefinition::onBeginParsing()
{
    _snap = false;
    _clamp = false;
    _values.clear();
}

void TableDefinition::parseFromTokens(parser::DefTokeniser& tokeniser)
{
	std::size_t level = 0;

	while (tokeniser.hasMoreTokens())
	{
		std::string token = tokeniser.nextToken();

		if (token == "{")
		{
			if (++level > 1)
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
				throw parser::ParseException("Invalid token '" + token + "' encountered: " + ex.what());
			}
		}
	}
}

} // namespace
