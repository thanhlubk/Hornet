#pragma once

#include "HornetViewExport.h"
#include <QVector>
#include <QVector3D>
#include <QVector4D>

enum class HORNETVIEW_EXPORT Projection
{
    Perspective,
    Orthographic
};
enum class HORNETVIEW_EXPORT ControlMode
{
    FPS,
    OrbitPan
};

struct Node {
    int id;
    float x, y, z;
    float r, g, b, a; // per-node color
    Node(int id_ = 0, float x_ = 0, float y_ = 0, float z_ = 0,
         float r_ = 0.95f, float g_ = 0.35f, float b_ = 0.25f, float a_ = 1.0f)
        : id(id_), x(x_), y(y_), z(z_), r(r_), g(g_), b(b_), a(a_) {}
};

struct Element {
    int id = -1;
    enum class Type {
        Tri3, Tri6, Quad4, Quad8,
        Line2, Line3,
        Tet4, Tet10, Hex8, Hex20
    };
    Type type = Type::Tri3;
    // bump capacity so Hex20 fits; older elements keep using only what they need
    int v[20] = {0};           // node IDs
    float r = 0.65f, g = 0.75f, b = 0.90f, a = 1.0f; // element color
};

enum class ForceStyle
{
    Tail,
    Head
};

struct ForceArrow
{
    int id = -1;
    QVector3D position;       // world position (tail or head, based on style)
    QVector3D direction;      // world dir
    float size = 1.0f;        // on-screen scale multiplier
    float lengthScale = 1.0f; // scales shaft only (0..∞)
    QVector4D color = {0.9f, 0.2f, 0.2f, 1.0f};
    ForceStyle style = ForceStyle::Tail;
    bool lightingEnabled = true;
};

enum class SelectType
{
    None,
    Node,
    Element,
    Force
};
