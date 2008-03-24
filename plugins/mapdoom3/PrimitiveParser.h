#ifndef PRIMITIVE_PARSER_H_
#define PRIMITIVE_PARSER_H_

#include "parser/DefTokeniser.h"
#include "inode.h"

namespace map {

class PrimitiveParser
{
public:
	/**
	 * Creates and returns a primitive node according to the encountered token.
	 */
    virtual scene::INodePtr parsePrimitive(parser::DefTokeniser& tok) const = 0;
};

} // namespace map

#endif /* PRIMITIVE_PARSER_H_ */
