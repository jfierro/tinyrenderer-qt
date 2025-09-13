#include "Model.h"

#include <QTextStream>
#include <QFile>

namespace
{
constexpr int MAX_FACES    = 65536;
constexpr int MAX_VERTICES = 65536;
constexpr int MAX_LINESIZE = 1024;

#define DECIMAL_NUMBER_REGEX "[+-]?(?:\\d*)?\\.?(?:\\d*)(?:e[-+]?\\d+)?"
#define INDEX_GROUP_PATTERN  "(\\d+)\\/(\\d+)\\/(\\d+)"
constexpr const char *whitespaceOrCommentPattern = "^(?:\\s*#.*)|\\s+$";
constexpr const char *vPattern = "^v\\s(" DECIMAL_NUMBER_REGEX
                                 ")\\s+("  DECIMAL_NUMBER_REGEX
                                 ")\\s+("  DECIMAL_NUMBER_REGEX ")\\s*$";
constexpr const char *fPattern = "^f\\s+" INDEX_GROUP_PATTERN
                                 "\\s+"   INDEX_GROUP_PATTERN
                                 "\\s+"   INDEX_GROUP_PATTERN "\\s*$";
}

const QVector<QVector3D> &
Model::vertices () const
{
  return vertexData;
}

const QVector<uint16_t> &
Model::indices () const
{
  return indexData;
}


std::optional<Model>
Model::readObjFile(const QString &filename)
{
  QFile file(filename);
  if (!file.open(QIODeviceBase::ReadOnly | QIODevice::Text))
    {
      return {};
    }

  QRegularExpression commentRegex(whitespaceOrCommentPattern);
  QRegularExpression vRegex(vPattern);
  QRegularExpression fRegex(fPattern);

  Model model;
  bool readingVertices = true;
  bool readingFaces = false;
  QTextStream ts(&file);
  int lineNumber = 0;
  int ignoredFaces = 0;
  while (!ts.atEnd())
    {
      QString line = ts.readLine(MAX_LINESIZE);
      lineNumber++;

      QRegularExpressionMatch commentMatch = commentRegex.match(line);
      if (commentMatch.hasMatch() || line.isEmpty())
        {
          continue;
        }
      if (readingVertices)
        {
          QRegularExpressionMatch vMatch = vRegex.match(line);
          if (vMatch.hasMatch())
            {
              if (model.vertexData.size() == MAX_VERTICES)
                {
                  qWarning() << QString("Vertex at line number %1 would exceed maximum number of vertices (%2).")
                                  .arg(lineNumber).arg(MAX_VERTICES);
                  return {};
                }
              bool okx, oky, okz;
              float x = vMatch.captured(1).toFloat(&okx);
              float y = vMatch.captured(2).toFloat(&oky);
              float z = vMatch.captured(3).toFloat(&okz);
              if (!okx || !oky || !okz)
                {
                  qWarning() << QString("Failed to parse a float at line number %1 (%2,%3,%4).")
                                  .arg(lineNumber).arg(okx).arg(oky).arg(okz);
                  return {};
                }
              model.vertexData.append({x, y, z});
            }
          else
            {
              readingVertices = false;
            }
        }
      QRegularExpressionMatch fMatch = fRegex.match(line);
      if (fMatch.hasMatch())
        {
          readingFaces = true;
          if (model.indexData.size() == 3*MAX_FACES)
            {
              qWarning() << QString("Face at line number %1 would exceed maximum number of faces (%2).")
              .arg(lineNumber).arg(MAX_FACES);
              return {};
            }
          bool ok0, ok1, ok2;
          int v0 = fMatch.captured(1).toInt(&ok0);
          int v1 = fMatch.captured(4).toInt(&ok1);
          int v2 = fMatch.captured(7).toInt(&ok2);

          if (!ok0 || !ok1 || !ok2)
            {
              qWarning() << QString("Failed to parse an index at line number %1 (%2,%3,%4).")
              .arg(lineNumber).arg(ok0).arg(ok1).arg(ok2);
              return {};
            }

          v0--;
          v1--;
          v2--;

          int maxIndex = model.vertexData.size()-1;
          if (   !(0 <= v0 && v0 <= maxIndex)
              || !(0 <= v1 && v1 <= maxIndex)
              || !(0 <= v2 && v2 <= maxIndex))
            {
              qWarning() << QString("Face at line number %1 refers to a non-existent "
                                      "vertex index (%2,%3,%4). The maximum is %5.")
                                 .arg(lineNumber).arg(v0).arg(v1).arg(v2).arg(maxIndex);
              return {};
            }
          model.indexData.append(v0);
          model.indexData.append(v1);
          model.indexData.append(v2);
        }
      else if (readingFaces)
        {
          ignoredFaces++;
        }
    }

  return model;
}
