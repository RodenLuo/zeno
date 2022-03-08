#pragma once

#include <zeno/types/PrimitiveObject.h>

namespace zeno {

static void prim_quads_to_tris(PrimitiveObject *prim) {
    prim->tris.reserve(prim->tris.size() + prim->quads.size() * 2);

    for (auto quad: prim->quads) {
        prim->tris.emplace_back(quad[0], quad[1], quad[2]);
        prim->tris.emplace_back(quad[0], quad[2], quad[3]);
    }
}

static void prim_polys_to_tris(PrimitiveObject *prim) {
    prim->tris.reserve(prim->tris.size() + prim->polys.size());

    for (auto [start, len]: prim->polys) {
        if (len < 3) continue;
        prim->tris.emplace_back(
                prim->loops[start],
                prim->loops[start + 1],
                prim->loops[start + 2]);
        for (int i = 3; i < len; i++) {
            prim->tris.emplace_back(
                    prim->loops[start],
                    prim->loops[start + i - 1],
                    prim->loops[start + i]);
        }
    }
}

}
