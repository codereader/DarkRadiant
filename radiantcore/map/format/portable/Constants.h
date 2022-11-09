#pragma once

//  Shared string constants used throughout the portable map format classes.

namespace map
{

namespace format
{

namespace constants
{

const char* const ATTR_VERSION = "version";
const char* const ATTR_FORMAT = "format";
const char* const ATTR_FORMAT_VALUE = "portable";

const char* const TAG_MAP_LAYERS = "layers";
const char* const TAG_MAP_LAYER = "layer";
const char* const ATTR_MAP_LAYER_ID = "id";
const char* const ATTR_MAP_LAYER_NAME = "name";
const char* const ATTR_MAP_LAYER_PARENT_ID = "parentId";
const char* const ATTR_MAP_LAYER_ACTIVE = "active";
const char* const ATTR_MAP_LAYER_HIDDEN = "hidden";

const char* const TAG_SELECTIONGROUPS = "selectionGroups";
const char* const TAG_SELECTIONGROUP = "selectionGroup";
const char* const ATTR_SELECTIONGROUP_ID = "id";
const char* const ATTR_SELECTIONGROUP_NAME = "name";

const char* const TAG_SELECTIONSETS = "selectionSets";
const char* const TAG_SELECTIONSET = "selectionSet";
const char* const ATTR_SELECTIONSET_ID = "id";
const char* const ATTR_SELECTIONSET_NAME = "name";

const char* const TAG_MAP_PROPERTIES = "properties";
const char* const TAG_MAP_PROPERTY = "property";
const char* const ATTR_MAP_PROPERTY_KEY = "key";
const char* const ATTR_MAP_PROPERTY_VALUE = "value";

const char* const TAG_ENTITY = "entity";
const char* const TAG_ENTITY_PRIMITIVES = "primitives";
const char* const TAG_ENTITY_KEYVALUES = "keyValues";
const char* const TAG_ENTITY_KEYVALUE = "keyValue";
const char* const ATTR_ENTITY_NUMBER = "number";
const char* const ATTR_ENTITY_PROPERTY_KEY = "key";
const char* const ATTR_ENTITY_PROPERTY_VALUE = "value";

const char* const TAG_BRUSH = "brush";
const char* const TAG_FACES = "faces";
const char* const TAG_FACE = "face";
const char* const TAG_FACE_PLANE = "plane";
const char* const TAG_FACE_TEXPROJ = "textureProjection";
const char* const TAG_FACE_MATERIAL = "material";
const char* const TAG_FACE_CONTENTSFLAG = "contentsFlag";
const char* const ATTR_BRUSH_NUMBER = "number";
const char* const ATTR_FACE_PLANE_X = "x";
const char* const ATTR_FACE_PLANE_Y = "y";
const char* const ATTR_FACE_PLANE_Z = "z";
const char* const ATTR_FACE_PLANE_D = "d";
const char* const ATTR_FACE_TEXTPROJ_XX = "xx";
const char* const ATTR_FACE_TEXTPROJ_YX = "yx";
const char* const ATTR_FACE_TEXTPROJ_TX = "tx";
const char* const ATTR_FACE_TEXTPROJ_XY = "xy";
const char* const ATTR_FACE_TEXTPROJ_YY = "yy";
const char* const ATTR_FACE_TEXTPROJ_TY = "ty";
const char* const ATTR_FACE_MATERIAL_NAME = "name";
const char* const ATTR_FACE_CONTENTSFLAG_VALUE = "value";

const char* const TAG_PATCH = "patch";
const char* const TAG_PATCH_MATERIAL = "material";
const char* const TAG_PATCH_CONTROL_VERTICES = "controlVertices";
const char* const TAG_PATCH_CONTROL_VERTEX = "controlVertex";
const char* const ATTR_PATCH_NUMBER = "number";
const char* const ATTR_PATCH_WIDTH = "width";
const char* const ATTR_PATCH_HEIGHT = "height";
const char* const ATTR_PATCH_FIXED_SUBDIV = "fixedSubdivisions";
const char* const ATTR_PATCH_FIXED_SUBDIV_X = "subdivisionsX";
const char* const ATTR_PATCH_FIXED_SUBDIV_Y = "subdivisionsY";
const char* const ATTR_PATCH_MATERIAL_NAME = "name";
const char* const ATTR_PATCH_CONTROL_VERTEX_ROW = "row";
const char* const ATTR_PATCH_CONTROL_VERTEX_COL = "column";
const char* const ATTR_PATCH_CONTROL_VERTEX_X = "x";
const char* const ATTR_PATCH_CONTROL_VERTEX_Y = "y";
const char* const ATTR_PATCH_CONTROL_VERTEX_Z = "z";
const char* const ATTR_PATCH_CONTROL_VERTEX_U = "u";
const char* const ATTR_PATCH_CONTROL_VERTEX_V = "v";

const char* const TAG_OBJECT_LAYERS = "layers";
const char* const TAG_OBJECT_LAYER = "layer";
const char* const ATTR_OBJECT_LAYER_ID = "id";

const char* const TAG_OBJECT_SELECTIONGROUPS = "selectionGroups";
const char* const TAG_OBJECT_SELECTIONGROUP = "selectionGroup";
const char* const ATTR_OBJECT_SELECTIONGROUP_ID = "id";

const char* const TAG_OBJECT_SELECTIONSETS = "selectionSets";
const char* const TAG_OBJECT_SELECTIONSET = "selectionSet";
const char* const ATTR_OBJECT_SELECTIONSET_ID = "id";

const char* const ATTR_VALUE_TRUE = "true";
const char* const ATTR_VALUE_FALSE = "false";

}

}

}
