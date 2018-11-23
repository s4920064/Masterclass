#version 330 core
// This code is based on code from here https://learnopengl.com/#!PBR/Lighting
layout (location =0) out vec4 fragColour;

//in vec3 worldPos;
//in vec3 normal;

uniform sampler2D frameTex;

uniform vec2 windowSize;

void main()
{
  vec2 texPos = gl_FragCoord.xy / windowSize;
  vec4 Colour = texture2D(frameTex, texPos);

  fragColour = Colour;
}
