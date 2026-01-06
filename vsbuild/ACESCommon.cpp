#include "ACESCommon.h"

aces::float3x3 aces::Chromaticities::makeRGBToXYZMat() const noexcept {
    constexpr float y = 1.0f;
    // X and Z values of RGB value (1, 1, 1), or "white"
    float x = white.x * y / white.y;
    float z = (1. - white.x - white.y) * y / white.y;

    // Scale factors for matrix rows
    float d = red.x * (blue.y - green.y) +
        blue.x * (green.y - red.y) +
        green.x * (red.y - blue.y);

    float sr = (x * (blue.y - green.y) -
        green.x * (y * (blue.y - 1) + blue.y * (x + z)) +
        blue.x * (y * (green.y - 1) + green.y * (x + z))) /
        d;

    float sg = (x * (red.y - blue.y) +
        red.x * (y * (blue.y - 1) + blue.y * (x + z)) -
        blue.x * (y * (red.y - 1) + red.y * (x + z))) /
        d;

    float sb = (x * (green.y - red.y) -
        red.x * (y * (green.y - 1) + green.y * (x + z)) +
        green.x * (y * (red.y - 1) + red.y * (x + z))) /
        d;

    // Assemble the matrix
    float3x3 m{};

    m[0][0] = sr * red[0];
    m[0][1] = sr * red[1];
    m[0][2] = sr * (1. - red[0] - red[1]);

    m[1][0] = sg * green[0];
    m[1][1] = sg * green[1];
    m[1][2] = sg * (1. - green[0] - green[1]);

    m[2][0] = sb * blue[0];
    m[2][1] = sb * blue[1];
    m[2][2] = sb * (1. - blue[0] - blue[1]);

    return m;
}
