#pragma once
#include "camera.h"
#include "constant_cache.h"

namespace pt {

class Viewer {
   public:
    enum State {
        Interactive,
        Render,
    };

    Viewer();
    void Update();
    void Initialize();
    void Finalize();

    inline const Camera& GetCam() const { return m_cam; }
    inline const bool Dirty() const { return m_dirty; }
    inline const State GetState() const { return m_state; }

   private:
    void EnterRenderMode();
    void HandleInput( float deltaTime );
    void UpdateCamera( float deltaTime );
    void DrawGui();
    void RenderGui();
    void InitCamera( const SceneCamera& cam, const Box3& bbox );
    void CopyCameraToCache();

    Camera m_cam;
    bool m_dirty;
    bool m_showGui;
    State m_state;
    ConstantBufferCache m_cache;
    ivec2 m_tileOffset;
    double m_lastTimestamp;
};

}  // namespace pt
