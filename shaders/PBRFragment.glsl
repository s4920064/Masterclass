#version 330 core
#extension GL_NV_shadow_samplers_cube : enable

// This code is based on code from here https://learnopengl.com/#!PBR/Lighting
layout (location =0) out vec4 fragColour;

smooth in vec3 VSVertexPos;
smooth in vec3 WSVertexPos;
smooth in vec3 WSVertexNormal;
smooth in vec2 WSTexCoord;

// material parameters
//uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

// lights
uniform vec3 lightPosition;
uniform vec3 lightColor;

uniform vec3 camPos;
uniform float exposure=2.2;

// A texture unit for storing the 3D texture
uniform samplerCube envMap;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------
vec3 checker(vec2 _uv)
{
  vec3 colour1 = vec3(0.9f,0.9f,0.9f);
  vec3 colour2 = vec3(0.6f,0.6f,0.6f);
  float checkSize = 60.0f;

  float v = floor( checkSize * _uv.x ) +floor( checkSize * _uv.y );
  if( mod( v, 2.0 ) < 1.0 )
     return colour2;
  else
     return colour1;
}


void main()
{
  // the uv coordinates of the object being drawn
  vec2 uv = WSTexCoord;
  vec2 c_uv = uv;

  // surface color
  vec3 albedo = checker(uv);

  // normal vector
  vec3 N = normalize(WSVertexNormal);
  // view vector
  vec3 V = normalize(camPos - VSVertexPos);
  // reflectance vector
  vec3 R = reflect(-V, N);

  // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
  // of 0.04 and if it's a metal, use their albedo color as F0 (metallic workflow)
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  // outgoing light
  vec3 Lo = vec3(0.0);

  // calculate per-light radiance

  // incident light ray
  vec3 L = normalize(lightPosition - VSVertexPos);
  // as a cubemap
  //vec3 L = textureCube(envMap, R);

  vec3 H = normalize(V + L);
  //vec3 H = normalize(L);

  // radiance from light source
  float distance = length(lightPosition - VSVertexPos);
  float attenuation = 1.0 / (distance * distance);
  vec3 radiance = lightColor * attenuation;

  // Cook-Torrance BRDF
  // Normal distribution function - orientation of microfacets
  float NDF = distributionGGX(N, H, roughness);
  // Geometry function - attenuation of light due to microfacets
  float G   = geometrySmith(N, V, L, roughness);
  // Fresnel
  vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

  vec3 nominator    = NDF * G * F;
  float denominator = 4 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0) + 0.001; // 0.001 to prevent divide by zero.
  vec3 brdf = nominator / denominator;

  // kS is equal to Fresnel
  vec3 kS = F;
  // for energy conservation, the diffuse and specular light can't
  // be above 1.0 (unless the surface emits light); to preserve this
  // relationship the diffuse component (kD) should equal 1.0 - kS.
  vec3 kD = vec3(1.0) - kS;
  // multiply kD by the inverse metalness such that only non-metals
  // have diffuse lighting, or a linear blend if partly metal (pure metals
  // have no diffuse light).
  kD *= 1.0 - metallic;

  // scale light by NdotL
  float NdotL = max(dot(N, L), 0.0);

  // add to outgoing radiance Lo
  Lo += (kD * albedo / PI + brdf) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again


  vec3 ambient = vec3(0.03) * albedo * ao;

  vec3 color = ambient + Lo;

  // HDR tonemapping
  color = color / (color + vec3(1.0));
  // gamma correct
  color = pow(color, vec3(1.0/exposure));
  vec3 colortest = vec3(0.5,1.0,0.5);

  fragColour = vec4(color, 1.0);
}
