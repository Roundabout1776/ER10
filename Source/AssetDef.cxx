#include "AssetTools.hxx"

#define INCBIN_PREFIX _INCBIN_

#include "incbin.h"

#define DEFINE_ASSET(NAME, PATH) \
    INCBIN(NAME, PATH); \
    extern const SAsset NAME{_INCBIN_ ## NAME ## Data, _INCBIN_ ## NAME ## Size};

namespace Asset::Common {
    DEFINE_ASSET(FramePNG, "../Asset/Texture/Frame.png")
    DEFINE_ASSET(RefPNG, "../Asset/Texture/Ref.png")
    DEFINE_ASSET(AngelPNG, "../Asset/Texture/Angel.png")
    DEFINE_ASSET(NoisePNG, "../Asset/Texture/Noise.png")
    DEFINE_ASSET(QuadOBJ, "../Asset/Mesh/Quad.obj")
    DEFINE_ASSET(PillarOBJ, "../Asset/Mesh/Pillar.obj")
    DEFINE_ASSET(HotelFloorOBJ, "../Asset/Tileset/Hotel/HotelFloor.obj")
    DEFINE_ASSET(HotelWallOBJ, "../Asset/Tileset/Hotel/HotelWall.obj")
    DEFINE_ASSET(HotelWallJointOBJ, "../Asset/Tileset/Hotel/HotelWallJoint.obj")
    DEFINE_ASSET(HotelAtlasPNG, "../Asset/Tileset/Hotel/HotelAtlas.png")
}

namespace Asset::Shader {
    DEFINE_ASSET(SharedGLSL, "../Asset/Shader/Shared.glsl")
    DEFINE_ASSET(HUDVERT, "../Asset/Shader/HUD.vert")
    DEFINE_ASSET(HUDFRAG, "../Asset/Shader/HUD.frag")
    DEFINE_ASSET(Uber2DVERT, "../Asset/Shader/Uber2D.vert")
    DEFINE_ASSET(Uber2DFRAG, "../Asset/Shader/Uber2D.frag")
    DEFINE_ASSET(Uber3DVERT, "../Asset/Shader/Uber3D.vert")
    DEFINE_ASSET(Uber3DFRAG, "../Asset/Shader/Uber3D.frag")
    DEFINE_ASSET(PostProcessVERT, "../Asset/Shader/PostProcess.vert")
    DEFINE_ASSET(PostProcessFRAG, "../Asset/Shader/PostProcess.frag")
}

namespace Asset::TileSet {

}
