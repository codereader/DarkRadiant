#ifndef ParserBrushDef_h__
#define ParserBrushDef_h__

#include "imapformat.h"

namespace map
{

// A primitive parser for the "old" brushDef format
class BrushDefParser :
	public PrimitiveParser
{
public:
	const std::string& getKeyword() const;

    scene::INodePtr parse(parser::DefTokeniser& tok) const;
};
typedef boost::shared_ptr<BrushDefParser> BrushDefParserPtr;

}

#endif // ParserBrushDef_h__
