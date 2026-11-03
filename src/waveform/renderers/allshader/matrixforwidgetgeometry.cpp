#include "waveform/renderers/allshader/matrixforwidgetgeometry.h"

#include "waveform/renderers/waveformwidgetrenderer.h"

QMatrix4x4 matrixForWidgetGeometry(WaveformWidgetRenderer* const waveformRenderer,
        bool applyDevicePixelRatio) {
    const float ratio = applyDevicePixelRatio ? waveformRenderer->getDevicePixelRatio() : 1.f;
    return matrixForWaveformGeometry(
            QSize(waveformRenderer->getWidth(), waveformRenderer->getHeight()),
            waveformRenderer->getOrientation(), ratio);
}
