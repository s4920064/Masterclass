#include "Jitter.h"
#include "Scene.h"

Jitter::Jitter()
{

}


Jitter::~Jitter()
{

}

void Jitter::make2x()
{
  // within a full jitter cycle, output a diagonal 2x subpixel pattern
  float jitterDistance = 0.5; // measured in pixel width
  m_offset = ngl::Vec2( jitterDistance*(1-2*float(m_cycle))/TEXTURE_WIDTH,
                        jitterDistance*(1-2*float(m_cycle))/TEXTURE_HEIGHT );

  // update the jitter cycle
  updateCycle(2);

  // update the matrix
  m_matrix.identity();
  m_matrix.translate(m_offset.m_x,m_offset.m_y,0.0f);
}

void Jitter::makeQuincunx()
{
  // within a full jitter cycle, output a subpixel quincunx pattern
  float jitterDistance = 0.5; // measured in pixel width
  switch(m_cycle)
  {
    case 0:
      m_offset = ngl::Vec2(-jitterDistance/TEXTURE_WIDTH,-jitterDistance/TEXTURE_HEIGHT);
      break;
    case 1:
      m_offset = ngl::Vec2(jitterDistance/TEXTURE_WIDTH,-jitterDistance/TEXTURE_HEIGHT);
      break;
    case 2:
      m_offset = ngl::Vec2(jitterDistance/TEXTURE_WIDTH,jitterDistance/TEXTURE_HEIGHT);
      break;
    case 3:
      m_offset = ngl::Vec2(-jitterDistance/TEXTURE_WIDTH,jitterDistance/TEXTURE_HEIGHT);
      break;
    case 4:
      m_offset = ngl::Vec2(0.0,0.0);
      break;
  }

  // update the jitter cycle
  updateCycle(5);

  // update the matrix
  m_matrix.identity();
  m_matrix.translate(m_offset.m_x,m_offset.m_y,0.0f);
}

void Jitter::updateCycle(int _samples)
{
  // update the current index for the jitter cycle
  m_cycle = (m_cycle + 1) % _samples;
//  printf("%d\n",m_cycle);
}
