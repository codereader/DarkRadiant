#ifndef ParserBrushDef3_h__
#define ParserBrushDef3_h__

#include "imapformat.h"

namespace map
{

class BrushDef3Parser :
	public PrimitiveParser
{
public:
	const std::string& getKeyword() const;

    virtual scene::INodePtr parse(parser::DefTokeniser& tok) const;
};
typedef boost::shared_ptr<BrushDef3Parser> BrushDef3ParserPtr;

// A special brushDef3 parser for Quake 4 maps
class BrushDef3ParserQuake4 :
	public BrushDef3Parser
{
public:
    virtual scene::INodePtr parse(parser::DefTokeniser& tok) const;
};
typedef boost::shared_ptr<BrushDef3ParserQuake4> BrushDef3ParserQuake4Ptr;

}

#endif // ParserBrushDef3_h__
