#version 420 core
#extension GL_NV_shadow_samplers_cube : enable

// This code is based on code from here https://learnopengl.com/#!PBR/Lighting
layout (location =0) out vec4 fragColour;

// Inputs

smooth in vec3 VSVertexPos;
smooth in vec3 WSVertexPos;
smooth in vec3 WSVertexNormal;
smooth in vec2 WSTexCoord;

in vec3 VertexPosition;
in vec3 VertexNormal;

// Uniforms

uniform vec3 camPos;
uniform samplerCube envTex;

//Structs
//------------------------------------------------------------------------------------------------------------
struct LightInfo {
    vec4 Position; // Light position in eye coords.
    vec3 La; // Ambient light intensity
    vec3 Ld; // Diffuse light intensity
    vec3 Ls; // Specular light intensity
};

struct MaterialInfo {
    vec3 Ka; // Ambient reflectivity
    vec3 Kd; // Diffuse reflectivity
    vec3 Ks; // Specular reflectivity
    float Shininess; // Specular shininess factor
};

//Reflection Models
//------------------------------------------------------------------------------------------------------------

vec3 fresnel(vec3 N, vec3 E, float ior, vec3 diffuse, float metallic)
{
  float cosTheta = max(dot(N,E),0.0f);
  vec3 F0=vec3(ior);
  F0 = mix (F0 , diffuse, metallic);
  return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}


//GGX Distribution
//Based on: http://www.codinglabs.net/article_physically_based_rendering_cook_torrance.aspx
//          and
//          https://learnopengl.com/PBR/Lighting

float GGXDistribution(vec3 n , vec3 h, float roughness)
{

  float a  = roughness * roughness;
  float a2 = a*a;
  float NH = max(dot(n,h),0.0);
  float NH2 = NH*NH;

  float num = a2;
  float denom = (NH2 * (a2-1.0) + 1.0);
  denom = 3.14 * denom * denom;
  return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float numerator = NdotV;
    float denominator = NdotV * (1.0 - k) + k;

    return numerator / denominator;
}

//Cook Torrance BRDF using the GGX Distribution
//Based on: https://github.com/glslify/glsl-specular-cook-torrance/blob/master/index.glsl
vec3 cookTorrance(vec3 l, vec3 v, vec3 n, vec3 h, float roughness, vec3 fresnel)
{
  float pi = 3.14159;

  float VN = max(dot(v,n),0.0f);
  float LN = max(dot(l,n),0.0f);
  float NH = max(dot(n,h),0.0f);
  float VH = max(dot(v,h),0.0f);


  float X = 2.0 * NH/LN;
  float G = GeometrySchlickGGX(VN,roughness);
  //float G = GeometrySmith(n,v,l,roughness);
  float D = GGXDistribution(n,h,roughness);
  //float F = pow(1.0 - VN, fresnel);
  vec3 F = fresnel;


  float denominator = 4.0 * max(VN,0.0f) * max(LN,0.0f);
  return G*F*D / max(denominator, 0.0001);
}

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

//Instances
//------------------------------------------------------------------------------------------------------------

LightInfo Light = LightInfo(
            vec4(1.0, 10.0, 10.0, 1.0),   // position
            vec3(1.0, 1.0, 1.0),        // La - ambient
            vec3(1.0, 1.0, 1.0),        // Ld - diffuse
            vec3(1.0, 1.0, 1.0)         // Ls - specular
            );

MaterialInfo Material = MaterialInfo(
            vec3(0.2, 0.2, 0.2),    // Ka - ambient
            vec3(0.3, 0.3, 0.3),    // Kd - diffuse
            vec3(1.0,1.0,1.0),    // Ks - specular
            10.0                    // Shininess
            );

void main()
{
  // Pi
  float pi = 3.14159;

  // UV coords
  vec2 uv = WSTexCoord;

  // vertex normal and position relative to the object
  vec3 vertPos = normalize(VertexPosition);
  vec3 vertNorm = normalize(VertexNormal);

  // world space normal
  vec3 N = normalize(WSVertexNormal);
  // light incident vector
  vec3 Li = normalize(Light.Position.xyz - WSVertexPos);
  // world space view vector
  vec3 V = normalize(-WSVertexPos);
  // world space half vector
  vec3 H = normalize(V + Light.Position.xyz);
  // up vector
  vec3 Up = vec3(0.0f,0.0f,1.0f);

  // Struct replacements
  // Light diffuse
  vec3 LightLd = textureLod(envTex, reflect(-V,N),5).rgb;
  // Material diffuse
  vec3 MaterialKd = checker(uv);

  // Diffuse
  vec3 Diffuse = LightLd * dot(Light.Position.xyz,N) * MaterialKd;

  // surface properties
  float roughness = 0.7;
  float metallic = 0;
  float IOR = 0.2;


  // BRDF
  vec3 Fresnel = fresnel(N,V,IOR,Diffuse,metallic);
  vec3 Ambient = Light.La * Material.Ka;
  vec3 Specular = Light.Ls * Material.Ks * max(cookTorrance(Li,V,N,H,roughness,Fresnel),0.1);
  vec3 BRDF = Diffuse/pi;

  // the rendering equation - output light
  //vec3 Lo = BRDF * Li * dot(N,V);
  vec3 Lo = Diffuse;//+Ambient+Specular;

  // output color
  fragColour = vec4(Lo,1.0f);
}
