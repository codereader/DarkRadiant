#pragma once

#include <vector>
#include <string>
#include <memory>
#include "ishaders.h"
#include "decl/DeclarationBase.h"

namespace shaders
{

// Syntax:
// table <tablename> { [snap] [clamp] { <data>, <data>, ... } }

class TableDefinition :
    public decl::DeclarationBase<ITableDefinition>
{
private:
	// Whether to prevent value interpolation
	bool _snap;

	// Whether to prevent wrapping around at index bounds
	bool _clamp;

	// The actual values of this table
	std::vector<float> _values;

public:
	TableDefinition(const std::string& name);

	// Retrieve a value from this table, respecting the clamp and snap flags
	float getValue(float index) override;

protected:
    void onBeginParsing() override;
    void parseFromTokens(parser::DefTokeniser& tokeniser) override;
};

} // namespace
