#version 130

uniform sampler2D	u_Diffusemap;
uniform sampler2D	u_Bumpmap;
uniform sampler2D	u_Specularmap;
uniform sampler2D	u_attenuationmap_xy;
uniform sampler2D	u_attenuationmap_z;
uniform sampler2D	u_ShadowMap;

uniform vec3    u_LocalViewOrigin;
uniform vec3    u_WorldUpLocal; // world 0,0,1 direction in local space
uniform vec3    u_LocalLightOrigin; // Light origin in local space
uniform vec3    u_LightColour;  // the RGB colour as defined on the light entity
uniform float   u_LightScale;
uniform vec4    u_ColourModulation;
uniform vec4    u_ColourAddition;
uniform mat4    u_ObjectTransform;     // object to world

// Defines the region within the shadow map atlas containing the depth information of the current light
uniform vec4        u_ShadowMapRect; // x,y,w,h
uniform bool        u_UseShadowMap;

// Activate ambient light mode (brightness unaffected by direction)
uniform bool u_IsAmbientLight;

// Texture coords as calculated by the vertex program
varying vec2 var_TexDiffuse;
varying vec2 var_TexBump;
varying vec2 var_TexSpecular;

varying vec3 var_vertex; // in world space
varying vec4 var_tex_atten_xy_z;
varying mat3 var_mat_os2ts;
varying vec4 var_Colour; // colour to be multiplied on the final fragment
varying vec3 var_WorldLightDirection; // direction the light is coming from in world space
varying vec3 var_LocalViewerDirection; // viewer direction in local space

// Function ported from TDM tdm_shadowmaps.glsl, determining the cube map face for the given direction
vec3 CubeMapDirectionToUv(vec3 v, out int faceIdx)
{
    vec3 v1 = abs(v);

    float maxV = max(v1.x, max(v1.y, v1.z));

    faceIdx = 0;
    if(maxV == v.x)
    {
        v1 = -v.zyx;
    }
    else if(maxV == -v.x)
    {
        v1 = v.zyx * vec3(1, -1, 1);
        faceIdx = 1;
    }
    else if(maxV == v.y)
    {
        v1 = v.xzy * vec3(1, 1, -1);
        faceIdx = 2;
    }
    else if(maxV == -v.y)
    {
       v1 = v.xzy * vec3(1, -1, 1);
       faceIdx = 3;
    }
    else if(maxV == v.z)
    {
        v1 = v.xyz * vec3(1, -1, -1);
        faceIdx = 4;
    }
    else //if(maxV == -v.z) {
    {
        v1 = v.xyz * vec3(-1, -1, 1);
        faceIdx = 5;
    }

    v1.xy /= -v1.z;
    return v1;
}

// Function ported from TDM tdm_shadowmaps.glsl, picking the depth value from the shadow map
float getDepthValueForVector(in sampler2D shadowMapTexture, vec4 shadowRect, vec3 lightVec)
{
    // Determine face index and cube map sampling vector
    int faceIdx;
    vec3 v1 = CubeMapDirectionToUv(lightVec, faceIdx);

    vec2 texSize = textureSize(shadowMapTexture, 0);
    vec2 shadow2d = (v1.xy * .5 + vec2(.5) ) * shadowRect.ww + shadowRect.xy;
    shadow2d.x += (shadowRect.w + 1./texSize.x) * faceIdx;

    float d = textureLod(shadowMapTexture, shadow2d, 0).r;
    return 1 / (1 - d);
}

void main()
{
    vec3 totalColor;

    // Perform the texture lookups
    vec4 diffuse = texture2D(u_Diffusemap, var_TexDiffuse);
    vec3 specular = texture2D(u_Specularmap, var_TexSpecular).rgb;
    vec4 bumpTexel = texture2D(u_Bumpmap, var_TexBump) * 2. - 1.;

    // Light texture lookups
    vec3 attenuation_xy = vec3(0,0,0);

    if (var_tex_atten_xy_z.w > 0.0)
    {
        attenuation_xy	= texture2DProj(u_attenuationmap_xy, var_tex_atten_xy_z.xyw).rgb;
    }

    vec3 attenuation_z	= texture2D(u_attenuationmap_z, vec2(var_tex_atten_xy_z.z, 0.5)).rgb;

    if (!u_IsAmbientLight)
    {
        // Ported from TDM interaction.common.fs.glsl
        vec4 fresnelParms = vec4(1.0, .23, .5, 1.0);
        vec4 fresnelParms2 = vec4(.2, .023, 120.0, 4.0);
        vec4 lightParms = vec4(.7, 1.8, 10.0, 30.0);

        // compute view direction in tangent space
        vec3 localV = normalize(var_mat_os2ts * (u_LocalViewOrigin - var_vertex));
    
        // compute light direction in tangent space
        vec3 localL = normalize(var_mat_os2ts * (u_LocalLightOrigin - var_vertex));
    
        vec3 RawN = normalize(bumpTexel.xyz);
        vec3 N = var_mat_os2ts * RawN;
    
        //must be done in tangent space, otherwise smoothing will suffer (see #4958)
        float NdotL = clamp(dot(RawN, localL), 0.0, 1.0);
        float NdotV = clamp(dot(RawN, localV), 0.0, 1.0);
        float NdotH = clamp(dot(RawN, normalize(localV + localL)), 0.0, 1.0);
    
        // fresnel part
        float fresnelTerm = pow(1.0 - NdotV, fresnelParms2.w);
        float rimLight = fresnelTerm * clamp(NdotL - 0.3, 0.0, fresnelParms.z) * lightParms.y;
        float specularPower = mix(lightParms.z, lightParms.w, specular.z);
        float specularCoeff = pow(NdotH, specularPower) * fresnelParms2.z;
        float fresnelCoeff = fresnelTerm * fresnelParms.y + fresnelParms2.y;
    
        vec3 specularColor = specularCoeff * fresnelCoeff * specular * (diffuse.rgb * 0.25 + vec3(0.75));
        float R2f = clamp(localL.z * 4.0, 0.0, 1.0);
    
        float NdotL_adjusted = NdotL;
        float light = rimLight * R2f + NdotL_adjusted;

        // Combine everything to get the colour (unshadowed)
        totalColor = (specularColor * u_LightColour * R2f + diffuse.rgb) * light * (u_LightColour * u_LightScale) * attenuation_xy * attenuation_z * var_Colour.rgb;

        if (u_UseShadowMap)
        {
            float shadowMapResolution = (textureSize(u_ShadowMap, 0).x * u_ShadowMapRect.w);

            // The light direction is used to sample the shadow map texture
            vec3 L = normalize(var_WorldLightDirection);

            vec3 absL = abs(var_WorldLightDirection);
            float maxAbsL = max(absL.x, max(absL.y, absL.z));
            float centerFragZ = maxAbsL;

            vec3 normal = mat3(u_ObjectTransform) * N;

            float lightFallAngle = -dot(normal, L);
            float errorMargin = 5.0 * maxAbsL / ( shadowMapResolution * max(lightFallAngle, 0.1) );

            float centerBlockerZ = getDepthValueForVector(u_ShadowMap, u_ShadowMapRect, L);
            float lit = float(centerBlockerZ >= centerFragZ - errorMargin);

            totalColor *= lit;
        }
    }
    else
    {
        // Ported from TDM's interaction.ambient.fs
        vec4 light = vec4(attenuation_xy * attenuation_z, 1);

        vec3 localNormal = vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1. - bumpTexel.x*bumpTexel.x - bumpTexel.y*bumpTexel.y, 0)));
        vec3 N = normalize(var_mat_os2ts * localNormal);
        
        vec3 light1 = vec3(.5); // directionless half
        light1 += max(dot(N, u_WorldUpLocal) * (1. - specular) * .5, 0);
        
        // Calculate specularity
        vec3 nViewDir = normalize(var_LocalViewerDirection);
        vec3 reflect = - (nViewDir - 2 * N * dot(N, nViewDir));

        float spec = max(dot(reflect, u_WorldUpLocal), 0);
        float specPow = clamp((spec * spec), 0.0, 1.1);
        light1 += vec3(spec * specPow * specPow) * specular * 1.0;
        
        // Apply the light's colour (with light scale) and the vertex colour
        light1.rgb *= (u_LightColour * u_LightScale) * var_Colour.rgb;

        light.rgb *= diffuse.rgb * light1;

        light = max(light, vec4(0)); // avoid negative values, which with floating point render buffers can lead to NaN artefacts
    
        totalColor = light.rgb;
    }

	gl_FragColor.rgb = totalColor;
	gl_FragColor.a = diffuse.a;
}

