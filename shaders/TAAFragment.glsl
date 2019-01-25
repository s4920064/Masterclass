#version 400 core

layout (location =0) out vec4 fragColour;

subroutine vec4 nbrClamp(vec2 _h_uv);
//subroutine vec2 reprojection(vec2 _c_uv);

subroutine uniform nbrClamp nbrClampOnOff;
//subroutine uniform reprojection reproject;

uniform sampler2D _currentFrameTex;
uniform sampler2D _currentDepthTex;
uniform sampler2D _previousFrameTex;

uniform vec2 _windowSize;
uniform bool _start;

uniform mat4 _viewInverse;
uniform mat4 _viewPrev;
uniform mat4 _projection;
uniform mat4 _projectionInverse;
uniform vec2 _jitter;

// haven't been able to implement multiple subroutines so some code is commented out for now
//subroutine(reprojection)
vec2 reprojectOn(vec2 _c_uv)
{
  // current fragment clip-space depth
  float c_z = -texture2D(_currentDepthTex,_c_uv).r;

  // current fragment clip-space position
  vec4 c_cs = vec4(2*_c_uv-1,c_z,1);

  // current fragment in world-space
  vec4 p_ws = _viewInverse*_projectionInverse*c_cs;

  // the previous fragment in clip-space
  vec4 h_vs = _projection*_viewPrev*p_ws;

  //h_vs = vec4(h_vs.xy,h_z,h_vs.w);
  vec4 h_cs = h_vs;

  // return the xy components as a uv lookup for sampling
  vec2 h_uv = 0.5*h_cs.xy+0.5;

  return h_uv;
}

//subroutine(reprojection)
vec2 reprojectOff(vec2 _c_uv)
{
  return _c_uv;
}

vec4 RGBtoYCbCr(vec4 _RGB)
{
  float Y = 0.299*_RGB.r+0.587*_RGB.g+0.114*_RGB.b;
  float Cb = 0.5 - 0.168736*_RGB.r - 0.331264*_RGB.g +0.5*_RGB.b;
  float Cr = 0.5 + 0.5*_RGB.r - 0.418688*_RGB.g - 0.081312*_RGB.b;
  return vec4(Y,Cb,Cr,1);
}

vec4 YCbCrtoRGB(vec4 _YCbCr)
{
  float R = _YCbCr.x + 1.402*(_YCbCr.r - 0.5);
  float G = _YCbCr.x - 0.34414*(_YCbCr.y - 0.5) - 0.71414*(_YCbCr.z - 0.5);
  float B = _YCbCr.x + 1.772*(_YCbCr.y - 0.5);
  return vec4(R,G,B,1);
}

subroutine(nbrClamp)
vec4 nbrClampOn(vec2 _h_uv)
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
      vec4 sampleColour = texture2D(_currentFrameTex,(gl_FragCoord.xy-vec2(i,j))/_windowSize);

      // convert the sample from RGB to YCbCr
      //sampleColour = RGBtoYCbCr(sampleColour);

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

  // convert the previous frame fragment color to YCbCr
  //prevColour = RGBtoYCbCr(prevColour);

  // clamp the the previous frame's color to the current frame's neighborhood
  vec4 clampedColour = vec4(max(min(prevColour.x,neighborMax.x),neighborMin.x),
                      max(min(prevColour.y,neighborMax.y),neighborMin.y),
                      max(min(prevColour.z,neighborMax.z),neighborMin.z),
                      1.0);

  // convert the clamped color to RGB
  //clampedColour = YCbCrtoRGB(clampedColour);

  return clampedColour;
}

subroutine(nbrClamp)
vec4 nbrClampOff(vec2 _h_uv)
{
  return texture2D(_previousFrameTex,_h_uv);
}

void main()
{

  // the uv coordinates for our current fragment
  vec2 uv = gl_FragCoord.xy / _windowSize;
  vec2 c_uv = uv;

  //unjitter
  c_uv += _jitter;

  // sample current color
  vec4 currColour = texture2D(_currentFrameTex, c_uv);

  // so the first frame doesn't start gradually fading in from black
  if(_start)
  {
    fragColour = currColour;
  }

  else
  {
    // reproject previous frame
    vec2 h_uv = reprojectOn(uv);

    // use neighborhood clamping on previous frame
    vec4 prevColour = nbrClampOnOff(h_uv);

    // blend the two frames
    float blendFactor = 0.07;
    fragColour = blendFactor*currColour+(1-blendFactor)*prevColour;
  }
}

