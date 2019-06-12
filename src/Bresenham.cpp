#include "bresenham.h"

#define SGN(x) ((x > 0) ? 1 : ((x < 0) ? -1 : 0))

int abs(int a) { return a>0?a:-a;}

bool Bresenham::Init(int x0,int y0,int x1,int y1)
{
  m_x0 = x0;
  m_y0 = y0;
  m_x1 = x1;
  m_y1 = y1;

  m_dx = abs(x1-x0); m_sx = x0<x1 ? 1 : -1;
  m_dy = abs(y1-y0); m_sy = y0<y1 ? 1 : -1;
  m_err = (m_dx>m_dy ? m_dx : -m_dy)/2;

  return (m_dx==0 && m_dy==0);
}

void Bresenham::GetPos(int *pX, int *pY)
{
  *pX = m_x0;
  *pY = m_y0;
}

bool Bresenham::Tick(int *deltaX, int *deltaY)
{
  int xx = m_x0;
  int yy = m_y0;

  int e2 = m_err;
  if (e2 >-m_dx) { m_err -= m_dy; m_x0 += m_sx; }
  if (e2 < m_dy) { m_err += m_dx; m_y0 += m_sy; }

  *deltaX = m_x0 - xx;
  *deltaY = m_y0 - yy;

  if (m_x0==m_x1 && m_y0==m_y1)
    return true;

  return false;
}
/*
void line(int x0, int y0, int x1, int y1) {

  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int err = (dx>dy ? dx : -dy)/2, e2;

  for(;;){
    setPixel(x0,y0);
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
}
*/
