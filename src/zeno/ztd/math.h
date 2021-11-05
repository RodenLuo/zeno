#pragma once

#include <zeno/common.h>
#include <cmath>

ZENO_NAMESPACE_BEGIN
namespace ztd {
inline namespace _math_h {

static auto pymod(auto x, auto y) {
    auto z = x / y;
    z -= std::floor(z);
    return z * y;
}

static auto &imin(auto &x, auto y) {
    x = std::min(x, y);
    return x;
}

static auto &imax(auto &x, auto y) {
    x = std::max(x, y);
    return x;
}

static auto &ipymod(auto &x, auto y) {
    x = pymod(x, y);
    return x;
}

}
}
ZENO_NAMESPACE_END