#pragma once

#include <QString>
#include <QVector3D>
#include <QVector>
#include <cstdint>
#include <optional>
#include <QRegularExpression.h>

class Model
{
public:
  Model() {};
  const QVector<QVector3D> &vertices() const;
  const QVector<uint16_t> &indices() const;

  static std::optional<Model> readObjFile(const QString &filename);

private:
  QVector<QVector3D> vertexData;
  QVector<uint16_t> indexData;
};
