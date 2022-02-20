
// The view origin as set by the C++ code
uniform vec3    u_view_origin;

// The texture coordinate for this vertex, will be used in the fragment shader
varying vec3    var_cubeMapTexCoord;

void main()
{
    vec3 texcoord = gl_Vertex.xyz - u_view_origin;

    // Rotate the skybox 90 degrees about the z axis to match what the TDM engine is displaying
    var_cubeMapTexCoord = vec3(-texcoord.y, texcoord.x, texcoord.z);

    // transform vertex position into homogenous clip-space
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // Pass through vertex colour
    gl_FrontColor = gl_Color;
}
