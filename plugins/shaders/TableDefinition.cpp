#include "TableDefinition.h"

#include "parser/DefTokeniser.h"

namespace shaders
{

TableDefinition::TableDefinition(const std::string& name, 
								 const std::string& blockContents) :
	_name(name),
	_blockContents(blockContents),
	_parsed(false)
{}

float TableDefinition::getValue(float index)
{
	if (!_parsed) parseDefinition();

	return 0.0f; // TODO
}

void TableDefinition::parseDefinition()
{
	// TODO
}

} // namespace
