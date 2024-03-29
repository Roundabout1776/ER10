#include "Draw.hxx"

#include <algorithm>
#include <numeric>
#include "CommonTypes.hxx"
#include "Log.hxx"
#include "Tile.hxx"
#include "glad/gl.h"
#include "World.hxx"
#include "Constants.hxx"
#include "Math.hxx"
#include "Memory.hxx"

#define SIZE_OF_VECTOR_ELEMENT(Vector) ((GLsizeiptr)sizeof(decltype(Vector)::value_type))

static std::string const SharedConstants{
#define SHARED_CONSTANTS_LITERAL
#include "SharedConstants.hxx"
#undef SHARED_CONSTANTS_LITERAL
};

namespace EUniformBlockBinding
{
    enum : int32_t
    {
        Globals,
        Uber2DCommon,
        Uber3DCommon,
        MapCommon,
        MapEditor,
        Map,
        MapWorld
    };
};

namespace Asset::Shader
{
    EXTERN_ASSET(SharedGLSL)
    EXTERN_ASSET(HUDVERT)
    EXTERN_ASSET(HUDFRAG)
    EXTERN_ASSET(MapVERT)
    EXTERN_ASSET(MapFRAG)
    EXTERN_ASSET(Uber2DVERT)
    EXTERN_ASSET(Uber2DFRAG)
    EXTERN_ASSET(Uber3DVERT)
    EXTERN_ASSET(Uber3DFRAG)
    EXTERN_ASSET(PostProcessVERT)
    EXTERN_ASSET(PostProcessFRAG)
}

void SProgram::CheckShader(unsigned int ShaderID)
{
    int Success;
    glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Success);
    if (!Success)
    {
        GLint MaxLength = 0;
        glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &MaxLength);
        GLsizei LengthQuery(0);
        std::vector<GLchar> InfoLog(MaxLength + 1, '\0');
        glGetShaderInfoLog(ShaderID, GLsizei(InfoLog.size()), &LengthQuery, &InfoLog[0]);
        auto Error = std::string(InfoLog.begin(), InfoLog.end());
        Log::Draw<ELogLevel::Critical>("Shader error!\n %s", Error.c_str());
    }
}

void SProgram::CheckProgram(unsigned int ProgramID)
{
    int Success;
    glValidateProgram(ProgramID);
    glGetProgramiv(ProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success)
    {
        GLint MaxLength = 0;
        glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &MaxLength);
        GLsizei LengthQuery(0);
        std::vector<GLchar> InfoLog(MaxLength + 1, '\0');
        glGetProgramInfoLog(ProgramID, GLsizei(InfoLog.size()), &LengthQuery, &InfoLog[0]);
        auto Error = std::string(InfoLog.begin(), InfoLog.end());
        Log::Draw<ELogLevel::Critical>("Program error!\n %s", Error.c_str());
    }
}

unsigned int SProgram::CreateVertexShader(const char* Data, int Length)
{
    unsigned VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    char const* Blocks[4] = {
        &Constants::GLSLVersion[0],
        &SharedConstants[0],
        Asset::Shader::SharedGLSL.SignedCharPtr(),
        Data
    };
    int const Lengths[4] = {
        (int)strlen(Constants::GLSLVersion),
        (int)SharedConstants.length(),
        (int)Asset::Shader::SharedGLSL.Length,
        Length
    };
    glShaderSource(VertexShaderID, 4, Blocks, &Lengths[0]);
    glCompileShader(VertexShaderID);
    CheckShader(VertexShaderID);
    return VertexShaderID;
}

unsigned int SProgram::CreateFragmentShader(const char* Data, int Length)
{
    unsigned FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    char const* Blocks[4] = {
        &Constants::GLSLVersion[0],
        &SharedConstants[0],
        Asset::Shader::SharedGLSL.SignedCharPtr(),
        Data
    };
    int const Lengths[4] = {
        (int)strlen(Constants::GLSLVersion),
        (int)SharedConstants.length(),
        (int)Asset::Shader::SharedGLSL.Length,
        Length
    };
    glShaderSource(FragmentShaderID, 4, Blocks, &Lengths[0]);
    glCompileShader(FragmentShaderID);
    CheckShader(FragmentShaderID);
    return FragmentShaderID;
}

unsigned int SProgram::CreateProgram(unsigned int VertexShader, unsigned int FragmentShader)
{
    unsigned ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShader);
    glAttachShader(ProgramID, FragmentShader);
    glLinkProgram(ProgramID);
    return ProgramID;
}

void SProgram::InitUniforms()
{
    UniformModeID = glGetUniformLocation(ID, "u_mode");
    UniformModeControlAID = glGetUniformLocation(ID, "u_modeControlA");
    UniformModeControlBID = glGetUniformLocation(ID, "u_modeControlB");

    UniformGlobals = glGetUniformBlockIndex(ID, "ub_globals");
    glUniformBlockBinding(ID, UniformGlobals, 0);
}

void SProgram::Init(const SAsset& InVertexShaderAsset, const SAsset& InFragmentShaderAsset)
{
#ifdef EQUINOX_REACH_DEVELOPMENT
    VertexShaderAsset = &InVertexShaderAsset;
    FragmentShaderAsset = &InFragmentShaderAsset;
#endif
    unsigned VertexShader = CreateVertexShader(InVertexShaderAsset.SignedCharPtr(), (int)InVertexShaderAsset.Length);
    unsigned FragmentShader = CreateFragmentShader(InFragmentShaderAsset.SignedCharPtr(), (int)InFragmentShaderAsset.Length);
    ID = CreateProgram(VertexShader, FragmentShader);
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
    SProgram::InitUniforms();
    InitUniforms();
    InitUniformBlocks();
    CheckProgram(ID);
}

void SProgram::Cleanup() const
{
    CleanupUniformBlocks();
    glDeleteProgram(ID);

    Log::Draw<ELogLevel::Debug>("Deleting SProgram");
}

void SProgram::Use() const
{
    glUseProgram(ID);
}

#ifdef EQUINOX_REACH_DEVELOPMENT
    #include <sstream>
void SProgram::Reload()
{
    namespace fs = std::filesystem;

    std::ifstream ShaderFile;

    unsigned VertexShader, FragmentShader;

    ShaderFile.open(VertexShaderAsset->Path(), std::ifstream::in);
    std::stringstream VertexShaderStringBuffer;
    VertexShaderStringBuffer << ShaderFile.rdbuf();
    auto VertexShaderString = VertexShaderStringBuffer.str();
    VertexShader = CreateVertexShader(VertexShaderString.data(), (int)VertexShaderString.length());

    ShaderFile.close();

    ShaderFile.open(FragmentShaderAsset->Path(), std::ifstream::in);
    std::stringstream FragmentShaderStringBuffer;
    FragmentShaderStringBuffer << ShaderFile.rdbuf();
    auto FragmentShaderString = FragmentShaderStringBuffer.str();
    FragmentShader = CreateFragmentShader(FragmentShaderString.data(), (int)FragmentShaderString.length());

    ShaderFile.close();

    Log::Draw<ELogLevel::Debug>("Reloading GL program");
    Log::Draw<ELogLevel::Debug>("Vertex shader: %s", VertexShaderAsset->Path().string().c_str());
    Log::Draw<ELogLevel::Debug>("Fragment shader: %s", FragmentShaderAsset->Path().string().c_str());

    glDeleteProgram(ID);
    ID = CreateProgram(VertexShader, FragmentShader);
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
    SProgram::InitUniforms();
    InitUniforms();
    CheckProgram(ID);
}
#endif

void SProgramPostProcess::InitUniforms()
{
    UniformColorTextureID = glGetUniformLocation(ID, "u_colorTexture");

    glProgramUniform1i(ID, UniformColorTextureID, ETextureUnits::MainFramebuffer);
}

void SProgram3D::InitUniforms()
{
    UniformModelID = glGetUniformLocation(ID, "u_model");
    UniformCommonAtlasID = glGetUniformLocation(ID, "u_commonAtlas");
    UniformPrimaryAtlasID = glGetUniformLocation(ID, "u_primaryAtlas");

    glProgramUniform1i(ID, UniformCommonAtlasID, ETextureUnits::AtlasCommon);
    glProgramUniform1i(ID, UniformPrimaryAtlasID, ETextureUnits::AtlasPrimary3D);

    glUniformBlockBinding(ID, glGetUniformBlockIndex(ID, "ub_common"), EUniformBlockBinding::Uber3DCommon);
}

void SProgram2D::InitUniforms()
{
    glUniformBlockBinding(ID, glGetUniformBlockIndex(ID, "ub_common"), EUniformBlockBinding::Uber2DCommon);
    UniformPositionScreenSpaceID = glGetUniformLocation(ID, "u_positionScreenSpace");
    UniformSizeScreenSpaceID = glGetUniformLocation(ID, "u_sizeScreenSpace");
    UniformCommonAtlasID = glGetUniformLocation(ID, "u_commonAtlas");

    glProgramUniform1i(ID, UniformCommonAtlasID, ETextureUnits::AtlasCommon);
}

void SProgramUber2D::InitUniforms()
{
    SProgram2D::InitUniforms();
    UniformUVRectID = glGetUniformLocation(ID, "u_uvRect");
    UniformPrimaryAtlasID = glGetUniformLocation(ID, "u_primaryAtlas");

    glProgramUniform1i(ID, UniformPrimaryAtlasID, ETextureUnits::AtlasPrimary2D);
}

void SProgramHUD::InitUniforms()
{
    SProgram2D::InitUniforms();
}

void SProgramMap::InitUniforms()
{
    SProgram2D::InitUniforms();
    UniformWorldTextures = glGetUniformLocation(ID, "u_worldTextures");

    glProgramUniform1i(ID, UniformWorldTextures, ETextureUnits::WorldTextures);

    glUniformBlockBinding(ID, glGetUniformBlockIndex(ID, "ub_common"), EUniformBlockBinding::MapCommon);
    glUniformBlockBinding(ID, glGetUniformBlockIndex(ID, "ub_editor"), EUniformBlockBinding::MapEditor);
    glUniformBlockBinding(ID, glGetUniformBlockIndex(ID, "ub_map"), EUniformBlockBinding::Map);
    glUniformBlockBinding(ID, glGetUniformBlockIndex(ID, "ub_world"), EUniformBlockBinding::MapWorld);
}

void SProgramMap::InitUniformBlocks()
{
    UniformBlockCommon.Init(sizeof(SShaderMapCommon));
    UniformBlockCommon.Bind(EUniformBlockBinding::MapCommon);

    UniformBlockEditor.Init(sizeof(SShaderMapEditor));
    UniformBlockEditor.Bind(EUniformBlockBinding::MapEditor);

    UniformBlockMap.Init(sizeof(SShaderMapData));
    UniformBlockMap.Bind(EUniformBlockBinding::Map);

    UniformBlockWorld.Init(sizeof(SShaderWorld));
    UniformBlockWorld.Bind(EUniformBlockBinding::MapWorld);
}

void SProgramMap::CleanupUniformBlocks() const
{
    UniformBlockCommon.Cleanup();
    UniformBlockEditor.Cleanup();
    UniformBlockMap.Cleanup();
    UniformBlockWorld.Cleanup();
}

void SProgramMap::SetEditorData(const SVec2& SelectedTile, const SVec4& SelectedBlock, uint32_t bEnabled, uint32_t bToggleMode, uint32_t bBlockMode) const
{
    SShaderMapEditor ShaderMapEditor;
    ShaderMapEditor.bBlockMode = bBlockMode;
    ShaderMapEditor.bToggleMode = bToggleMode;
    ShaderMapEditor.SelectedTile = SelectedTile;
    ShaderMapEditor.SelectedBlock = SelectedBlock;
    ShaderMapEditor.bEnabled = bEnabled;

    glBindBuffer(GL_UNIFORM_BUFFER, UniformBlockEditor.UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SShaderMapEditor), &ShaderMapEditor);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SProgramMap::SetCursor(const SVec2& Cursor) const
{
    UniformBlockCommon.SetVector2(offsetof(SShaderMapCommon, Cursor), Cursor);
}

void SGeometry::InitFromRawMesh(const CRawMesh& RawMesh)
{
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        (GLsizeiptr)RawMesh.Positions.size() * SIZE_OF_VECTOR_ELEMENT(RawMesh.Positions),
        &RawMesh.Positions[0],
        GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(1);
    glGenBuffers(1, &CBO);
    glBindBuffer(GL_ARRAY_BUFFER, CBO);
    glBufferData(GL_ARRAY_BUFFER,
        (GLsizeiptr)RawMesh.TexCoords.size() * SIZE_OF_VECTOR_ELEMENT(RawMesh.TexCoords),
        &RawMesh.TexCoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        0,
        nullptr);

    ElementCount = (int)RawMesh.Indices.size();
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        ElementCount * SIZE_OF_VECTOR_ELEMENT(RawMesh.Indices), &RawMesh.Indices[0],
        GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void SGeometry::Cleanup()
{
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &CBO);
    glDeleteVertexArrays(1, &VAO);

    Log::Draw<ELogLevel::Debug>("Deleting SGeometry with ElementCount: %d", ElementCount);
}

void STileset::InitPlaceholder()
{
    std::array<SVec3, 8> TempVertices{};
    std::array<unsigned short, 12> Indices{};

    /** Floor Quad */
    auto& FloorGeometry = TileGeometry[ETileGeometryType::Floor];
    FloorGeometry.ElementOffset = 0;
    FloorGeometry.ElementCount = 6;
    Indices[0] = 0;
    Indices[1] = 1;
    Indices[2] = 2;
    Indices[3] = 0;
    Indices[4] = 2;
    Indices[5] = 3;

    TempVertices[0] = { 0.5f, 0.0f, 0.5f };
    TempVertices[1] = { 0.5f, 0.0f, -0.5f };
    TempVertices[2] = { -0.5f, 0.0f, -0.5f };
    TempVertices[3] = { -0.5f, 0.0f, 0.5f };

    /** Wall Quad */
    auto& WallGeometry = TileGeometry[ETileGeometryType::Wall];
    WallGeometry.ElementOffset = 12;
    WallGeometry.ElementCount = 6;
    Indices[6] = 4 + 0;
    Indices[7] = 4 + 1;
    Indices[8] = 4 + 2;
    Indices[9] = 4 + 0;
    Indices[10] = 4 + 2;
    Indices[11] = 4 + 3;

    TempVertices[4] = { 0.5f, 1.0f, -0.5f };
    TempVertices[5] = { -0.5f, 1.0f, -0.5f };
    TempVertices[6] = { -0.5f, 0.0f, -0.5f };
    TempVertices[7] = { 0.5f, 0.0f, -0.5f };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, TempVertices.size() * (GLsizeiptr)sizeof(SVec3), &TempVertices[0],
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    ElementCount = (int)Indices.size();
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ElementCount * (GLsizeiptr)sizeof(unsigned short), &Indices[0],
        GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void STileset::InitBasic(
    const SAsset& Floor,
    const SAsset& Hole,
    const SAsset& Wall,
    const SAsset& WallJoint,
    const SAsset& DoorFrame,
    const SAsset& Door)
{
    auto Positions = Memory::GetVector<SVec3>();
    auto TexCoords = Memory::GetVector<SVec2>();
    auto Indices = Memory::GetVector<unsigned short>();

    int LastElementOffset = 0;
    int LastVertexOffset = 0;

    auto InitGeometry = [&](const SAsset& Resource, int Type) {
        auto Mesh = CRawMesh(Resource);

        auto& Geometry = TileGeometry[Type];
        Geometry.ElementOffset = LastElementOffset;
        Geometry.ElementCount = Mesh.GetElementCount();

        for (int Index = 0; Index < Geometry.ElementCount; ++Index)
        {
            Indices.push_back(Mesh.Indices[Index] + LastVertexOffset);
        }

        for (int Index = 0; Index < Mesh.GetVertexCount(); ++Index)
        {
            Positions.push_back(Mesh.Positions[Index]);
            TexCoords.push_back(Mesh.TexCoords[Index]);
        }

        LastVertexOffset += Mesh.GetVertexCount();
        LastElementOffset += (int)((Geometry.ElementCount) * sizeof(unsigned short));
    };

    InitGeometry(Floor, ETileGeometryType::Floor);
    InitGeometry(Hole, ETileGeometryType::Hole);
    InitGeometry(Wall, ETileGeometryType::Wall);
    InitGeometry(WallJoint, ETileGeometryType::WallJoint);
    InitGeometry(DoorFrame, ETileGeometryType::DoorFrame);
    InitGeometry(Door, ETileGeometryType::Door);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)Positions.size() * (GLsizeiptr)sizeof(SVec3), &Positions[0],
        GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(1);
    glGenBuffers(1, &CBO);
    glBindBuffer(GL_ARRAY_BUFFER, CBO);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(TexCoords.size() * SIZE_OF_VECTOR_ELEMENT(TexCoords)),
        &TexCoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        0,
        nullptr);

    ElementCount = (int)Indices.size();
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ElementCount * (GLsizeiptr)sizeof(unsigned short), &Indices[0],
        GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void SCamera::RegenerateProjection()
{
    float const FOVRadians = Math::Radians(FieldOfViewY);
    float const TanHalfFovY = std::tan(FOVRadians / 2.0f);

    Projection = SMat4x4();
    Projection.X.X = 1.0f / (Aspect * TanHalfFovY);
    Projection.Y.Y = 1.0f / (TanHalfFovY);
    Projection.Z.Z = -(ZFar + ZNear) / (ZFar - ZNear);
    Projection.Z.W = -1.0f;
    Projection.W.Z = -(2.0f * ZFar * ZNear) / (ZFar - ZNear);
}

void SCamera::Update()
{
    View = SMat4x4::LookAtRH(Position, Target, SVec3{ 0.0f, 1.0f, 0.0f });
}

void SWorldFramebuffer::Init(int TextureUnitID, int InWidth, int InHeight, SVec3 InClearColor)
{
    Width = InWidth;
    Height = InHeight;
    ClearColor = InClearColor;

    glActiveTexture(GL_TEXTURE0 + TextureUnitID);
    glGenTextures(1, &ColorID);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ColorID);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, InWidth, InHeight, WORLD_MAX_LAYERS, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE0);

    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ColorID, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SWorldFramebuffer::Cleanup()
{
    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &ColorID);

    Log::Draw<ELogLevel::Debug>("Deleting SWorldFramebuffer");
}

void SWorldFramebuffer::ResetViewport() const
{
    glViewport(0, 0, Width, Height);
}

void SWorldFramebuffer::SetLayer(int LayerIndex) const
{
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ColorID, 0, LayerIndex);
}

void SMainFramebuffer::Init(int TextureUnitID, int InWindowWidth, int InWindowHeight)
{
    CalculateSize(InWindowWidth, InWindowHeight);

    glActiveTexture(GL_TEXTURE0 + TextureUnitID);
    glGenTextures(1, &ColorID);
    glBindTexture(GL_TEXTURE_2D, ColorID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &DepthID);
    glBindTexture(GL_TEXTURE_2D, DepthID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Width, Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE0, 0);

    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ColorID, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, DepthID, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SMainFramebuffer::Cleanup()
{
    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &ColorID);
    glDeleteTextures(1, &DepthID);

    Log::Draw<ELogLevel::Debug>("Deleting SMainFramebuffer");
}

void SMainFramebuffer::CalculateSize(int InWindowWidth, int InWindowHeight)
{
    int NewWidth = Constants::ReferenceWidth;
    int NewHeight = Constants::ReferenceHeight;

    const int MaxAvailableScale = std::min(InWindowWidth / Constants::ReferenceWidth, InWindowHeight / Constants::ReferenceHeight);
    if (MaxAvailableScale >= 1)
    {
        NewWidth += (InWindowWidth - MaxAvailableScale * Constants::ReferenceWidth) / MaxAvailableScale;
        NewHeight += (InWindowHeight - MaxAvailableScale * Constants::ReferenceHeight) / MaxAvailableScale;

        NewWidth++;
        NewHeight++;
    }

    Width = NewWidth;
    Height = NewHeight;
    Scale = MaxAvailableScale;
}

void SMainFramebuffer::Resize(int InWindowWidth, int InWindowHeight)
{
    CalculateSize(InWindowWidth, InWindowHeight);

    glBindTexture(GL_TEXTURE_2D, ColorID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, DepthID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, Width, Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void SMainFramebuffer::ResetViewport() const
{
    glViewport(0, 0, Width, Height);
}

void SUniformBlock::Init(int Size)
{
    glGenBuffers(1, &UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferData(GL_UNIFORM_BUFFER, Size, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SUniformBlock::Cleanup() const
{
    glDeleteBuffers(1, &UBO);
}

void SUniformBlock::Bind(int BindingPoint) const
{
    // glBindBufferRange(GL_UNIFORM_BUFFER, 0, UBO, 0, Size);
    glBindBufferBase(GL_UNIFORM_BUFFER, BindingPoint, UBO);
}

void SUniformBlock::SetMatrix(int Position, const SMat4x4& Value) const
{
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, Position, sizeof(Value), &Value.X);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SUniformBlock::SetVector2(int Position, const SVec2& Value) const
{
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, Position, sizeof(Value), &Value.X);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SUniformBlock::SetFloat(int Position, const float Value) const
{
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, Position, sizeof(Value), &Value);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SRenderer::Init(int Width, int Height)
{
    /* Common OpenGL settings. */
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Setup a quad for 2D rendering. */
    glGenVertexArrays(1, &Quad2D.VAO);
    glBindVertexArray(Quad2D.VAO);

    const float QuadVertices[] = {
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &Quad2D.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, Quad2D.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), &QuadVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    const float QuadUVs[] = {
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 1.0f
    };
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &Quad2D.CBO);
    glBindBuffer(GL_ARRAY_BUFFER, Quad2D.CBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadUVs), &QuadUVs[0], GL_STATIC_DRAW);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        0,
        nullptr);

    const unsigned short QuadIndices[] = {
        0, 1, 2, 0, 2, 3
    };
    glGenBuffers(1, &Quad2D.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Quad2D.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QuadIndices), &QuadIndices[0], GL_STATIC_DRAW);
    Quad2D.ElementCount = std::size(QuadIndices);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    /* Initialize uniform blocks. */
    GlobalsUniformBlock.Init(sizeof(SShaderGlobals));
    GlobalsUniformBlock.Bind(EUniformBlockBinding::Globals);

    Queue2D.CommonUniformBlock.Init(32);
    Queue2D.CommonUniformBlock.Bind(EUniformBlockBinding::Uber2DCommon);

    Queue3D.CommonUniformBlock.Init(sizeof(SMat4x4) * 2);
    Queue3D.CommonUniformBlock.Bind(EUniformBlockBinding::Uber3DCommon);

    /* Initialize framebuffers. */
    WorldLayersFramebuffer.Init(ETextureUnits::WorldTextures,
        int(MapWorldLayerTextureSize.X),
        int(MapWorldLayerTextureSize.Y),
        TVec3{ 0.0f, 0.0f, 1.0f });
    MainFramebuffer.Init(ETextureUnits::MainFramebuffer, Width, Height);

    /* Initialize atlases. */
    Atlases[ATLAS_COMMON].Init(ETextureUnits::AtlasCommon);
    Atlases[ATLAS_PRIMARY2D].Init(ETextureUnits::AtlasPrimary2D);
    Atlases[ATLAS_PRIMARY3D].Init(ETextureUnits::AtlasPrimary3D);

    /* Initialize shaders. */
    ProgramHUD.Init(Asset::Shader::HUDVERT, Asset::Shader::HUDFRAG);
    ProgramMap.Init(Asset::Shader::MapVERT, Asset::Shader::MapFRAG);
    ProgramUber2D.Init(Asset::Shader::Uber2DVERT, Asset::Shader::Uber2DFRAG);
    ProgramUber3D.Init(Asset::Shader::Uber3DVERT, Asset::Shader::Uber3DFRAG);
    ProgramPostProcess.Init(Asset::Shader::PostProcessVERT, Asset::Shader::PostProcessFRAG);
}

void SRenderer::Cleanup()
{
    MainFramebuffer.Cleanup();
    WorldLayersFramebuffer.Cleanup();
    for (auto& Atlas : Atlases)
    {
        Atlas.Cleanup();
    }
    Quad2D.Cleanup();
    GlobalsUniformBlock.Cleanup();
    Queue2D.CommonUniformBlock.Cleanup();
    Queue3D.CommonUniformBlock.Cleanup();
    ProgramHUD.Cleanup();
    ProgramMap.Cleanup();
    ProgramUber2D.Cleanup();
    ProgramUber3D.Cleanup();
    ProgramPostProcess.Cleanup();
}

void SRenderer::SetMapIcons(const std::array<SSpriteHandle, MAP_ICON_COUNT>& SpriteHandles) const
{
    std::array<SShaderSprite, MAP_ICON_COUNT> Sprites;

    for (auto Index = 0; Index < (int)MAP_ICON_COUNT; Index++)
    {
        Sprites[Index].UVRect = SpriteHandles[Index].Sprite->UVRect;
        Sprites[Index].SizeX = SpriteHandles[Index].Sprite->SizePixels.X;
        Sprites[Index].SizeY = SpriteHandles[Index].Sprite->SizePixels.Y;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, ProgramMap.UniformBlockCommon.UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(SShaderMapCommon, Icons), sizeof(SShaderSprite) * MAP_ICON_COUNT, Sprites.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SRenderer::SetupTileset(const STileset* Tileset)
{
    LevelDrawData.TileSet = Tileset;
    for (int TileTypeIndex = 0; TileTypeIndex < ETileGeometryType::Count; TileTypeIndex++)
    {
        auto& DrawCall = LevelDrawData.DrawCalls[TileTypeIndex];
        DrawCall.SubGeometry = &Tileset->TileGeometry[TileTypeIndex];
    }
}

void SRenderer::UploadMapData(const SWorldLevel* Level, const SCoordsAndDirection& POV) const
{
    SShaderMapData ShaderMapData{};

    ShaderMapData.Width = (int)Level->Width;
    ShaderMapData.Height = (int)Level->Height;
    ShaderMapData.POV = POV;
    ShaderMapData.Tiles = Level->Tiles;

    glBindBuffer(GL_UNIFORM_BUFFER, ProgramMap.UniformBlockMap.UBO);
    /* Upload only relevant tiles within (width * height) range. */
    glBufferSubData(GL_UNIFORM_BUFFER, 0, offsetof(SShaderMapData, Tiles) + sizeof(STile) * (Level->Width * Level->Height), &ShaderMapData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SRenderer::SetTime(float Time) const
{
    GlobalsUniformBlock.SetFloat(offsetof(SShaderGlobals, Time), Time);
}

void SRenderer::Flush(const SPlatformState& WindowData)
{
    GlobalsUniformBlock.SetVector2(offsetof(SShaderGlobals, ScreenSize), { (float)MainFramebuffer.Width, (float)MainFramebuffer.Height });

    /* Begin Draw */
    glBindFramebuffer(GL_FRAMEBUFFER, MainFramebuffer.FBO);
    glClearColor(0.0028f, 0.0566f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Draw 3D */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    SVec2Int SceneOffset = (SVec2Int(MainFramebuffer.Width, MainFramebuffer.Height) - Constants::SceneSize) / 2;
    glViewport(SceneOffset.X, SceneOffset.Y, Constants::SceneSize.X, Constants::SceneSize.Y);

    ProgramUber3D.Use();

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (int Index = 0; Index < Queue3D.CurrentIndex; ++Index)
    {
        const auto& Entry = Queue3D.Entries[Index];
        if (Entry.Geometry == nullptr)
        {
            continue;
        }
        glUniform1i(ProgramUber3D.UniformModeID, Entry.Mode.ID);
        glBindVertexArray(Entry.Geometry->VAO);
        if (Entry.InstancedDrawCall != nullptr)
        {
            for (int DrawCallIndex = 0; DrawCallIndex < Entry.InstancedDrawCallCount; ++DrawCallIndex)
            {
                auto& DrawCall = *(Entry.InstancedDrawCall + DrawCallIndex);
                auto TotalCount = DrawCall.Count + DrawCall.DynamicCount;
                if (TotalCount > 0)
                {
                    glUniformMatrix4fv(ProgramUber3D.UniformModelID, TotalCount, GL_FALSE,
                        &DrawCall.Transform[0].X.X);
                    glDrawElementsInstanced(GL_TRIANGLES,
                        DrawCall.SubGeometry->ElementCount,
                        GL_UNSIGNED_SHORT,
                        reinterpret_cast<void*>(DrawCall.SubGeometry->ElementOffset),
                        TotalCount);
                }
            }
        }
        else
        {
            glUniformMatrix4fv(ProgramUber3D.UniformModelID, 1, GL_FALSE, &Entry.Model.X.X);
            glDrawElements(GL_TRIANGLES, Entry.Geometry->ElementCount, GL_UNSIGNED_SHORT, nullptr);
        }
    }
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    /* Draw 2D */
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glViewport(0, 0, MainFramebuffer.Width, MainFramebuffer.Height);

    glBindVertexArray(Quad2D.VAO);

    for (int Index = 0; Index < Queue2D.CurrentIndex; ++Index)
    {
        auto const& Entry = Queue2D.Entries[Index];

        SProgram2D const* Program;
        switch (Entry.Program2DType)
        {
            case EProgram2DType::HUD:
                Program = &ProgramHUD;
                break;
            case EProgram2DType::Map:
                Program = &ProgramMap;
                break;
            case EProgram2DType::Uber2D:
                Program = &ProgramUber2D;
                break;
            default:
                continue;
        }
        Program->Use();

        glUniform2f(Program->UniformPositionScreenSpaceID, Entry.Position.X, Entry.Position.Y);
        glUniform2f(Program->UniformSizeScreenSpaceID, (float)Entry.SizePixels.X, (float)Entry.SizePixels.Y);

        const SEntryMode& Mode = Entry.Mode;
        glUniform1i(Program->UniformModeID, Mode.ID);

        if (Entry.Program2DType == EProgram2DType::Uber2D)
        {
            glUniform4fv(ProgramUber2D.UniformUVRectID, 1, &Entry.UVRect.X);

            if (Mode.ID > UBER2D_MODE_TEXTURE)
            {
                glUniform4fv(ProgramUber2D.UniformModeControlAID, 1, &Mode.ControlA.X);
                glUniform4fv(ProgramUber2D.UniformModeControlBID, 1, &Mode.ControlB.X);
            }
            if (Mode.ID == UBER2D_MODE_BACK_BLUR)
            {
                glDrawElementsInstanced(GL_TRIANGLES, Quad2D.ElementCount, GL_UNSIGNED_SHORT, nullptr,
                    (int)Mode.ControlA.X);
                glUniform1i(Program->UniformModeID, 0);
            }
        }
        else if (Entry.Program2DType == EProgram2DType::HUD)
        {
        }

        glDrawElements(GL_TRIANGLES, Quad2D.ElementCount, GL_UNSIGNED_SHORT, nullptr);
    }

    /* Blit main framebuffer to our window. */
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, WindowData.Width, WindowData.Height);
    glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, MainFramebuffer.Width * MainFramebuffer.Scale, MainFramebuffer.Height * MainFramebuffer.Scale);

    ProgramPostProcess.Use();
    glDrawElements(GL_TRIANGLES, Quad2D.ElementCount, GL_UNSIGNED_SHORT, nullptr);

    Queue2D.Reset();
    Queue3D.Reset();
}

void SRenderer::UploadProjectionAndViewFromCamera(const SCamera& Camera) const
{
    Queue3D.CommonUniformBlock.SetMatrix(0, Camera.Projection);
    /* @TODO: Proper struct offsets? */
    Queue3D.CommonUniformBlock.SetMatrix(sizeof(SMat4x4), Camera.View);
}

void SRenderer::DrawHUD(SVec3 Position, SVec2Int Size, int Mode)
{
    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::HUD;
    Entry.Position = Position;
    Entry.SizePixels = Size;

    Entry.Mode = SEntryMode{ Mode };

    Queue2D.Enqueue(Entry);
}

void SRenderer::DrawMap(SWorldLevel* Level, SVec3 Position, SVec2Int Size, const SCoordsAndDirection& POV)
{
    bool bPOVChanged = Level->DirtyFlags & ELevelDirtyFlags::POVChanged;
    bool bDirtyRange = Level->DirtyFlags & ELevelDirtyFlags::DirtyRange;

    if (bPOVChanged || bDirtyRange)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, ProgramMap.UniformBlockMap.UBO);
        if (bPOVChanged)
        {
            glBufferSubData(GL_UNIFORM_BUFFER, offsetof(SShaderMapData, POV), sizeof(SCoordsAndDirection), &POV);

            Level->DirtyFlags &= ~ELevelDirtyFlags::POVChanged;

            Log::Draw<ELogLevel::Verbose>("%s(): POV: { { %.2f, %.2f }, %d }", __func__, POV.Coords.X, POV.Coords.Y, POV.Direction.Index);
        }

        if (bDirtyRange)
        {
            auto DirtyCount = (GLsizeiptr)(Level->DirtyRange.Y - Level->DirtyRange.X) + 1;
            auto FirstTile = Level->GetTile(Level->DirtyRange.X);
            glBufferSubData(GL_UNIFORM_BUFFER, offsetof(SShaderMapData, Tiles) + (Level->DirtyRange.X * sizeof(STile)), DirtyCount * (GLsizeiptr)sizeof(STile), FirstTile);

            Level->DirtyFlags &= ~ELevelDirtyFlags::DirtyRange;

            Log::Draw<ELogLevel::Debug>("%s(): DirtyRange: %d to %d", __func__, Level->DirtyRange.X, Level->DirtyRange.Y);
        }
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::Map;
    Entry.Position = Position;
    Entry.SizePixels = Size;

    Entry.Mode = SEntryMode{ MAP_MODE_NORMAL };

    Queue2D.Enqueue(Entry);
}

void SRenderer::DrawMapImmediate(const SVec2& Position, const SVec2& Size)
{
    ProgramMap.Use();

    glUniform1i(ProgramMap.UniformModeID, MAP_MODE_NORMAL);
    glUniform2f(ProgramMap.UniformPositionScreenSpaceID, Position.X, Position.Y);
    glUniform2f(ProgramMap.UniformSizeScreenSpaceID, Size.X, Size.Y);

    glBindVertexArray(Quad2D.VAO);

    glDrawElements(GL_TRIANGLES, Quad2D.ElementCount, GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);
}

void SRenderer::DrawWorldMap(const SVec2& Position, const SVec2& Size)
{
    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::Map;
    Entry.Position = SVec3(Position);
    Entry.SizePixels = Size;

    Entry.Mode = SEntryMode{ MAP_MODE_WORLD };

    Queue2D.Enqueue(Entry);
}

void SRenderer::DrawWorldMapImmediate(const SVec2& Position, const SVec2& Size)
{
    ProgramMap.Use();

    glUniform1i(ProgramMap.UniformModeID, MAP_MODE_WORLD);
    glUniform2f(ProgramMap.UniformPositionScreenSpaceID, Position.X, Position.Y);
    glUniform2f(ProgramMap.UniformSizeScreenSpaceID, Size.X, Size.Y);

    glBindVertexArray(Quad2D.VAO);

    glDrawElements(GL_TRIANGLES, Quad2D.ElementCount, GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);
}

void SRenderer::DrawWorldLayers(const SWorld* World, SVec2Int Range)
{
    SShaderWorld ShaderWorld{};

    glBindFramebuffer(GL_FRAMEBUFFER, WorldLayersFramebuffer.FBO);

    glBindVertexArray(Quad2D.VAO);

    ProgramMap.SetEditorData(SVec2(), SVec4(), true, false, false);
    ProgramMap.Use();
    glUniform1i(ProgramMap.UniformModeID, MAP_MODE_WORLD_LAYER);
    glUniform2f(ProgramMap.UniformPositionScreenSpaceID, 0.0f, 0.0f);

    int LayerIndex{};
    for (auto LevelIndex = Range.X; LevelIndex < Range.Y; LevelIndex++)
    {
        auto Level = &World->Levels[LevelIndex];
        if (Level == nullptr)
        {
            continue;
        }
        WorldLayersFramebuffer.SetLayer(LayerIndex);

        auto Size = Level->CalculateMapIsoSize();

        GlobalsUniformBlock.SetVector2(offsetof(SShaderGlobals, ScreenSize), SVec2(Size));

        glViewport(0, 0, Size.X, Size.Y);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        UploadMapData(Level, {});

        glUniform2f(ProgramMap.UniformSizeScreenSpaceID, (float)Size.X, (float)Size.Y);

        glDrawElements(GL_TRIANGLES, Quad2D.ElementCount, GL_UNSIGNED_SHORT, nullptr);

        ShaderWorld.Layers[LayerIndex].Index = LayerIndex;
        ShaderWorld.Layers[LayerIndex].TextureSize = Size;
        ShaderWorld.Layers[LayerIndex].Color = Level->Color;

        LayerIndex++;
    }

    ProgramMap.SetEditorData(SVec2(), SVec4(), false, false, false);

    glBindVertexArray(0);

    glBindBuffer(GL_UNIFORM_BUFFER, ProgramMap.UniformBlockWorld.UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SShaderWorld), &ShaderWorld);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SRenderer::Draw2D(SVec3 Position, const SSpriteHandle& SpriteHandle)
{
    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::Uber2D;
    Entry.Position = Position;
    Entry.SizePixels = SpriteHandle.Sprite->SizePixels;
    Entry.UVRect = SpriteHandle.Sprite->UVRect;

    Queue2D.Enqueue(Entry);
}

void SRenderer::Draw2DEx(SVec3 Position, const SSpriteHandle& SpriteHandle, int Mode, SVec4 ModeControlA)
{
    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::Uber2D;
    Entry.Position = Position;
    Entry.SizePixels = SpriteHandle.Sprite->SizePixels;
    Entry.UVRect = SpriteHandle.Sprite->UVRect;

    Entry.Mode = SEntryMode{ Mode, ModeControlA };

    Queue2D.Enqueue(Entry);
}

void SRenderer::Draw2DEx(SVec3 Position, const SSpriteHandle& SpriteHandle, int Mode, SVec4 ModeControlA,
    SVec4 ModeControlB)
{
    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::Uber2D;
    Entry.Position = Position;
    Entry.SizePixels = SpriteHandle.Sprite->SizePixels;
    Entry.UVRect = SpriteHandle.Sprite->UVRect;

    Entry.Mode = SEntryMode{ Mode, ModeControlA, ModeControlB };

    Queue2D.Enqueue(Entry);
}

void SRenderer::Draw2DHaze(SVec3 Position, const SSpriteHandle& SpriteHandle, float XIntensity, float YIntensity,
    float Speed)
{

    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::Uber2D;
    Entry.Position = Position;
    Entry.SizePixels = SpriteHandle.Sprite->SizePixels;
    Entry.UVRect = SpriteHandle.Sprite->UVRect;

    Entry.Mode = SEntryMode{ UBER2D_MODE_HAZE, { XIntensity, YIntensity, Speed, 0.0f } };

    Queue2D.Enqueue(Entry);
}

void SRenderer::Draw2DBackBlur(SVec3 Position, const SSpriteHandle& SpriteHandle, float Count, float Speed, float Step)
{
    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::Uber2D;
    Entry.Position = Position;
    Entry.SizePixels = SpriteHandle.Sprite->SizePixels;
    Entry.UVRect = SpriteHandle.Sprite->UVRect;

    Entry.Mode = SEntryMode{ UBER2D_MODE_BACK_BLUR, { Count, Speed, Step, 0.0f } };

    Queue2D.Enqueue(Entry);
}

void SRenderer::Draw2DGlow(SVec3 Position, const SSpriteHandle& SpriteHandle, SVec3 Color, float Intensity)
{
    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::Uber2D;
    Entry.Position = Position;
    Entry.SizePixels = SpriteHandle.Sprite->SizePixels;
    Entry.UVRect = SpriteHandle.Sprite->UVRect;

    Entry.Mode = SEntryMode{ UBER2D_MODE_GLOW, { Color, Intensity } };

    Queue2D.Enqueue(Entry);
}

void SRenderer::Draw2DDisintegrate(SVec3 Position, const SSpriteHandle& SpriteHandle, const SSpriteHandle& NoiseHandle,
    float Progress)
{
    SEntry2D Entry;
    Entry.Program2DType = EProgram2DType::Uber2D;
    Entry.Position = Position;
    Entry.SizePixels = SpriteHandle.Sprite->SizePixels;
    Entry.UVRect = SpriteHandle.Sprite->UVRect;

    Entry.Mode = SEntryMode{
        UBER2D_MODE_DISINTEGRATE,
        { Progress, 0.0f, 0.0f, 0.0f },
        { NoiseHandle.Sprite->UVRect }
    };

    Queue2D.Enqueue(Entry);
}

void SRenderer::Draw3D(SVec3 Position, SGeometry* Geometry)
{
    SEntry3D Entry;

    Entry.Geometry = Geometry;
    Entry.Model = SMat4x4::Identity();
    Entry.Model.Translate(Position);

    Entry.Mode = SEntryMode{
        UBER3D_MODE_BASIC
    };

    Queue3D.Enqueue(Entry);
}

void SRenderer::Draw3DLevel(SWorldLevel* Level, const SVec2Int& POVOrigin, const SDirection& POVDirection)
{
    auto constexpr DrawDistanceForward = 4;
    auto constexpr DrawDistanceSide = 2;

    auto& DoorDrawCall = LevelDrawData.DrawCalls[ETileGeometryType::Door];

    /* @TODO: Generic CleanDynamic method? */
    DoorDrawCall.DynamicCount = 0;

    if (Level->DirtyFlags & ELevelDirtyFlags::DrawSet)
    {
        LevelDrawData.Clear();

        auto& FloorDrawCall = LevelDrawData.DrawCalls[ETileGeometryType::Floor];

        auto& HoleDrawCall = LevelDrawData.DrawCalls[ETileGeometryType::Hole];

        auto& WallDrawCall = LevelDrawData.DrawCalls[ETileGeometryType::Wall];

        auto& WallJointDrawCall = LevelDrawData.DrawCalls[ETileGeometryType::WallJoint];

        auto& DoorFrameDrawCall = LevelDrawData.DrawCalls[ETileGeometryType::DoorFrame];

        if (!Level->IsValidTile(POVOrigin))
        {
            return;
        }

        auto POVDirectionInverted = POVDirection.Inverted();
        auto POVDirectionVectorForward = POVDirection.GetVector<int>();
        auto POVDirectionVectorSide = SVec2Int{ POVDirectionVectorForward.Y, -POVDirectionVectorForward.X };

        for (int SideCounter = -DrawDistanceSide; SideCounter <= DrawDistanceSide; ++SideCounter)
        {
            for (int ForwardCounter = -1; ForwardCounter < DrawDistanceForward; ++ForwardCounter)
            {
                auto RelativeX = (POVDirectionVectorForward.X * ForwardCounter) + (POVDirectionVectorSide.X * SideCounter);
                auto RelativeY = (POVDirectionVectorForward.Y * ForwardCounter) + (POVDirectionVectorSide.Y * SideCounter);

                auto X = POVOrigin.X + RelativeX;
                auto Y = POVOrigin.Y + RelativeY;

                auto XOffset = (float)X;
                auto YOffset = (float)Y;

                auto TileCoords = SVec2Int{ X, Y };

                auto TileTransform = SMat4x4::Identity();
                TileTransform.Translate(SVec3{ XOffset, 0.0f, YOffset });

                /* @TODO: Draw joints in separate loop. */
                /* @TODO: Maybe don't store them at all? */
                if (Level->bUseWallJoints && Level->IsValidWallJoint({ X, Y }))
                {
                    auto bWallJoint = Level->IsWallJointAt({ X, Y });
                    if (bWallJoint)
                    {
                        WallJointDrawCall.Push(TileTransform);
                    }
                }

                auto Tile = Level->GetTileAt(TileCoords);

                if (Tile == nullptr)
                {
                    continue;
                }

                if (SideCounter < -1 && ForwardCounter == 0 && !Tile->IsEdgeEmpty(POVDirection.Side().Inverted()))
                {
                    continue;
                }

                if (SideCounter > 1 && ForwardCounter == 0 && !Tile->IsEdgeEmpty(POVDirection.Side()))
                {
                    continue;
                }

                if (SideCounter == 0 && ForwardCounter >= 1 && !Tile->IsEdgeEmpty(POVDirection.Inverted()))
                {
                    break;
                }

                if (Tile->CheckFlag(TILE_FLOOR_BIT))
                {
                    FloorDrawCall.Push(TileTransform);
                }
                else if (Tile->CheckFlag(TILE_HOLE_BIT))
                {
                    HoleDrawCall.Push(TileTransform);
                }

                for (auto& Direction : SDirection::All())
                {
                    if (Tile->IsEdgeEmpty(Direction))
                    {
                        continue;
                    }

                    auto Transform = TileTransform;
                    Transform.Rotate(Direction.RotationFromDirection(),
                        { 0.0f, 1.0f, 0.0f });

                    if (Tile->CheckEdgeFlag(TILE_EDGE_WALL_BIT, Direction))
                    {
                        WallDrawCall.Push(Transform);
                    }

                    if (Tile->CheckEdgeFlag(TILE_EDGE_DOOR_BIT, Direction))
                    {
                        DoorFrameDrawCall.Push(Transform);

                        /* Check two adjacent tiles for ongoing door animation.
                         * Prevents static doors from being drawn if the animation is playing. */
                        if (Level->DoorInfo.Timeline.IsPlaying())
                        {
                            if (TileCoords == Level->DoorInfo.TileCoords && Direction == Level->DoorInfo.Direction)
                            {
                                continue;
                            }
                            if (TileCoords == POVOrigin && Direction == POVDirectionInverted)
                            {
                                continue;
                            }
                        }

                        Draw3DLevelDoor(DoorDrawCall, TileCoords, Direction, -1.0f);
                    }
                }
            }
        }
        Level->DirtyFlags &= ~ELevelDirtyFlags::DrawSet;

        Log::Draw<ELogLevel::Debug>("%s(): Regenerated Level Draw Set", __func__);
    }

    Draw3DLevelDoor(
        DoorDrawCall,
        Level->DoorInfo.TileCoords,
        Level->DoorInfo.Direction,
        Level->DoorInfo.Timeline.Value);

    SEntry3D Entry;

    Entry.Geometry = LevelDrawData.TileSet;
    Entry.Model = SMat4x4::Identity();
    Entry.InstancedDrawCall = &LevelDrawData.DrawCalls[0];
    Entry.InstancedDrawCallCount = ETileGeometryType::Count;

    Entry.Mode = SEntryMode{
        UBER3D_MODE_LEVEL
    };

    Queue3D.Enqueue(Entry);
}

void SRenderer::Draw3DLevelDoor(SInstancedDrawCall& DoorDrawCall, const SVec2Int& TileCoords, SDirection Direction, float AnimationAlpha) const
{
    if (TileCoords.X + TileCoords.Y < 0)
    {
        return;
    }

    auto Temp = SDirection{ Direction }.GetVector<float>();
    SVec3 DirectionalOffset{};
    DirectionalOffset.X = Temp.X;
    DirectionalOffset.Z = Temp.Y;

    auto TileCoordsOffset = SVec3{ (float)TileCoords.X, 0.0f, (float)TileCoords.Y };

    SMat4x4 Transform = SMat4x4::Identity();
    Transform.Translate((DirectionalOffset * 0.5f) + TileCoordsOffset);

    switch (LevelDrawData.TileSet->DoorAnimationType)
    {
        case EDoorAnimationType::TwoDoors:
        {
            /* Right door. */
            auto RightTransform = Transform;

            RightTransform.Rotate(SDirection{ Direction }.Inverted().RotationFromDirection(), { 0.0f, 1.0f, 0.0f });
            RightTransform.Translate({ LevelDrawData.TileSet->DoorOffset, 0.0f, 0.0f });

            /* Left door. */
            auto LeftTransform = Transform;

            LeftTransform.Rotate(Math::PI, { 0.0f, 1.0f, 0.0f });
            LeftTransform.Rotate(SDirection{ Direction }.Inverted().RotationFromDirection(), { 0.0f, 1.0f, 0.0f });
            LeftTransform.Translate({ LevelDrawData.TileSet->DoorOffset, 0.0f, 0.0f });

            /* Animate relevant doors. */
            if (AnimationAlpha > 0.0f)
            {
                auto A = AnimationAlpha * Math::HalfPI;

                LeftTransform.Rotate(A);
                //                A = std::max(0.0f, A - 0.4f);
                //                A *= 1.35f;
                RightTransform.Rotate(-A);

                DoorDrawCall.PushDynamic(LeftTransform);
                DoorDrawCall.PushDynamic(RightTransform);
            }
            else if (AnimationAlpha < 0.0f)
            {
                DoorDrawCall.Push(LeftTransform);
                DoorDrawCall.Push(RightTransform);
            }
        }
        break;

        default:
            break;
    }
}

void STexture::InitFromPixels(int Width, int Height, bool bAlpha, const void* Pixels)
{
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexImage2D(GL_TEXTURE_2D, 0, bAlpha ? GL_RGBA : GL_RGB, Width, Height, 0, bAlpha ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, Pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void STexture::InitEmpty(int Width, int Height, bool bAlpha)
{
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexImage2D(GL_TEXTURE_2D, 0, bAlpha ? GL_RGBA : GL_RGB, Width, Height, 0, bAlpha ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void STexture::InitFromRawImage(const CRawImage& RawImage)
{
    InitFromPixels(RawImage.Width, RawImage.Height, RawImage.Channels == 4, RawImage.Data);
}

void STexture::Cleanup()
{
    glDeleteTextures(1, &ID);

    Log::Draw<ELogLevel::Debug>("Deleting STexture", "");
}

void SAtlas::Init(int InTextureUnitID)
{
    TextureUnitID = InTextureUnitID;
    InitEmpty(WidthAndHeight, WidthAndHeight, true);
    std::iota(SortingIndices.begin(), SortingIndices.end(), 0);
}

SSpriteHandle SAtlas::AddSprite(const SAsset& Resource)
{
    CRawImageInfo const RawImageInfo(Resource);

    Sprites[CurrentIndex].SizePixels = { RawImageInfo.Width, RawImageInfo.Height };
    Sprites[CurrentIndex].Resource = &Resource;
    return { this, &Sprites[CurrentIndex++] };
}

void SAtlas::Build()
{
    glActiveTexture(GL_TEXTURE0 + TextureUnitID);
    glBindTexture(GL_TEXTURE_2D, ID);

    std::sort(SortingIndices.begin(), SortingIndices.end(), [&](const int& IndexA, const int& IndexB) {
        return Sprites[IndexA].SizePixels.Y > Sprites[IndexB].SizePixels.Y;
    });

    int CursorX{};
    int CursorY{};
    int MaxHeight{};

    for (int Index = 0; Index < CurrentIndex; ++Index)
    {
        auto& Sprite = Sprites[SortingIndices[Index]];
        CRawImage const Image(*Sprite.Resource);

        if (CursorX + Image.Width > WidthAndHeight)
        {
            CursorY += MaxHeight;
            CursorX = 0;
            MaxHeight = 0;
        }

        if (CursorY + Image.Height > WidthAndHeight)
        {
            break;
        }

        float MinU = (float)(CursorX) / (float)(WidthAndHeight);
        float MinV = (float)(CursorY) / (float)(WidthAndHeight);
        float MaxU = MinU + ((float)(Image.Width) / (float)WidthAndHeight);
        float MaxV = MinV + ((float)(Image.Height) / (float)WidthAndHeight);
        Sprite.UVRect = { MinU, MinV, MaxU, MaxV };

        glTexSubImage2D(GL_TEXTURE_2D, 0,
            CursorX, CursorY,
            Image.Width, Image.Height,
            GL_RGBA,
            GL_UNSIGNED_BYTE, Image.Data);

        CursorX += Image.Width;

        if (Image.Height > MaxHeight)
        {
            MaxHeight = Image.Height;
        }
    }

    glActiveTexture(GL_TEXTURE0);
}
