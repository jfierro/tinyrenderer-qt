#include "FrameBuffer.h"
#include <QtCore/qdebug.h>

FrameBuffer::FrameBuffer(int w, int h)
    : w(w), h(h), frameBuffer(w, h, QImage::Format_ARGB32), depthBuffer(w, h, QImage::Format_Grayscale8)
{
}

void
FrameBuffer::clear (QColor c)
{
  frameBuffer.fill(c);
}

const QImage &
FrameBuffer::qimage () const
{
  return frameBuffer;
}

const QImage &
FrameBuffer::depthMap() const
{
  return depthBuffer;
}


int
FrameBuffer::width () const
{
  return w;
}

int
FrameBuffer::height () const
{
  return h;
}

void
FrameBuffer::clearDepthBuffer()
{
  depthBuffer.fill(0);
}

void
FrameBuffer::set(int x, int y, QColor c)
{
  frameBuffer.setPixelColor(x, frameBuffer.height()-1 - y, c);
}


// Clamping is not the same as clipping! But it'll have to do for now.
void
FrameBuffer::line(int ax, int ay, int bx, int by, QColor c)
{
  ax = std::clamp(ax, 0, w-1);
  ay = std::clamp(ay, 0, h-1);
  bx = std::clamp(bx, 0, w-1);
  by = std::clamp(by, 0, h-1);

  bool transpose = false;
  if (std::abs(by - ay) > std::abs(bx - ax))
    {
      std::swap(ax, ay);
      std::swap(bx, by);
      transpose = true;
    }
  if (ax > bx)
    {
      std::swap(ax, bx);
      std::swap(ay, by);
    }

  int y = ay;
  int ierror = 0;
  for (int x = ax; x <= bx; x++)
    {
      if (transpose) frameBuffer.setPixelColor(y, frameBuffer.height()-1 - x, c);
      else           frameBuffer.setPixelColor(x, frameBuffer.height()-1 - y, c);
      ierror += 2*std::abs(by-ay);
      y      += ((by > ay) ? 1 : -1)*(ierror > bx - ax);
      ierror -= 2*(bx-ax)           *(ierror > bx - ax);
    }
}

// Very ugly triangle drawing :P
// TODO: range validations, clipping?
// TODO: guard against division by 0
void
FrameBuffer::triangle (point p, point q, point r, QColor c)
{
  // swim the point with highest y-coord so that p0 is the highest point
  if (p.y < q.y)
    {
      std::swap(p, q);
    }
  if (p.y < r.y)
    {
      std::swap(p, r);
    }
  // of the remaining 2, swim the one with lower x-coord
  if (q.x > r.x)
    {
      std::swap(q, r);
    }

  // Now p0->p1 is the left edge, and p0->p2 is the right edge.
  // Define corresponding slopes in the x direction
  float mleft  = (float)(q.x - p.x)/(p.y - q.y);
  float mright = (float)(r.x - p.x)/(p.y - r.y);
  int ymid = std::max(q.y, r.y);
  int ymin = std::min(q.y, r.y);

  int y = p.y;
  int xleft = p.x;
  int xright = p.x;
  while (y >= ymid)
    {
      int x1 = p.x + std::round(mleft*(p.y - y));
      int x2 = p.x + std::round(mright*(p.y - y));
      xleft = x1;
      xright = x2;
      scanline(y, x1, x2, c);
      y--;
    }

  // recalculate slopes
  if (q.y == ymid)
    {
      mleft = (float)(r.x - q.x)/(ymid - ymin);
    }
  else
    {
      mright = (float)(q.x - r.x)/(ymid - ymin);
    }

  while (y >= ymin)
    {
      int x1 = xleft + std::round(mleft*(ymid - y));
      int x2 = xright + std::round(mright*(ymid - y));
      scanline(y, x1, x2, c);
      y--;
    }
}

// This version is closer to tinyrenderer's
void
FrameBuffer::triangle2 (point p, point q, point r, QColor c)
{
  // Sort by the y coordinate such that p.y <= q.y <= r.y
  if (p.y > q.y) std::swap(p, q);
  if (p.y > r.y) std::swap(p, r);
  if (q.y > r.y) std::swap(q, r);

  // draw first segment
  if (p.y != q.y)
    {
      for (int y = p.y; y <= q.y; y++)
        {
          int x1 = p.x + std::round(((float)(q.x - p.x)/(q.y - p.y))*(y - p.y));
          int x2 = p.x + std::round(((float)(r.x - p.x)/(r.y - p.y))*(y - p.y));
          scanline(y, x1, x2, c);
        }
    }

  // draw second segment
  if (q.y != r.y)
    {
      for (int y = q.y; y <= r.y; y++) // overwriting one scanline here
        {
          int x1 = q.x + std::round(((float)(r.x - q.x)/(r.y - q.y))*(y - q.y));
          int x2 = p.x + std::round(((float)(r.x - p.x)/(r.y - p.y))*(y - p.y));
          scanline(y, x1, x2, c);
        }
    }
}

namespace
{
float signedArea(const point &p, const point &q, const point &r)
{
  return 0.5f*(p.x*(q.y-r.y) + q.x*(r.y-p.y) + r.x*(p.y-q.y));
}

float signedArea(const point3 &p, const point3 &q, const point3 &r)
{
  return 0.5f*(p.x*(q.y-r.y) + q.x*(r.y-p.y) + r.x*(p.y-q.y));
}

float signedArea(float x, float y, const point &p, const point &q)
{
  return 0.5f*(x*(p.y-q.y) + p.x*(q.y-y) + q.x*(y-p.y));
}

bool inside(float x, float y, const point &p, const point &q, const point &r, float area)
{
  float alpha = signedArea(x, y, q, r)/area;
  float beta  = signedArea(x, y, r, p)/area;
  float gamma = signedArea(x, y, p, q)/area;
  return alpha >= 0 && beta >= 0 && gamma >= 0;
}

QColor blend(QColor a, QColor b, float alphaA, float alphaB)
{
  float alphaTotal = alphaA + alphaB;
  float aContrib = alphaA/alphaTotal;
  float bContrib = alphaB/alphaTotal;

  float red   = a.red()  *aContrib + b.red()  *bContrib;
  float green = a.green()*aContrib + b.green()*bContrib;
  float blue  = a.blue() *aContrib + b.blue() *bContrib;

  int alpha = alphaTotal*255 + 0.5f;
  QColor c(red+0.5f, green+0.5f, blue+0.5f, std::clamp(alpha, 0, 255));
  return c;
}
}

// Barycentric coordinate testing
void
FrameBuffer::triangle3 (point p, point q, point r, QColor c)
{
  int minx = std::min(std::min(p.x, q.x), r.x);
  int maxx = std::max(std::max(p.x, q.x), r.x);
  int miny = std::min(std::min(p.y, q.y), r.y);
  int maxy = std::max(std::max(p.y, q.y), r.y);
  float area = signedArea(p, q, r);

  if (area < 1)
    {
      return; // backface culling
    }

#pragma omp parallel for
  for (int x = minx; x <= maxx; x++)
    {
      for (int y = miny; y <= maxy; y++)
        {
          float alpha = signedArea({x,y}, q, r)/area;
          float beta  = signedArea({x,y}, r, p)/area;
          float gamma = signedArea({x,y}, p, q)/area;
          if (alpha >= 0 && beta >= 0 && gamma >= 0)
            {
              frameBuffer.setPixelColor(x, frameBuffer.height()-1 - y, c);
            }
        }
    }
}

// Barycentric coordinate interpolation with depth testing
void
FrameBuffer::triangle3z(point3 p, point3 q, point3 r, QColor c)
{
  int minx = std::min(std::min(p.x, q.x), r.x);
  int maxx = std::max(std::max(p.x, q.x), r.x);
  int miny = std::min(std::min(p.y, q.y), r.y);
  int maxy = std::max(std::max(p.y, q.y), r.y);
  float area = signedArea(p, q, r);

  if (area < 1)
    {
      return; // backface culling
    }

  int ignored = 0;
#pragma omp parallel for
  for (int y = miny; y <= maxy; y++)
    {
      QRgb *colorScanLine = (QRgb*)frameBuffer.scanLine(frameBuffer.height()-1 - y);
      quint8 *depthScanLine = depthBuffer.scanLine(depthBuffer.height()-1 - y);

      for (int x = minx; x <= maxx; x++)
        {
          float alpha = signedArea({x,y,0}, q, r)/area;
          float beta  = signedArea({x,y,0}, r, p)/area;
          float gamma = signedArea({x,y,0}, p, q)/area;
          if (alpha >= 0 && beta >= 0 && gamma >= 0)
            {
              int dist = std::clamp((int)std::round(alpha*p.z + beta*q.z + gamma*r.z), 0, 255);

              int dBufVal = depthScanLine[x];
              if (dist >= dBufVal)
                {
                  depthScanLine[x] = dist;
                  colorScanLine[x] = c.rgba();
                }
            }
        }
    }
}

// Barycentric coordinate testing with 4 samples per pixel
void
FrameBuffer::triangle4(point p, point q, point r, QColor c)
{
  int minx = std::min(std::min(p.x, q.x), r.x);
  int maxx = std::max(std::max(p.x, q.x), r.x);
  int miny = std::min(std::min(p.y, q.y), r.y);
  int maxy = std::max(std::max(p.y, q.y), r.y);
  float area = signedArea(p, q, r);

  if (area < 1)
    {
      return; // backface culling
    }

#pragma omp parallel for
  for (int x = minx; x <= maxx; x++)
    {
      for (int y = miny; y <= maxy; y++)
        {
          int samplesInside =   (inside(x - 0.25, y + 0.25, p, q, r, area)?1:0)
                              + (inside(x + 0.25, y + 0.25, p, q, r, area)?1:0)
                              + (inside(x - 0.25, y - 0.25, p, q, r, area)?1:0)
                              + (inside(x + 0.25, y - 0.25, p, q, r, area)?1:0);

          if (samplesInside == 0) continue;
          QColor oldColor = frameBuffer.pixelColor(x, frameBuffer.height()-1 - y);
          frameBuffer.setPixelColor(x, frameBuffer.height()-1 - y,
                                    blend(oldColor, c, oldColor.alpha()/255.0f, (samplesInside/4.0)));
        }
    }
}

namespace
{
inline bool edgeIsTopLeft(const point &a, const point &b)
{
  return ((a.y == b.y) && (b.x < a.x)) || (a.y < b.y);
}

inline int edgeFunction(const int &x, const int &y, const point &s, const int &dx, const int &dy)
{
  return (x - s.x)*dy - (y - s.y)*dx;
}

inline float edgeFunction(const float &x, const float &y, const point &s, const int &dx, const int &dy)
{
  return (x - s.x)*dy - (y - s.y)*dx;
}

inline int isInside(const float &x, const float &y,
                    const point &p, const point &q, const point &r,
                    const int &pe_dx, const int &pe_dy,
                    const int &qe_dx, const int &qe_dy,
                    const int &re_dx, const int &re_dy,
                    const bool &pe_topleft, const bool &qe_topleft, const bool &re_topleft)
{
  int e0 = edgeFunction(x, y, p, pe_dx, pe_dy);
  int e1 = edgeFunction(x, y, q, qe_dx, qe_dy);
  int e2 = edgeFunction(x, y, r, re_dx, re_dy);

  bool isInside =    ((e0 < 0) || ((e0 == 0 ) && pe_topleft))
                  && ((e1 < 0) || ((e1 == 0 ) && qe_topleft))
                  && ((e2 < 0) || ((e2 == 0 ) && re_topleft));

  return isInside ? 1 : 0;
}

}

// Using Pineda's edge functions
void
FrameBuffer::triangle5 (point p, point q, point r, QColor c)
{
  // we define 3 edges:
  //   pe: from P to Q
  //   qe: from Q to R
  //   re: from R to P
  int pe_dx = q.x - p.x;
  int pe_dy = q.y - p.y;
  int qe_dx = r.x - q.x;
  int qe_dy = r.y - q.y;
  int re_dx = p.x - r.x;
  int re_dy = p.y - r.y;

  // backface culling
  if (edgeFunction(r.x, r.y, p, pe_dx, pe_dy) > 0)
    {
      return;
    }

  bool pe_topleft = edgeIsTopLeft(p, q);
  bool qe_topleft = edgeIsTopLeft(q, r);
  bool re_topleft = edgeIsTopLeft(r, p);

  // Calculate bounding box
  int minx = std::min(std::min(p.x, q.x), r.x);
  int maxx = std::max(std::max(p.x, q.x), r.x);
  int miny = std::min(std::min(p.y, q.y), r.y);
  int maxy = std::max(std::max(p.y, q.y), r.y);

#pragma omp parallel for
  for (int x = minx; x <= maxx; x++)
    {
      for (int y = miny; y <= maxy; y++)
        {
          int e0 = edgeFunction(x, y, p, pe_dx, pe_dy);
          int e1 = edgeFunction(x, y, q, qe_dx, qe_dy);
          int e2 = edgeFunction(x, y, r, re_dx, re_dy);

          bool isInside =    ((e0 < 0) || ((e0 == 0 ) && pe_topleft))
                          && ((e1 < 0) || ((e1 == 0 ) && qe_topleft))
                          && ((e2 < 0) || ((e2 == 0 ) && re_topleft));
          if (isInside)
            {
              frameBuffer.setPixelColor(x, frameBuffer.height()-1 - y, c);
            }
        }
    }
}

// Using Pineda's edge functions + multisampling
void
FrameBuffer::triangle6(point p, point q, point r, QColor c)
{
  // we define 3 edges:
  //   pe: from P to Q
  //   qe: from Q to R
  //   re: from R to P
  int pe_dx = q.x - p.x;
  int pe_dy = q.y - p.y;
  int qe_dx = r.x - q.x;
  int qe_dy = r.y - q.y;
  int re_dx = p.x - r.x;
  int re_dy = p.y - r.y;

  // backface culling
  if (edgeFunction(r.x, r.y, p, pe_dx, pe_dy) > 0)
    {
      return;
    }

  bool pe_topleft = edgeIsTopLeft(p, q);
  bool qe_topleft = edgeIsTopLeft(q, r);
  bool re_topleft = edgeIsTopLeft(r, p);

  // Calculate bounding box
  int minx = std::min(std::min(p.x, q.x), r.x);
  int maxx = std::max(std::max(p.x, q.x), r.x);
  int miny = std::min(std::min(p.y, q.y), r.y);
  int maxy = std::max(std::max(p.y, q.y), r.y);


#pragma omp parallel for
  for (int x = minx; x <= maxx; x++)
    {
      for (int y = miny; y <= maxy; y++)
        {
          int count =   isInside(x+0.25f, y+0.25f, p, q, r, pe_dx, pe_dy, qe_dx, qe_dy, re_dx, re_dy, pe_topleft, qe_topleft, re_topleft)
                      + isInside(x-0.25f, y+0.25f, p, q, r, pe_dx, pe_dy, qe_dx, qe_dy, re_dx, re_dy, pe_topleft, qe_topleft, re_topleft)
                      + isInside(x-0.25f, y-0.25f, p, q, r, pe_dx, pe_dy, qe_dx, qe_dy, re_dx, re_dy, pe_topleft, qe_topleft, re_topleft)
                      + isInside(x+0.25f, y-0.25f, p, q, r, pe_dx, pe_dy, qe_dx, qe_dy, re_dx, re_dy, pe_topleft, qe_topleft, re_topleft);
          QColor oldColor = frameBuffer.pixelColor(x, frameBuffer.height()-1 - y);
          frameBuffer.setPixelColor(x, frameBuffer.height()-1 - y,
                                    blend(oldColor, c, oldColor.alpha()/255.0f, count/4.0f));
        }
    }
}

void
FrameBuffer::scanline (int y, int x1, int x2, QColor c)
{
  for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++)
    {
      frameBuffer.setPixelColor(x, frameBuffer.height()-1 - y, c);
    }
}
