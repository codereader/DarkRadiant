#version 130

in vec4 attr_Position;  // bound to attribute 0 in source, in object space

uniform mat4 u_ModelViewProjection; // combined modelview and projection matrix
uniform mat4 u_ObjectTransform;     // object transform (object2world)
uniform mat4 u_LightTextureMatrix;  // light texture transform (world-to-light-UV)

varying vec4 var_tex_atten_xy_z;

void main()
{
    vec4 worldVertex = u_ObjectTransform * attr_Position;

    // calc light xy,z attenuation in light space
    var_tex_atten_xy_z = u_LightTextureMatrix * worldVertex;

    // Apply the supplied object transform to the incoming vertex
    // transform vertex position into homogenous clip-space
    gl_Position = u_ModelViewProjection * worldVertex;
}
