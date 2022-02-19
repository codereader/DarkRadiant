#version 120

// vertex attributes
attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;
attribute vec3 attr_Tangent;
attribute vec3 attr_Bitangent;
attribute vec3 attr_Normal;

uniform vec3 u_viewOrigin;

varying vec4 var_TexCoord0;

void main()
{
    //var_TexCoord0 = attr_Position - vec4(u_viewOrigin, 1);
    var_TexCoord0 = attr_TexCoord0;

    gl_Position = attr_Position;
    gl_FrontColor = gl_Color; // Pass through vertex colour
}
