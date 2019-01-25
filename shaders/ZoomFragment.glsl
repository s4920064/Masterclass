#version 400 core

layout (location =0) out vec4 fragColour;

subroutine vec4 zoom();

subroutine uniform zoom zoomOnOff;

uniform sampler2D _frameTex;
uniform vec2 _mousePos;
uniform vec2 _zoomSize;
uniform vec2 _windowSize;
uniform vec4 _zoomSquare;

subroutine(zoom)
vec4 on()
{
  vec2 uv;
  if( gl_FragCoord.x < _zoomSquare.x &&
      gl_FragCoord.x > _zoomSquare.y &&
      gl_FragCoord.y < _zoomSquare.z &&
      gl_FragCoord.y > _zoomSquare.w )
  {
    // the ratio for the offset was a trial and error value, don't how it's offset exactly
    uv = gl_FragCoord.xy / _zoomSize - _mousePos/_windowSize + vec2(0.405,0.35);
  }
  else
  {
    uv = gl_FragCoord.xy / _windowSize;
  }
  return texture2D(_frameTex, uv);
}

subroutine(zoom)
vec4 off()
{
  vec2 uv = gl_FragCoord.xy / _windowSize;
  return texture2D(_frameTex, uv);
}

void main()
{
  fragColour = zoomOnOff();
}
