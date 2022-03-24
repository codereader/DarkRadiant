#version 140

in vec4 attr_Position; // bound to attribute 0 in source, in object space
in vec4 attr_TexCoord; // bound to attribute 8 in source

uniform vec3 u_LightOrigin;     // light origin in world coords
uniform mat4 u_ObjectTransform; // object transform (object2world)

// The two top-rows of the diffuse stage texture transformation matrix
uniform vec4 u_DiffuseTextureMatrix[2];

// The final diffuse texture coordinate at this vertex
varying vec2 var_TexDiffuse;

const mat3 cubicTransformations[6] = mat3[6]
(
    mat3(0,  0, -1,
         0, -1,  0,
        -1,  0,  0),

    mat3(0,  0,  1,
         0, -1,  0,
         1,  0,  0),

    mat3(1,  0,  0,
         0,  0, -1,
         0,  1,  0),

    mat3(1,  0,  0,
         0,  0,  1,
         0, -1,  0),

    mat3(1,  0,  0,
         0, -1,  0,
         0,  0, -1),

    mat3(-1,  0,  0,
          0, -1,  0,
          0,  0,  1)
);

void main()
{
    // Transform the model vertex to world space, then subtract the light origin
    // to move the vertex into light space (with the light residing at 0,0,0)
    vec4 lightSpacePos = u_ObjectTransform * attr_Position;
    lightSpacePos.xyz -= u_LightOrigin;

    vec4 fragPos = vec4(cubicTransformations[gl_InstanceID] * lightSpacePos.xyz, 1);

    gl_Position.x = fragPos.x / 6 + fragPos.z * 5/6 - fragPos.z / 3 * gl_InstanceID;
    gl_Position.y = fragPos.y;
    gl_Position.z = -fragPos.z - 2;
    gl_Position.w = -fragPos.z;
}
