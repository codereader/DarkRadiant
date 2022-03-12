#version 130

// Sampler bound to texture unit 0
uniform samplerCube u_cubemap;

// The view origin as set by the C++ code
uniform vec3    u_view_origin;

// The texture coordinate as calculated in the vertex program
varying vec3    var_cubeMapTexCoord;

void main()
{
    // Swap Y and Z coordinates
    vec3 texcoord = vec3(var_cubeMapTexCoord.x, var_cubeMapTexCoord.z, var_cubeMapTexCoord.y);

    // Look up the fragment using the cube map sampler
    gl_FragColor = texture(u_cubemap, texcoord);
}

