#version 330 core
// This code is based on code from here https://learnopengl.com/#!PBR/Lighting
layout (location =0) out vec4 fragColour;

//in vec3 worldPos;
//in vec3 normal;

uniform sampler2D currentFrameTex;
uniform sampler2D previousFrameTex;

uniform vec2 windowSize;

void main()
{
  vec2 texPos = gl_FragCoord.xy / windowSize;
  vec4 currColour = texture2D(currentFrameTex, texPos);
  vec4 prevColour = texture2D(previousFrameTex, texPos);

  float blendFactor = 0.5;
  vec4 Colour = blendFactor*currColour+(1-blendFactor)*prevColour;

  fragColour = Colour;
}
