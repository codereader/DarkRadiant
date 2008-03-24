#ifndef TOKENS_H_
#define TOKENS_H_

namespace map {

const std::string HEADER_SEQUENCE = "DarkRadiant Map Information File Version";
const char* const NODE_TO_LAYER_MAPPING = "NodeToLayerMapping";
const char* const LAYER = "Layer";
const char* const LAYERS = "Layers";
const char* const NODE = "Node";

const char* const DUMMY_BRUSH =
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
