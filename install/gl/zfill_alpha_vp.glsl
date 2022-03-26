#version 140

in vec4 attr_Position; // bound to attribute 0 in source, in object space
in vec4 attr_TexCoord; // bound to attribute 8 in source

uniform mat4 u_ModelViewProjection; // combined modelview and projection matrix
uniform mat4 u_ObjectTransform; // object transform (object2world)

// The two top-rows of the diffuse stage texture transformation matrix
uniform vec4 u_DiffuseTextureMatrix[2];

// The final diffuse texture coordinate at this vertex
varying vec2 var_TexDiffuse;

void main()
{
    // Apply the supplied object transform to the incoming vertex
    // transform vertex position into homogenous clip-space
    gl_Position = u_ModelViewProjection * u_ObjectTransform * attr_Position;

    // Apply the stage texture transform to the incoming tex coord, component wise
    var_TexDiffuse.x = dot(u_DiffuseTextureMatrix[0], attr_TexCoord);
    var_TexDiffuse.y = dot(u_DiffuseTextureMatrix[1], attr_TexCoord);
}
