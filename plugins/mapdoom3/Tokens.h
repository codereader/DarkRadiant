#ifndef TOKENS_H_
#define TOKENS_H_

#include <string>

namespace map {

// InfoFile tokens --------------------------------------------------
const std::string HEADER_SEQUENCE = "DarkRadiant Map Information File Version";
const std::string NODE_TO_LAYER_MAPPING = "NodeToLayerMapping";
const std::string LAYER = "Layer";
const std::string LAYERS = "Layers";
const std::string NODE = "Node";

// Doom 3 Map File Tokens --------------------------------------------------
const std::string VERSION = "Version";

const std::string DUMMY_BRUSH =
	"// dummy brush 0\n\
	{\n\
	brushDef3\n\
	{\n\
	( 0 0 -1 0 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
	( 0 0 1 -8 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
	( 0 -1 0 -16 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
	( 1 0 0 -16 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
	( 0 1 0 -16 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
	( -1 0 0 -16 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
	}\n\
	}\n";

} // namespace map

#endif /* TOKENS_H_ */
