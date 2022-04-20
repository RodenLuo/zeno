#pragma once

#include <zeno/core/IObject.h>
#include <memory>
#include <vector>
#include <mutex>

namespace zeno {

struct GlobalComm {
    struct FrameData {
        std::vector<std::shared_ptr<IObject>> view_objects;
    };
    std::vector<FrameData> m_frames;
    int m_maxPlayFrame = 0;
    std::mutex m_mtx;

    ZENO_API void newFrame();
    ZENO_API void finishFrame();
    ZENO_API void addViewObject(std::shared_ptr<IObject> const &object);
    ZENO_API int maxPlayFrames();
    ZENO_API void clearState();
    ZENO_API std::vector<std::shared_ptr<IObject>> getViewObjects(int frameid);
    ZENO_API std::vector<std::shared_ptr<IObject>> getViewObjects();
};

}
