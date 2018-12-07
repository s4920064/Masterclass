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
  vec4 c_cs = vec4(2*_c_uv-1,-c_z,1);
  // current fragment in world-space
  vec4 c_ws = _projectionInverse*_viewInverse*c_cs;
  // the previous fragment in cononical-view-volume space
  vec4 h_cs = _viewPrev*_projection*c_ws;
  // return the xy components as a uv lookup for sampling
  return 0.5*h_cs.xy+0.5;
}

vec4 clampedHistory(vec2 _h_uv)
{
  // no. of pixels for the width/height of neighborhood
  int neighborSize = 3;
  // starting values for the min and max color values
  vec4 neighborMin = vec4(1.0);
  vec4 neighborMax = vec4(0.0,0.0,0.0,1.0);

  // for each pixel in the neighborhood
  for(int i;i<neighborSize;i++)
  {
    for(int j=0;j<neighborSize;j++)
    {
      // sample the neighborhood
      vec4 sampleColour = texture2D(_currentFrameTex,(gl_FragCoord.xy-vec2(i,j))/_textureSize);

      // using the sample, build the neighborhood min/max for each color channel
      for(int c=0;c<3;c++)
      {
        neighborMin[c] = clamp(sampleColour[c],0.0,neighborMin[c]);
        neighborMax[c] = clamp(sampleColour[c],neighborMax[c],1.0);
      }
    }
  }

  // sample the previous frame
  vec4 prevColour = texture2D(_previousFrameTex,_h_uv);

  // clamp the the previous frame's color to the current frame's neighborhood
  vec4 clampedColour = vec4(max(min(prevColour.r,neighborMax.r),neighborMin.r),
                      max(min(prevColour.g,neighborMax.g),neighborMin.g),
                      max(min(prevColour.b,neighborMax.b),neighborMin.b),
                      1.0);
  return clampedColour;
}

void main()
{
  // the uv coordinates for our current fragment
  vec2 c_uv = gl_FragCoord.xy / _textureSize;

  //unjitter
  //c_uv += _jitter;

  // sample current color
  vec4 currColour = texture2D(_currentFrameTex, c_uv);

  // reproject previous frame
  vec2 h_uv = reprojectPreviousFrame(c_uv);
  vec4 prevColour = texture2D(_previousFrameTex, h_uv);

  // use neighborhood clamping on previous frame
  prevColour = clampedHistory(c_uv);

  // will it blend?
  float blendFactor = 0.1;
  vec4 Colour = blendFactor*currColour+(1-blendFactor)*prevColour;

  // TO DO: check if the following function the same as the above line
  //vec4 Colour = mix(prevColour,currColour, blendFactor);

  fragColour = Colour;
}
