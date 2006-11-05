#ifndef REMAP_H_
#define REMAP_H_

template<typename Triple>
class TripleRemapXYZ
{
public:
  static float& x(Triple& triple)
  {
    return triple.x();
  }
  static float& y(Triple& triple)
  {
    return triple.y();
  }
  static float& z(Triple& triple)
  {
    return triple.z();
  }
};

template<typename Triple>
class TripleRemapYZX
{
public:
  static float& x(Triple& triple)
  {
    return triple.y();
  }
  static float& y(Triple& triple)
  {
    return triple.z();
  }
  static float& z(Triple& triple)
  {
    return triple.x();
  }
};

template<typename Triple>
class TripleRemapZXY
{
public:
  static float& x(Triple& triple)
  {
    return triple.z();
  }
  static float& y(Triple& triple)
  {
    return triple.x();
  }
  static float& z(Triple& triple)
  {
    return triple.y();
  }
};

const float arrowhead_length = 16;
const float arrowhead_radius = 4;

inline void draw_arrowline(const float length, PointVertex* line, const std::size_t axis)
{
  (*line++).vertex = vertex3f_identity;
  (*line).vertex = vertex3f_identity;
  vertex3f_to_array((*line).vertex)[axis] = length - arrowhead_length;
}

template<typename VertexRemap, typename NormalRemap>
inline void draw_arrowhead(const std::size_t segments, const float length, FlatShadedVertex* vertices, VertexRemap, NormalRemap)
{
  std::size_t head_tris = (segments << 3);
  const double head_segment = c_2pi / head_tris;
  for(std::size_t i = 0; i < head_tris; ++i)
  {
    {
      FlatShadedVertex& point = vertices[i*6+0];
      VertexRemap::x(point.vertex) = length - arrowhead_length;
      VertexRemap::y(point.vertex) = arrowhead_radius * static_cast<float>(cos(i * head_segment));
      VertexRemap::z(point.vertex) = arrowhead_radius * static_cast<float>(sin(i * head_segment));
      NormalRemap::x(point.normal) = arrowhead_radius / arrowhead_length;
      NormalRemap::y(point.normal) = static_cast<float>(cos(i * head_segment));
      NormalRemap::z(point.normal) = static_cast<float>(sin(i * head_segment));
    }
    {
      FlatShadedVertex& point = vertices[i*6+1];
      VertexRemap::x(point.vertex) = length;
      VertexRemap::y(point.vertex) = 0;
      VertexRemap::z(point.vertex) = 0;
      NormalRemap::x(point.normal) = arrowhead_radius / arrowhead_length;
      NormalRemap::y(point.normal) = static_cast<float>(cos((i + 0.5) * head_segment));
      NormalRemap::z(point.normal) = static_cast<float>(sin((i + 0.5) * head_segment));
    }
    {
      FlatShadedVertex& point = vertices[i*6+2];
      VertexRemap::x(point.vertex) = length - arrowhead_length;
      VertexRemap::y(point.vertex) = arrowhead_radius * static_cast<float>(cos((i+1) * head_segment));
      VertexRemap::z(point.vertex) = arrowhead_radius * static_cast<float>(sin((i+1) * head_segment));
      NormalRemap::x(point.normal) = arrowhead_radius / arrowhead_length;
      NormalRemap::y(point.normal) = static_cast<float>(cos((i+1) * head_segment));
      NormalRemap::z(point.normal) = static_cast<float>(sin((i+1) * head_segment));
    }

    {
      FlatShadedVertex& point = vertices[i*6+3];
      VertexRemap::x(point.vertex) = length - arrowhead_length;
      VertexRemap::y(point.vertex) = 0;
      VertexRemap::z(point.vertex) = 0;
      NormalRemap::x(point.normal) = -1;
      NormalRemap::y(point.normal) = 0;
      NormalRemap::z(point.normal) = 0;
    }
    {
      FlatShadedVertex& point = vertices[i*6+4];
      VertexRemap::x(point.vertex) = length - arrowhead_length;
      VertexRemap::y(point.vertex) = arrowhead_radius * static_cast<float>(cos(i * head_segment));
      VertexRemap::z(point.vertex) = arrowhead_radius * static_cast<float>(sin(i * head_segment));
      NormalRemap::x(point.normal) = -1;
      NormalRemap::y(point.normal) = 0;
      NormalRemap::z(point.normal) = 0;
    }
    {
      FlatShadedVertex& point = vertices[i*6+5];
      VertexRemap::x(point.vertex) = length - arrowhead_length;
      VertexRemap::y(point.vertex) = arrowhead_radius * static_cast<float>(cos((i+1) * head_segment));
      VertexRemap::z(point.vertex) = arrowhead_radius * static_cast<float>(sin((i+1) * head_segment));
      NormalRemap::x(point.normal) = -1;
      NormalRemap::y(point.normal) = 0;
      NormalRemap::z(point.normal) = 0;
    }
  }
}

#endif /*REMAP_H_*/
