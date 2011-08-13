#pragma once

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

namespace shaders
{

// Syntax:
// table <tablename> { [snap] [clamp] { <data>, <data>, ... } }

class TableDefinition
{
private:
	// The block name
	std::string _name;

	// Raw content
	std::string _blockContents;

	// Whether to prevent value interpolation
	bool _snap;

	// Whether to prevent wrapping around at index bounds
	bool _clamp;

	// The actual values of this table
	std::vector<float> _values;

	// Whether we parsed the block contents already
	bool _parsed;

public:
	TableDefinition(const std::string& name, const std::string& blockContents);

	const std::string& getName() const
	{
		return _name;
	}

	// Retrieve a value from this table, respecting the clamp and snap flags
	float getValue(float index);

private:
	void parseDefinition();
};
typedef boost::shared_ptr<TableDefinition> TableDefinitionPtr;

} // namespace
