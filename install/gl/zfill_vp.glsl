#version 120

in vec4 attr_Position; // bound to attribute 0 in source, in object space
in vec4 attr_TexCoord; // bound to attribute 8 in source

uniform mat4 u_objectTransform; // object transform (object2world)

varying vec2 var_TexDiffuse;

void main()
{
    // Apply the supplied object transform to the incoming vertex
    // transform vertex position into homogenous clip-space
    gl_Position = gl_ModelViewProjectionMatrix * u_objectTransform * attr_Position;

    // transform texcoords
    var_TexDiffuse = (gl_TextureMatrix[0] * attr_TexCoord).st;

    // assign color
    gl_FrontColor = gl_Color;
}
