#version 130

in vec4 attr_Position;  // bound to attribute 0 in source, in object space
in vec4 attr_TexCoord;  // bound to attribute 8 in source
in vec4 attr_Tangent;   // bound to attribute 9 in source
in vec4 attr_Bitangent; // bound to attribute 10 in source
in vec4 attr_Normal;    // bound to attribute 11 in source
in vec4 attr_Colour;    // bound to attribute 12 in source

uniform vec4 u_DiffuseTextureMatrix[2];
uniform vec4 u_ColourModulation;    // vertex colour weight
uniform vec4 u_ColourAddition;      // constant additive vertex colour value
uniform mat4 u_ModelViewProjection; // combined modelview and projection matrix
uniform mat4 u_ObjectTransform;     // object transform (object2world)

varying vec2 var_TexDiffuse; // texture coordinates
varying vec4 var_Colour; // colour to be multiplied on the final fragment

void main()
{
    // transform vertex position into homogenous clip-space
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // Apply the texture matrix to get the texture coords for this vertex
    var_TexDiffuse.x = dot(u_DiffuseTextureMatrix[0], attr_TexCoord);
    var_TexDiffuse.y = dot(u_DiffuseTextureMatrix[1], attr_TexCoord);

    // Vertex colour factor
    var_Colour = (attr_Colour * u_ColourModulation + u_ColourAddition);
}
