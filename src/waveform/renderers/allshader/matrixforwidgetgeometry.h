#pragma once

#include <QMatrix4x4>
#include <QSize>

class WaveformWidgetRenderer;

inline QMatrix4x4 matrixForWaveformGeometry(
        QSize size, Qt::Orientation orientation, float devicePixelRatio = 1.f) {
    QMatrix4x4 matrix;
    matrix.ortho(QRectF(0.f, 0.f,
            size.width() * devicePixelRatio, size.height() * devicePixelRatio));
    if (orientation == Qt::Vertical) {
        matrix.rotate(90.f, 0.f, 0.f, 1.f);
        matrix.translate(0.f, -size.width() * devicePixelRatio, 0.f);
    }
    return matrix;
}

// Calculate the orthogonal project matrix for the WaveformWidgetRenderer, taking into account
// width and height, but also orientation (to rotate the matrix).
// Note that this allows us to basically use the same renderer code for horizontal and vertical
// orientation of the waveform, as we simply multiple the vertices with a rotated matrix in
// the vertex shaders.
QMatrix4x4 matrixForWidgetGeometry(
        WaveformWidgetRenderer* const widget, bool applyDevicePixelRatio);
