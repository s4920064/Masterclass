#version 410 core
// this demo is based on code from here https://learnopengl.com/#!PBR/Lighting

/// @brief the vertex passed in
layout (location = 0) in vec3 inVert;
/// @brief the normal passed in
layout (location = 1) in vec3 inNormal;
/// @brief the in uv
layout (location = 2) in vec2 inUV;

smooth out vec3 VSVertexPos;
smooth out vec3 WSVertexPos;
smooth out vec3 WSVertexNormal;
smooth out vec2 WSTexCoord;

uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;
uniform mat3 normalMatrix;

void main()
{
  // Copy across the texture coordinates
  WSTexCoord = inUV;

  VSVertexPos = vec3(V * vec4(inVert, 1.0f));
  WSVertexNormal = normalize(mat3(normalMatrix)*inNormal);
  vec4 worldPos = M*vec4(inVert,1.0);
  WSVertexPos = worldPos.xyz;
  gl_Position = MVP*vec4(inVert,1.0);
}
