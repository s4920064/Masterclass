#version 330 core

layout (location =0) out vec4 fragColour;

uniform sampler2D _currentFrameTex;
uniform sampler2D _currentDepthTex;
uniform sampler2D _previousFrameTex;

uniform vec2 _textureSize;

uniform mat4 _view;
uniform mat4 _viewInverse;
uniform mat4 _viewPrev;
uniform mat4 _projection;
uniform mat4 _projectionInverse;
uniform vec2 _jitter;

vec2 reprojectPreviousFrame(vec2 _c_uv)
{
  // current fragment cononical-view-volume space depth
  float c_z = texture2D(_currentDepthTex,_c_uv).r;
  // current fragment cononical-view-volume space position
  vec4 c_cs = vec4(_c_uv,-c_z,1);
  // current fragment in world-space
  vec4 c_ws = _viewInverse*_projectionInverse*c_cs;
  // the previous fragment in cononical-view-volume space
  vec4 h_cs = _viewPrev*_projection*c_ws;
  // return the xy components as a uv lookup for sampling
  return h_cs.xy;
}

vec4 clampedHistory(vec2 _h_uv)
{
  int neighborSize = 3;
  vec4 neighborMin = vec4(1.0);
  vec4 neighborMax = vec4(0.0,0.0,0.0,1.0);
  for(int i;i<neighborSize;i++)
  {
    // sample the neighborhood
    for(int j;j<neighborSize;j++)
    {
      vec4 sampleColour = texture2D(_currentFrameTex,(gl_FragCoord.xy-vec2(int(neighborSize/2))+vec2(i,j))/_textureSize);
      // build the min/max for each color channel
      for(int c;c<3;c++)
      {
        neighborMin[c] = clamp(sampleColour[c],0.0,neighborMin[c]);
        neighborMax[c] = clamp(sampleColour[c],neighborMax[c],1.0);
      }
    }
  }
  // sample the previous frame
  vec4 prevColour = texture2D(_previousFrameTex,_h_uv);
  // clamp the color to the neighborhood
  vec4 clampedColour = vec4(max(min(prevColour.r,neighborMax.r),neighborMin.r),
                      max(min(prevColour.g,neighborMax.g),neighborMin.g),
                      max(min(prevColour.b,neighborMax.b),neighborMin.b),
                      1.0);
  return clampedColour;
//  return vec4(neighborMax.rgb/(neighborSize),1.0);
}

void main()
{
  vec2 c_uv = gl_FragCoord.xy / _textureSize;
  //c_uv -= _jitter;
  vec4 currColour = texture2D(_currentFrameTex, c_uv);
  //vec2 h_uv = reprojectPreviousFrame(c_uv);
  vec4 prevColour = texture2D(_previousFrameTex, c_uv);
  //vec4 prevColour = clampedHistory(h_uv);

  float blendFactor = 0.2;
  vec4 Colour = blendFactor*currColour+(1-blendFactor)*prevColour;

  fragColour = Colour;
}
