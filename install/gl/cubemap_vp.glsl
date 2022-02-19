
// The view origin as set by the C++ code
uniform vec3    u_view_origin;

// The texture coordinate for this vertex, will be used in the fragment shader
varying vec3    var_cubeMapTexCoord;

void main()
{
    var_cubeMapTexCoord = gl_Vertex.xyz - u_view_origin;

    // transform vertex position into homogenous clip-space
    gl_Position = ftransform();

    // Pass through vertex colour
    gl_FrontColor = gl_Color;
}
