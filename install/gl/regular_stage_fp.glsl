#version 130

// Sampler bound to texture unit 0
uniform sampler2D u_Map;

varying vec2 var_TexDiffuse; // texture coordinates
varying vec4 var_Colour; // colour to be multiplied on the final fragment

void main()
{
    // Look up the fragment using the cube map sampler
    gl_FragColor = texture(u_Map, var_TexDiffuse) * var_Colour;
}

