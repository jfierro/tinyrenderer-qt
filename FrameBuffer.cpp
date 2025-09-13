#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(int w, int h)
    : frameBuffer(w, h), painter(&frameBuffer)
{}

void
FrameBuffer::clear (QColor c)
{
  frameBuffer.fill(c);
}

const QPixmap &
FrameBuffer::pixmap () const
{
  return frameBuffer;
}

void
FrameBuffer::set(int x, int y, QColor c)
{
  painter.setPen(c);
  painter.drawPoint(x, frameBuffer.height() - y);
}

// TODO: validations? clipping?
void
FrameBuffer::line(int ax, int ay, int bx, int by, QColor c)
{
  painter.setPen(c);

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
      if (transpose) painter.drawPoint(y, frameBuffer.height() - x);
      else           painter.drawPoint(x, frameBuffer.height() - y);
      ierror += 2*std::abs(by-ay);
      y      += ((by > ay) ? 1 : -1)*(ierror > bx - ax);
      ierror -= 2*(bx-ax)           *(ierror > bx - ax);
    }
}

// TODO: range validations, clipping?
// TODO: guard against division by 0
void
FrameBuffer::triangle (point p, point q, point r, QColor c)
{
  painter.setPen(c);

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
  painter.setPen(c);

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
}

void
FrameBuffer::triangle3 (point p, point q, point r, QColor c)
{
  int minx = std::min(std::min(p.x, q.x), r.x);
  int maxx = std::max(std::max(p.x, q.x), r.x);
  int miny = std::min(std::min(p.y, q.y), r.y);
  int maxy = std::max(std::max(p.y, q.y), r.y);
  float area = signedArea(p, q, r);

  painter.setPen(c);
//#pragma omp parallel for
  for (int x = minx; x <= maxx; x++)
    {
      for (int y = miny; y <= maxy; y++)
        {
          float alpha = signedArea({x,y}, q, r)/area;
          float beta  = signedArea({x,y}, r, p)/area;
          float gamma = signedArea({x,y}, p, q)/area;
          if (alpha >= 0 && beta >= 0 && gamma >= 0)
            {
              painter.drawPoint(x, frameBuffer.height() - y);
            }
        }
    }
}

void
FrameBuffer::scanline (int y, int x1, int x2, QColor c)
{
  for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++)
    {
      painter.drawPoint(x, frameBuffer.height() - y);
    }
}
