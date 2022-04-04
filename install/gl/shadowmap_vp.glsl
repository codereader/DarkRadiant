#version 140

in vec4 attr_Position; // bound to attribute 0 in source, in object space
in vec4 attr_TexCoord; // bound to attribute 8 in source

uniform vec3 u_LightOrigin;     // light origin in world coords
uniform mat4 u_ObjectTransform; // object transform (object2world)

// The two top-rows of the diffuse stage texture transformation matrix
uniform vec4 u_DiffuseTextureMatrix[2];

// The final diffuse texture coordinate at this vertex
varying vec2 var_TexDiffuse;

// The modelview matrices for each of the 6 cube map faces
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

// These 4 clip planes are 45 degrees planes passing the origin (opening towards negative z)
// bounding the view frustum of a point light at 0,0,0
const float clipEps = 0e-2;
const vec4 ClipPlanes[4] = vec4[4]
(
    vec4(1, 0, -1, clipEps),
    vec4(-1, 0, -1, clipEps),
    vec4(0, 1, -1, clipEps),
    vec4(0, -1, -1, clipEps)
);

void main()
{
    // Transform the model vertex to world space, then subtract the light origin
    // to move the vertex into light space (with the light residing at 0,0,0)
    vec4 lightSpacePos = u_ObjectTransform * attr_Position;
    lightSpacePos.xyz -= u_LightOrigin;

    // Render the vertex 6 times, once for each cubemap face (gl_InstanceID = [0..5])
    // This is just a rotation, no scaling or projection involved
    vec4 fragPos = vec4(cubicTransformations[gl_InstanceID] * lightSpacePos.xyz, 1);

    gl_Position.x = fragPos.x / 6 + fragPos.z * 5/6 - fragPos.z / 3 * gl_InstanceID;
    gl_Position.y = fragPos.y;
    gl_Position.z = -fragPos.z - 2;
    gl_Position.w = -fragPos.z;

    // To clip the vertices outside the view frustum, calculate its clip distance
    // every fragment with a distance < 0 will be discarded
    // This relies on the GL_CLIP_DISTANCE0-3 flags activated in the renderer code
    gl_ClipDistance[0] = dot(fragPos, ClipPlanes[0]);
    gl_ClipDistance[1] = dot(fragPos, ClipPlanes[1]);
    gl_ClipDistance[2] = dot(fragPos, ClipPlanes[2]);
    gl_ClipDistance[3] = dot(fragPos, ClipPlanes[3]);

    // Apply the texture matrix, we need the tex coords for alpha-testing
    var_TexDiffuse.x = dot(u_DiffuseTextureMatrix[0], attr_TexCoord);
    var_TexDiffuse.y = dot(u_DiffuseTextureMatrix[1], attr_TexCoord);
}
