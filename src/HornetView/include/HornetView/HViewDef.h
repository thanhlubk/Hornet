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

enum class ForceStyle
{
    Tail,
    Head
};

struct ForceArrow
{
    float size = 1.0f;        // on-screen scale multiplier
    float lengthScale = 1.0f; // scales shaft only (0..∞)
    QVector4D color = {0.9f, 0.2f, 0.2f, 1.0f};
    ForceStyle style = ForceStyle::Tail;
    bool lightingEnabled = true;
};

struct ConstraintCone
{
    float size = 1.0f;        // on-screen scale multiplier
    QVector4D color = {0.2f, 0.9f, 0.2f, 1.0f}; // green
    bool lightingEnabled = true;
};

enum class SelectType
{
    None,
    Node,
    Element,
    Force,
    Constraint
};
