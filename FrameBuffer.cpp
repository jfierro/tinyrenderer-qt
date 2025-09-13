#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(int w, int h)
    : w(w), h(h), frameBuffer(w, h, QImage::Format_ARGB32)
{}

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
inline float signedArea(point p, point q, point r)
{
  return 0.5f*(p.x*(r.y-q.y) + q.x*(p.y-r.y) + r.x*(q.y-p.y));
}

inline float signedArea(float x, float y, point p, point q)
{
  return 0.5f*(x*(q.y-p.y) + p.x*(y-q.y) + q.x*(p.y-y));
}

inline bool inside(float x, float y, point p, point q, point r, float area)
{
  float alpha = signedArea(x, y, q, r)/area;
  float beta  = signedArea(x, y, r, p)/area;
  float gamma = signedArea(x, y, p, q)/area;
  return alpha >= 0 && beta >= 0 && gamma >= 0;
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

// Barycentric coordinate testing with 4 samples per pixel
void
FrameBuffer::triangle4(point p, point q, point r, QColor c)
{
  int minx = std::min(std::min(p.x, q.x), r.x);
  int maxx = std::max(std::max(p.x, q.x), r.x);
  int miny = std::min(std::min(p.y, q.y), r.y);
  int maxy = std::max(std::max(p.y, q.y), r.y);
  float area = signedArea(p, q, r);

#pragma omp parallel for
  for (int x = minx; x <= maxx; x++)
    {
      for (int y = miny; y <= maxy; y++)
        {
          int samplesInside =   inside(x - 0.25, y + 0.25, p, q, r, area)
                              + inside(x + 0.25, y + 0.25, p, q, r, area)
                              + inside(x - 0.25, y - 0.25, p, q, r, area)
                              + inside(x + 0.25, y - 0.25, p, q, r, area);

          QColor c2 = QColor(c.red(), c.blue(), c.green(), c.alpha()*(samplesInside/4.0));
          frameBuffer.setPixelColor(x, frameBuffer.height()-1 - y, c2);
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
