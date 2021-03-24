#pragma once

#include <tao/pegtl.hpp>

namespace shaders
{

namespace parser
{

using namespace tao;
using namespace tao::pegtl;

struct Nothing : space {};

struct TableKeywordPlusSpace : seq<pegtl::string<'t', 'a', 'b', 'l', 'e'>, space> {};

struct DeclName : plus<alnum, plus<sor<alnum, one<'/', '_'>>>> {};

struct LineComment : until<eolf> {};
struct BlockComment : until<pegtl::string<'*', '/'>> {};

struct Comment : if_must<pegtl::string<'/', '/'>, LineComment> {};
struct CommentBlock : if_must<pegtl::string<'/', '*'>, BlockComment> {};

struct BlockStart : one<'{'> {};
struct BlockEnd : one<'}'> {};

struct BlockContents : not_one<'{', '}'> {};

struct BlockDefinition : seq<BlockStart, star<sor<BlockContents, BlockDefinition>>, BlockEnd> {};

struct MaterialBlockDefinition : seq<BlockStart, star<sor<BlockContents, BlockDefinition>>, BlockEnd> {};

struct MaterialDeclaration : if_must<DeclName, star<sor<space, Comment, CommentBlock>>, MaterialBlockDefinition> {};

struct TableDeclaration : if_must<TableKeywordPlusSpace, DeclName, star<sor<space, Comment, CommentBlock>>, BlockDefinition> {};

// Any possible element of a material file (Tables, Materials, Comments and plain whitespace)
struct Anything : sor<Nothing, TableDeclaration, MaterialDeclaration, Comment, CommentBlock> {};

// A Material has no required elements, it can include anything (including nothing)
struct MaterialFile : until<eof, Anything> {};

template<typename Rule> struct MaterialFileSelector : std::true_type {};

}

}
