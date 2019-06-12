
class Bresenham
{
  int m_dx,m_dy, m_sx, m_sy;
  int m_x0,m_y0,m_x1,m_y1;
  int m_err;
public:
  bool Init(int x0,int y0,int x1,int y1);
  bool Tick(int *deltaX, int *deltaY);
  void GetPos(int *pX, int *pY);
};
