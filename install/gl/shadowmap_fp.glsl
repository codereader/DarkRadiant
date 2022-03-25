#version 140

uniform sampler2D   u_Diffuse;
uniform float       u_AlphaTest;

// The final diffuse texture coordinate at this vertex, calculated in the vertex shader
varying vec2 var_TexDiffuse;

void main()
{
    if (u_AlphaTest >= 0 && texture2D(u_Diffuse, var_TexDiffuse).a <= u_AlphaTest)
    {
        discard;
    }
}
