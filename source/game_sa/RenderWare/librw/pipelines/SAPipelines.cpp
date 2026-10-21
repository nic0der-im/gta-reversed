#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "SAPipelines.h"

#include "../RwFakerw.h"

#include "CustomBuildingDNPipeline.h"
#include "CustomCarEnvMapPipeline.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

//! The four San Andreas object pipelines on librw's GL3 backend.
//!
//! Geometry passed in is the game's own `RpGeometry` (fakerw keeps the plugin-sdk structs
//! authoritative), so the instancing code reads positions/normals/prelit colours/texture
//! coordinates straight from it and uploads them into a `rw::gl3::InstanceDataHeader`.
//! Drawing goes through librw's exported `gl3::` helpers so render states, fog, lights,
//! alpha test and uniforms behave exactly like librw's own pipelines.

namespace notsa::native::sapipelines {

namespace {

constexpr uint32 kMaxBones           = 64;
constexpr uint32 kNightColourAttrib  = 13; //!< not used by librw: external night colours
constexpr uint32 kNumKinds           = 4;

//-----------------------------------------------------------------------------
// Shader sources
//-----------------------------------------------------------------------------

//! Building (`CCustomBuildingPipeline`), and with `SA_DAYNIGHT` also the DN variant.
const char* kBuildingVert = R"GLSL(
VSIN(ATTRIB_POS)	vec3 in_pos;

VSOUT vec4 v_color;
VSOUT vec2 v_tex0;
VSOUT float v_fog;

uniform vec4 u_saFlags;		// x: dynamic lighting enabled
uniform vec4 u_saDNBalance;	// x: 0 = day, 1 = night

#ifdef SA_DAYNIGHT
VSIN(13)	vec4 in_night;
#endif

void
main(void)
{
	vec4 Vertex = u_world*vec4(in_pos, 1.0);
	gl_Position = u_proj*u_view*Vertex;

	vec4 color = in_color;
#ifdef SA_DAYNIGHT
	color = mix(in_color, in_night, clamp(u_saDNBalance.x, 0.0, 1.0));
#endif

	if(u_saFlags.x > 0.5){
		vec3 Normal = mat3(u_world)*in_normal;
		color.rgb += u_ambLight.rgb*surfAmbient;
		color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
	}

	v_color = clamp(color, 0.0, 1.0)*u_matColor;
	v_tex0 = in_tex0;
	v_fog = DoFog(gl_Position.w);
}
)GLSL";

const char* kBuildingFrag = R"GLSL(
uniform sampler2D tex0;

FSIN vec4 v_color;
FSIN vec2 v_tex0;
FSIN float v_fog;

void
main(void)
{
	vec4 color = v_color*texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));
	color.rgb = mix(u_fogColor.rgb, color.rgb, v_fog);
	DoAlphaTest(color.a);
	FRAGCOLOR(color);
}
)GLSL";

//! Vehicle (`CCustomCarEnvMapPipeline`): diffuse * prelit, optional env map (camera projected
//! or scrolling second texture coordinate set) and a Blinn-Phong specular term.
const char* kVehicleVert = R"GLSL(
VSIN(ATTRIB_POS)	vec3 in_pos;

VSOUT vec4 v_color;
VSOUT vec2 v_tex0;
VSOUT vec2 v_tex1;
VSOUT float v_fog;
VSOUT float v_spec;

uniform vec4 u_saFlags;			// x: lighting, z: env mode (0 none, 1 camera, 2 wave), w: specular
uniform vec4 u_saEnvMap;		// xy: scale, zw: uv offset
uniform vec4 u_saSpecular;		// x: strength, y: power, z: env shininess

uniform mat4 u_saEnvMapMatrix;

void
main(void)
{
	vec4 Vertex = u_world*vec4(in_pos, 1.0);
	gl_Position = u_proj*u_view*Vertex;

	vec3 Normal = mat3(u_world)*in_normal;

	vec4 color = in_color;
	if(u_saFlags.x > 0.5){
		color.rgb += u_ambLight.rgb*surfAmbient;
		color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
	}
	v_color = clamp(color, 0.0, 1.0)*u_matColor;

	vec3 nv = normalize(mat3(u_view)*Normal);
	if(u_saFlags.z > 1.5){
		// Scrolling reflection: the geometry's second UV set + offset (retail "EnvWave")
		v_tex1 = in_tex1 + u_saEnvMap.zw;
	}else{
		// Camera space normal projected through the env-map matrix
		v_tex1 = (u_saEnvMapMatrix*vec4(nv, 1.0)).xy*u_saEnvMap.xy + u_saEnvMap.zw;
	}

	vec3 L = vec3(0.0, 0.0, 1.0);
	if(u_lightParams[0].x == 1.0)
		L = normalize(mat3(u_view)*(-u_lightDirection[0].xyz));
	vec3 V = normalize((u_view*vec4(Vertex.xyz, 1.0)).xyz);
	vec3 H = normalize(L + normalize(-V));
	v_spec = u_saFlags.w > 0.5
		? pow(max(dot(nv, H), 0.0), max(u_saSpecular.y, 1.0))*u_saSpecular.x
		: 0.0;

	v_tex0 = in_tex0;
	v_fog = DoFog(gl_Position.w);
}
)GLSL";

const char* kVehicleFrag = R"GLSL(
uniform sampler2D tex0;
uniform sampler2D tex1;

uniform vec4 u_saFlags;				// x: lighting, z: env mode, w: specular
uniform vec4 u_saEnvMap;			// xy: scale, zw: uv offset
uniform vec4 u_saSpecular;			// z: env shininess
uniform vec4 u_saWetRoad;			// x: 0..1
uniform vec4 u_saVehicleColours[4];	// paint colours resolved from carcols.dat

FSIN vec4 v_color;
FSIN vec2 v_tex0;
FSIN vec2 v_tex1;
FSIN float v_fog;
FSIN float v_spec;

void
main(void)
{
	vec4 color = v_color*texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));

	if(u_saFlags.z > 0.5){
		vec3 refl = texture(tex1, vec2(v_tex1.x, 1.0-v_tex1.y)).rgb;
		float wet = mix(0.7, 1.0, clamp(u_saWetRoad.x, 0.0, 1.0));
		refl *= mix(vec3(1.0), u_saVehicleColours[0].rgb, 0.25);
		color.rgb += refl*u_saSpecular.z*wet;
	}
	color.rgb += v_spec;

	color.rgb = mix(u_fogColor.rgb, color.rgb, v_fog);
	DoAlphaTest(color.a);
	FRAGCOLOR(color);
}
)GLSL";

//! Skinned peds with specular. Without skin data the vertex weights are filled with
//! (1,0,0,0) and the single bone matrix is identity, which degenerates to a rigid transform.
const char* kPedVert = R"GLSL(
uniform mat4 u_boneMatrices[64];

uniform vec4 u_saFlags;		// x: dynamic lighting enabled
uniform vec4 u_saSpecular;	// x: strength, y: power

VSIN(ATTRIB_POS)	vec3 in_pos;

VSOUT vec4 v_color;
VSOUT vec2 v_tex0;
VSOUT float v_fog;
VSOUT float v_spec;

void
main(void)
{
	vec3 skinVertex = vec3(0.0, 0.0, 0.0);
	vec3 skinNormal = vec3(0.0, 0.0, 0.0);
	for(int i = 0; i < 4; i++){
		int b = int(in_indices[i]);
		skinVertex += (u_boneMatrices[b]*vec4(in_pos, 1.0)).xyz*in_weights[i];
		skinNormal += (mat3(u_boneMatrices[b])*in_normal)*in_weights[i];
	}

	vec4 Vertex = u_world*vec4(skinVertex, 1.0);
	gl_Position = u_proj*u_view*Vertex;

	vec3 Normal = mat3(u_world)*skinNormal;

	vec4 color = in_color;
	if(u_saFlags.x > 0.5){
		color.rgb += u_ambLight.rgb*surfAmbient;
		color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
	}
	v_color = clamp(color, 0.0, 1.0)*u_matColor;

	vec3 nv = normalize(mat3(u_view)*Normal);
	vec3 L = vec3(0.0, 0.0, 1.0);
	if(u_lightParams[0].x == 1.0)
		L = normalize(mat3(u_view)*(-u_lightDirection[0].xyz));
	vec3 V = normalize((u_view*vec4(Vertex.xyz, 1.0)).xyz);
	vec3 H = normalize(L + normalize(-V));
	v_spec = u_saSpecular.x > 0.0
		? pow(max(dot(nv, H), 0.0), max(u_saSpecular.y, 1.0))*u_saSpecular.x
		: 0.0;

	v_tex0 = in_tex0;
	v_fog = DoFog(gl_Position.w);
}
)GLSL";

const char* kPedFrag = R"GLSL(
uniform sampler2D tex0;

FSIN vec4 v_color;
FSIN vec2 v_tex0;
FSIN float v_fog;
FSIN float v_spec;

void
main(void)
{
	vec4 color = v_color*texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));
	color.rgb += v_spec;
	color.rgb = mix(u_fogColor.rgb, color.rgb, v_fog);
	DoAlphaTest(color.a);
	FRAGCOLOR(color);
}
)GLSL";

//-----------------------------------------------------------------------------
// State
//-----------------------------------------------------------------------------

struct Pipeline {
    rw::gl3::Shader* shader{};     //!< alpha test enabled
    rw::gl3::Shader* shaderNoAT{}; //!< `NO_ALPHATEST` variant
};

Pipeline s_pipelines[kNumKinds];
bool     s_registered = false;
int32    s_shaderUniformCount = -1; //!< `uniformRegistry.numUniforms` the programs were built with

int32 s_uFlags          = -1;
int32 s_uDNBalance      = -1;
int32 s_uEnvMap         = -1;
int32 s_uEnvMapMatrix   = -1;
int32 s_uSpecular       = -1;
int32 s_uWetRoad        = -1;
int32 s_uVehicleColours = -1;
int32 s_uBones          = -1;

//! Column-major, identity by default.
float s_envMapMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

//-----------------------------------------------------------------------------
// Per-geometry GL buffers
//-----------------------------------------------------------------------------

struct MeshEntry {
    RpMaterial* material;
    bool32      vertexAlpha;
};

struct GeometryCache {
    rw::gl3::InstanceDataHeader header{};
    std::vector<MeshEntry>      meshes;
    Kind                        kind{};
    uint32                      meshSerial{};
    uint32                      vertexStride{};
    bool                        hasTexCoord1{}; //!< second UV set uploaded (EnvWave)
    bool                        valid{};
};

std::unordered_map<RpGeometry*, GeometryCache> s_cache;

void DestroyCache(GeometryCache& cache) {
    auto& header = cache.header;
    if (header.ibo) {
        glDeleteBuffers(1, &header.ibo);
    }
    if (header.vbo) {
        glDeleteBuffers(1, &header.vbo);
    }
    delete[] header.indexBuffer;
    delete[] header.vertexBuffer;
    delete[] header.attribDesc;
    delete[] header.inst;
    cache = {};
}

//! `PluginRegistry::Register` returns an offset at least as large as the base object;
//! `StaticRef` storage starts zeroed (and the exe's initial value is -1), so both are rejected.
bool PluginAttached(uint32 offset, size_t baseSize) {
    return offset >= baseSize && offset < 4096;
}

bool NightColoursAttached() {
    return PluginAttached(CCustomBuildingDNPipeline::ms_extraVertColourPluginOffset, sizeof(RpGeometry));
}

bool EnvMapPluginAttached() {
    return PluginAttached((uint32)CCustomCarEnvMapPipeline::ms_envMapPluginOffset, sizeof(RpMaterial));
}

bool SpecMapPluginAttached() {
    return PluginAttached((uint32)CCustomCarEnvMapPipeline::ms_specularMapPluginOffset, sizeof(RpMaterial));
}

const ExtraVertColour* GetExtraVertColour(const RpGeometry* geometry) {
    if (!geometry || !NightColoursAttached()) {
        return nullptr;
    }
    return CCustomBuildingDNPipeline::GetExtraVertColourPtr(geometry);
}

//-----------------------------------------------------------------------------
// Shader creation
//-----------------------------------------------------------------------------

GLuint CompileStage(const char* debugName, GLenum type, const char* const* src) {
    GLuint shader = glCreateShader(type);
    if (!shader) {
        NOTSA_LOG_ERR("[SAPipelines] glCreateShader failed for {}", debugName);
        return 0;
    }

    int32 count = 0;
    while (src[count]) {
        count++;
    }
    glShaderSource(shader, count, src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(length > 1 ? length - 1 : 0, '\0');
        if (length > 1) {
            glGetShaderInfoLog(shader, length, nullptr, log.data());
        }
        NOTSA_LOG_ERR(
            "[SAPipelines] {} {} shader failed to compile:\n{}",
            debugName,
            type == GL_VERTEX_SHADER ? "vertex" : "fragment",
            log
        );
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

//! Compiles and links a program and wraps it in a librw `gl3::Shader` (logging failures
//! together with the driver's info log).
rw::gl3::Shader* CreateShader(const char* debugName, const char* const* vsrc, const char* const* fsrc) {
    const GLuint vs = CompileStage(debugName, GL_VERTEX_SHADER, vsrc);
    if (!vs) {
        return nullptr;
    }
    const GLuint fs = CompileStage(debugName, GL_FRAGMENT_SHADER, fsrc);
    if (!fs) {
        glDeleteShader(vs);
        return nullptr;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(length > 1 ? length - 1 : 0, '\0');
        if (length > 1) {
            glGetProgramInfoLog(program, length, nullptr, log.data());
        }
        NOTSA_LOG_ERR("[SAPipelines] {} program failed to link:\n{}", debugName, log);
        glDeleteProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return nullptr;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    auto* shader = rwNewT(rw::gl3::Shader, 1, (uint32)rw::MEMDUR_EVENT | (uint32)rw::ID_DRIVER);
    shader->program    = program;
    shader->numUniforms = rw::gl3::uniformRegistry.numUniforms;
    shader->uniformLocations = rwNewT(GLint, shader->numUniforms, (uint32)rw::MEMDUR_EVENT | (uint32)rw::ID_DRIVER);
    shader->serialNums       = rwNewT(uint32, shader->numUniforms, (uint32)rw::MEMDUR_EVENT | (uint32)rw::ID_DRIVER);
    for (int32 i = 0; i < shader->numUniforms; i++) {
        shader->uniformLocations[i] = glGetUniformLocation(program, rw::gl3::uniformRegistry.uniforms[i].name);
        shader->serialNums[i]       = ~0u; // force an upload on the first draw
    }

    // Uniform block bindings (`USE_UBOS` is off in librw's shaders, but keep the registry sane).
    for (int32 i = 0; i < rw::gl3::uniformRegistry.numBlocks; i++) {
        const GLint index = glGetUniformBlockIndex(program, rw::gl3::uniformRegistry.blockNames[i]);
        if (index >= 0) {
            glUniformBlockBinding(program, index, i);
        }
    }

    // Sampler uniforms: textures always go through librw's stage assignment.
    glUseProgram(program);
    for (int32 i = 0; i < 4; i++) {
        char name[16];
        std::snprintf(name, sizeof(name), "tex%d", i);
        const GLint location = glGetUniformLocation(program, name);
        if (location >= 0) {
            glUniform1i(location, i);
        }
    }
    if (rw::gl3::currentShader) {
        glUseProgram(rw::gl3::currentShader->program);
    }

    return shader;
}

rw::gl3::Shader* CreateShaderPair(
    const char* debugName,
    const char* vertSrc,
    const char* fragSrc,
    const char* defines,
    rw::gl3::Shader** outNoAT
) {
    const char* vs[] = { rw::gl3::shaderDecl, defines, rw::gl3::header_vert_src, vertSrc, nullptr };
    const char* fs[] = { rw::gl3::shaderDecl, defines, rw::gl3::header_frag_src, fragSrc, nullptr };
    const char* fsNoAT[] = { rw::gl3::shaderDecl, defines, "#define NO_ALPHATEST\n", rw::gl3::header_frag_src, fragSrc, nullptr };

    auto* shader = CreateShader(debugName, vs, fs);
    *outNoAT     = CreateShader(debugName, vs, fsNoAT);
    return shader;
}

//-----------------------------------------------------------------------------
// Instancing over the game's `RpGeometry`
//-----------------------------------------------------------------------------

struct SkinInfo {
    const RwMatrixWeights* weights{};
    const RwUInt32*        indices{};
};

SkinInfo GetSkinInfo(RpGeometry* geometry) {
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);
    if (!skin) {
        return {};
    }
    return { RpSkinGetVertexBoneWeights(skin), RpSkinGetVertexBoneIndices(skin) };
}

void FillVertexBuffer(RpGeometry* geometry, GeometryCache& cache, Kind kind) {
    auto& header = cache.header;
    auto* meshTarget = geometry->morphTarget ? &geometry->morphTarget[0] : nullptr;
    if (!meshTarget || !meshTarget->verts) {
        return;
    }

    const uint32 stride = cache.vertexStride;
    const int32  count  = geometry->numVertices;
    uint8*       verts  = header.vertexBuffer;

    const SkinInfo skin = kind == Kind::Ped ? GetSkinInfo(geometry) : SkinInfo{};
    const ExtraVertColour* extraColours = kind == Kind::BuildingDayNight ? GetExtraVertColour(geometry) : nullptr;

    for (int32 a = 0; a < header.numAttribs; a++) {
        const rw::gl3::AttribDesc& attrib = header.attribDesc[a];
        uint8* dst = verts + attrib.offset;

        switch (attrib.index) {
        case rw::gl3::ATTRIB_POS:
            for (int32 i = 0; i < count; i++) {
                std::memcpy(dst + i * stride, &meshTarget->verts[i], sizeof(RwV3d));
            }
            break;

        case rw::gl3::ATTRIB_NORMAL:
            for (int32 i = 0; i < count; i++) {
                float* out = reinterpret_cast<float*>(dst + i * stride);
                if (meshTarget->normals) {
                    out[0] = meshTarget->normals[i].x;
                    out[1] = meshTarget->normals[i].y;
                    out[2] = meshTarget->normals[i].z;
                } else {
                    out[0] = 0.0f;
                    out[1] = 0.0f;
                    out[2] = 1.0f;
                }
            }
            break;

        case rw::gl3::ATTRIB_COLOR: {
            // The DN variant must start from the baked *day* colours: the game's
            // `NVCPipelineProcess` also rewrites `preLitLum` with the blended result.
            const RwRGBA* source = geometry->preLitLum;
            if (extraColours && extraColours->DayColors) {
                source = extraColours->DayColors;
            }
            for (int32 i = 0; i < count; i++) {
                uint8* out = dst + i * stride;
                if (source) {
                    out[0] = source[i].red;
                    out[1] = source[i].green;
                    out[2] = source[i].blue;
                    out[3] = source[i].alpha;
                } else {
                    out[0] = out[1] = out[2] = out[3] = 0xFF;
                }
            }
            break;
        }

        case rw::gl3::ATTRIB_TEXCOORDS0:
        case rw::gl3::ATTRIB_TEXCOORDS1: {
            const int32 set = attrib.index == rw::gl3::ATTRIB_TEXCOORDS0 ? 0 : 1;
            const RwTexCoords* tex = geometry->texCoords[set];
            for (int32 i = 0; i < count; i++) {
                float* out = reinterpret_cast<float*>(dst + i * stride);
                out[0] = tex ? tex[i].u : 0.0f;
                out[1] = tex ? tex[i].v : 0.0f;
            }
            break;
        }

        case rw::gl3::ATTRIB_WEIGHTS:
            for (int32 i = 0; i < count; i++) {
                float* out = reinterpret_cast<float*>(dst + i * stride);
                if (skin.weights) {
                    std::memcpy(out, &skin.weights[i], sizeof(RwMatrixWeights));
                } else {
                    out[0] = 1.0f;
                    out[1] = out[2] = out[3] = 0.0f;
                }
            }
            break;

        case rw::gl3::ATTRIB_INDICES:
            for (int32 i = 0; i < count; i++) {
                uint8* out = dst + i * stride;
                if (skin.indices) {
                    std::memcpy(out, &skin.indices[i], sizeof(RwUInt32));
                } else {
                    std::memset(out, 0, sizeof(RwUInt32));
                }
            }
            break;

        case kNightColourAttrib:
            for (int32 i = 0; i < count; i++) {
                uint8* out = dst + i * stride;
                const RwRGBA* night = extraColours ? extraColours->NightColors : nullptr;
                if (night) {
                    out[0] = night[i].red;
                    out[1] = night[i].green;
                    out[2] = night[i].blue;
                    out[3] = night[i].alpha;
                } else if (geometry->preLitLum) { // no night colours: day == night
                    out[0] = geometry->preLitLum[i].red;
                    out[1] = geometry->preLitLum[i].green;
                    out[2] = geometry->preLitLum[i].blue;
                    out[3] = geometry->preLitLum[i].alpha;
                } else {
                    out[0] = out[1] = out[2] = out[3] = 0xFF;
                }
            }
            break;

        default:
            break;
        }
    }
}

void UploadVertexBuffer(GeometryCache& cache, int32 vertexCount) {
    auto& header = cache.header;
    glBindBuffer(GL_ARRAY_BUFFER, header.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        (GLsizeiptr)vertexCount * cache.vertexStride,
        header.vertexBuffer,
        GL_STATIC_DRAW
    );
}

void Build(RpGeometry* geometry, GeometryCache& cache, Kind kind) {
    DestroyCache(cache);

    RpMeshHeader* meshHeader = geometry->mesh;
    if (!meshHeader || !meshHeader->numMeshes || geometry->numVertices <= 0) {
        return;
    }

    const uint32 flags    = geometry->flags;
    const bool   hasNormals = (flags & rpGEOMETRYNORMALS) != 0;
    const bool   hasTex0    = geometry->numTexCoordSets >= 1 && geometry->texCoords[0];
    const bool   hasTex1    = geometry->numTexCoordSets >= 2 && geometry->texCoords[1];

    // Attribute layout (locations must match the shaders' `VSIN`s).
    rw::gl3::AttribDesc attribs[8];
    int32 numAttribs = 0;
    uint32 stride    = 0;
    const auto addAttrib = [&](uint32 index, int32 size, int32 type, bool normalized, uint32 bytes) {
        auto& a    = attribs[numAttribs++];
        a.index       = index;
        a.size        = size;
        a.type        = type;
        a.normalized  = normalized ? 1 : 0;
        a.stride      = 0; // patched below
        a.offset      = stride;
        stride       += bytes;
    };

    addAttrib(rw::gl3::ATTRIB_POS, 3, GL_FLOAT, false, sizeof(RwV3d));
    if (hasNormals || kind == Kind::Ped) {
        addAttrib(rw::gl3::ATTRIB_NORMAL, 3, GL_FLOAT, false, sizeof(RwV3d));
    }
    addAttrib(rw::gl3::ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, true, 4);
    if (hasTex0) {
        addAttrib(rw::gl3::ATTRIB_TEXCOORDS0, 2, GL_FLOAT, false, sizeof(RwTexCoords));
    }
    if (kind == Kind::Vehicle && hasTex1) {
        addAttrib(rw::gl3::ATTRIB_TEXCOORDS1, 2, GL_FLOAT, false, sizeof(RwTexCoords));
    }
    if (kind == Kind::Ped) {
        addAttrib(rw::gl3::ATTRIB_WEIGHTS, 4, GL_FLOAT, false, sizeof(RwMatrixWeights));
        addAttrib(rw::gl3::ATTRIB_INDICES, 4, GL_UNSIGNED_BYTE, false, sizeof(RwUInt32));
    }
    if (kind == Kind::BuildingDayNight) {
        addAttrib(kNightColourAttrib, 4, GL_UNSIGNED_BYTE, true, 4);
    }

    for (int32 i = 0; i < numAttribs; i++) {
        attribs[i].stride = stride;
    }

    // Meshes.
    auto* meshBase = reinterpret_cast<RpMesh*>(reinterpret_cast<uint8*>(meshHeader) + sizeof(RpMeshHeader) + meshHeader->firstMeshOffset);

    uint32 totalIndices = 0;
    for (uint32 i = 0; i < meshHeader->numMeshes; i++) {
        if (!meshBase[i].indices) {
            return; // unindexed geometry is not supported by the retail pipelines either
        }
        totalIndices += meshBase[i].numIndices;
    }

    auto& header = cache.header;
    header.platform       = rw::PLATFORM_GL3;
    header.serialNumber   = meshHeader->serialNum;
    header.numMeshes      = meshHeader->numMeshes;
    header.primType       = (meshHeader->flags & rpMESHHEADERTRISTRIP) ? GL_TRIANGLE_STRIP : GL_TRIANGLES;
    header.totalNumIndex  = totalIndices;
    header.totalNumVertex = (uint32)geometry->numVertices;
    header.numAttribs     = numAttribs;
    cache.vertexStride    = stride;
    cache.hasTexCoord1    = kind == Kind::Vehicle && hasTex1;
    cache.meshSerial      = meshHeader->serialNum;

    header.attribDesc = new rw::gl3::AttribDesc[numAttribs];
    std::memcpy(header.attribDesc, attribs, numAttribs * sizeof(rw::gl3::AttribDesc));

    header.indexBuffer = new uint16[totalIndices];
    header.vertexBuffer = new uint8[(size_t)geometry->numVertices * stride];
    header.inst = new rw::gl3::InstanceData[header.numMeshes];
    cache.meshes.resize(header.numMeshes);

    uint32 indexOffset = 0;
    for (uint32 i = 0; i < header.numMeshes; i++) {
        const RpMesh& mesh = meshBase[i];
        auto& inst = header.inst[i];

        inst.numIndex   = mesh.numIndices;
        inst.material   = nullptr; // game materials are kept in `cache.meshes`
        inst.program    = 0;
        inst.offset     = indexOffset;
        rw::findMinVertAndNumVertices(mesh.indices, mesh.numIndices, &inst.minVert, &inst.numVertices);

        std::memcpy(reinterpret_cast<uint8*>(header.indexBuffer) + indexOffset, mesh.indices, mesh.numIndices * sizeof(uint16));
        indexOffset += mesh.numIndices * sizeof(uint16);

        bool vertexAlpha = false;
        if (geometry->preLitLum) {
            for (int32 v = 0; v < inst.numVertices; v++) {
                const int32 vertex = (int32)inst.minVert + v;
                if (vertex < geometry->numVertices && geometry->preLitLum[vertex].alpha != 0xFF) {
                    vertexAlpha = true;
                    break;
                }
            }
        }
        inst.vertexAlpha = vertexAlpha;
        cache.meshes[i] = { mesh.material, vertexAlpha };
    }

    FillVertexBuffer(geometry, cache, kind);

    glGenBuffers(1, &header.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, header.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)totalIndices * sizeof(uint16), header.indexBuffer, GL_STATIC_DRAW);

    glGenBuffers(1, &header.vbo);
    UploadVertexBuffer(cache, geometry->numVertices);

    cache.kind  = kind;
    cache.valid = true;
}

void Reinstance(RpGeometry* geometry, GeometryCache& cache) {
    if (!cache.valid) {
        return;
    }
    FillVertexBuffer(geometry, cache, cache.kind);
    UploadVertexBuffer(cache, geometry->numVertices);
}

//-----------------------------------------------------------------------------
// Drawing helpers
//-----------------------------------------------------------------------------

//! Zeroed scratch texture used to hand a game texture to librw's `setTexture`. The trailing
//! bytes stand in for librw's texture plugin area (the anisotropy plugin stores one int).
alignas(16) uint8 s_textureMirrorStorage[sizeof(rw::Texture) + 128]{};

void BindTexture(int32 stage, RwTexture* texture) {
    auto* mirror = reinterpret_cast<rw::Texture*>(s_textureMirrorStorage);
    if (!texture) {
        rw::gl3::setTexture(stage, nullptr);
        return;
    }

    rw::Raster* raster = notsa::fakerw::GetLibrwRaster(texture->raster);
    if (!raster) {
        // Not created through this layer (TXD loading lands in B3): bind librw's white texture.
        rw::gl3::setTexture(stage, nullptr);
        return;
    }

    // Same packing as librw: filter (8 bits), address U (4), address V (4). Textures created
    // before the TXD reader lands may not have it initialised, so keep every field in range.
    uint32 filterAddressing = texture->filterAddressing;
    const auto inRange = [&filterAddressing](uint32 shift, uint32 first, uint32 last, uint32 fallback) {
        const uint32 mask = 0xFu << shift;
        const uint32 value = (filterAddressing & mask) >> shift;
        if (value < first || value > last) {
            filterAddressing = (filterAddressing & ~mask) | (fallback << shift);
        }
    };
    if ((filterAddressing & 0xFFu) < 1 || (filterAddressing & 0xFFu) > 6) {
        filterAddressing = (filterAddressing & ~0xFFu) | 1; // rwFILTERNEAREST
    }
    inRange(8,  1, 4, 1); // address U: wrap
    inRange(12, 1, 4, 1); // address V: wrap

    mirror->raster           = raster;
    mirror->filterAddressing = filterAddressing;
    mirror->setMaxAnisotropy(1);
    rw::gl3::setTexture(stage, mirror);
}

void SetUniformVec4(int32 id, float x, float y, float z, float w) {
    if (id >= 0) {
        float value[4] = { x, y, z, w };
        rw::gl3::setUniform(id, value);
    }
}

void UploadCommonUniforms() {
    SetUniformVec4(s_uWetRoad, GetWetRoadLevel(), 0.0f, 0.0f, 0.0f);
    if (s_uEnvMapMatrix >= 0) {
        rw::gl3::setUniform(s_uEnvMapMatrix, s_envMapMatrix);
    }
    if (s_uVehicleColours >= 0) {
        float colours[16] = {
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
        };
        for (int32 i = 0; i < 4; i++) {
            const uint8 index = CVehicleModelInfo::ms_currentCol[i];
            const CRGBA& colour = CVehicleModelInfo::ms_vehicleColourTable[index % CVehicleModelInfo::NUM_VEHICLE_COLORS];
            colours[i * 4 + 0] = colour.r / 255.0f;
            colours[i * 4 + 1] = colour.g / 255.0f;
            colours[i * 4 + 2] = colour.b / 255.0f;
            colours[i * 4 + 3] = 1.0f;
        }
        rw::gl3::setUniform(s_uVehicleColours, colours);
    }
}

void UploadBones(RpAtomic* atomic) {
    if (s_uBones < 0) {
        return;
    }

    rw::Matrix identities[kMaxBones];
    for (auto& m : identities) {
        m.setIdentity();
    }

    RpSkin* skin = RpSkinGeometryGetSkin(atomic->geometry);
    const uint32 numBones = skin ? RpSkinGetNumBones(skin) : 0;

    if (numBones == 0 || numBones > kMaxBones) {
        rw::gl3::setUniform(s_uBones, identities);
        return;
    }

    RpHAnimHierarchy* hierarchy = RpSkinAtomicGetHAnimHierarchy(atomic);
    const RwMatrix* inverseMatrices = RpSkinGetSkinToBoneMatrices(skin);
    if (!hierarchy || !hierarchy->pMatrixArray || !inverseMatrices || hierarchy->numNodes < (int32)numBones) {
        rw::gl3::setUniform(s_uBones, identities);
        return;
    }

    if (hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES) {
        for (uint32 i = 0; i < numBones; i++) {
            const rw::Matrix inverse = notsa::fakerw::ToLibrw(inverseMatrices[i]);
            const rw::Matrix node    = notsa::fakerw::ToLibrw(hierarchy->pMatrixArray[i]);
            rw::Matrix::mult(&identities[i], &inverse, &node);
        }
    } else {
        rw::Matrix inverseAtomic;
        inverseAtomic.setIdentity();
        if (RwFrame* frame = RpAtomicGetFrame(atomic)) {
            if (RwMatrix* ltm = RwFrameGetLTM(frame)) {
                const rw::Matrix world = notsa::fakerw::ToLibrw(*ltm);
                rw::Matrix::invert(&inverseAtomic, &world);
            }
        }
        for (uint32 i = 0; i < numBones; i++) {
            const rw::Matrix inverse = notsa::fakerw::ToLibrw(inverseMatrices[i]);
            const rw::Matrix node    = notsa::fakerw::ToLibrw(hierarchy->pMatrixArray[i]);
            rw::Matrix tmp;
            rw::Matrix::mult(&tmp, &node, &inverseAtomic);
            rw::Matrix::mult(&identities[i], &inverse, &tmp);
        }
    }

    rw::gl3::setUniform(s_uBones, identities);
}

rw::gl3::Shader* SelectShader(const Pipeline& pipeline, bool alphaTest) {
    return alphaTest ? pipeline.shader : pipeline.shaderNoAT;
}

bool LightingEnabled(const RpGeometry* geometry) {
    return (geometry->flags & rpGEOMETRYLIGHT) != 0
        && (geometry->flags & rpGEOMETRYNORMALS) != 0
        && geometry->morphTarget
        && geometry->morphTarget[0].normals;
}

//-----------------------------------------------------------------------------
// Drawing
//-----------------------------------------------------------------------------

void RenderBuilding(RpAtomic* atomic, GeometryCache& cache, Kind kind) {
    const bool alphaTest = rw::gl3::getAlphaTest() != 0;
    rw::gl3::Shader* shader = SelectShader(s_pipelines[(uint8)kind], alphaTest);
    if (!shader) {
        return;
    }

    UploadCommonUniforms();
    SetUniformVec4(s_uFlags, LightingEnabled(atomic->geometry) ? 1.0f : 0.0f, 1.0f, 0.0f, 0.0f);
    if (kind == Kind::BuildingDayNight) {
        SetUniformVec4(s_uDNBalance, GetDNBalanceParam(), 0.0f, 0.0f, 0.0f);
    }

    auto& header = cache.header;
    for (uint32 i = 0; i < header.numMeshes; i++) {
        RpMaterial* material = cache.meshes[i].material;
        if (!material) {
            continue;
        }

        rw::gl3::setMaterial(
            atomic->geometry->flags,
            rw::RGBA{ material->color.red, material->color.green, material->color.blue, material->color.alpha },
            rw::SurfaceProperties{ material->surfaceProps.ambient, material->surfaceProps.specular, material->surfaceProps.diffuse }
        );
        BindTexture(0, material->texture);
        rw::SetRenderState(rw::VERTEXALPHA, cache.meshes[i].vertexAlpha || material->color.alpha != 0xFF);

        shader->use();
        rw::gl3::drawInst(&header, &header.inst[i]);
    }
}

void RenderVehicle(RpAtomic* atomic, GeometryCache& cache) {
    auto& pipeline = s_pipelines[(uint8)Kind::Vehicle];
    const bool alphaTest = rw::gl3::getAlphaTest() != 0;
    rw::gl3::Shader* shader = SelectShader(pipeline, alphaTest);
    if (!shader) {
        return;
    }

    UploadCommonUniforms();

    const bool lighting = LightingEnabled(atomic->geometry);
    const bool envAttached = EnvMapPluginAttached();
    const bool specAttached = SpecMapPluginAttached();

    const RwMatrix* ltm = nullptr;
    if (RwFrame* frame = RpAtomicGetFrame(atomic)) {
        ltm = RwFrameGetLTM(frame);
    }

    auto& header = cache.header;
    for (uint32 i = 0; i < header.numMeshes; i++) {
        RpMaterial* material = cache.meshes[i].material;
        if (!material) {
            continue;
        }

        // Env map / specular data comes from the game's own material plugins.
        const CustomEnvMapPipeMaterialData* envData = envAttached ? CCustomCarEnvMapPipeline::EnvMapPlGetData(material) : nullptr;
        RwTexture* envTexture = envData ? envData->Texture : nullptr;

        int32 envMode = 0;
        float envScaleX = 1.0f, envScaleY = 1.0f;
        float envShininess = 0.0f, envOffsetU = 0.0f, envOffsetV = 0.0f;
        if (envTexture && envData->Shininess > 0.0f) {
            envMode = envTexture->name[0] == 'x' ? 2 : 1; // 'x' prefixed reflections scroll (EnvWave)
            if (envMode == 2 && !cache.hasTexCoord1) {
                envMode = 1; // no second UV set: project camera space normals instead
            }
            envScaleX    = envData->Scale.x;
            envScaleY    = envData->Scale.y;
            envShininess = envData->Shininess;

            // Retail slides the reflection UV with the vehicle's world position over a
            // `TranslationScale * 50` period (the texture wraps).
            if (ltm) {
                const float periodX = std::max(std::fabs((float)envData->TranslationScale.x) * 50.0f, 1e-3f);
                const float periodY = std::max(std::fabs((float)envData->TranslationScale.y) * 50.0f, 1e-3f);
                envOffsetU = -std::fmod(ltm->pos.x, periodX) / periodX;
                envOffsetV = -std::fmod(ltm->pos.y, periodY) / periodY;
            }
        }

        float specularStrength = 0.0f;
        float specularPower    = 1.0f;
        if (specAttached) {
            if (const CustomSpecMapPipeMaterialData* specData = CCustomCarEnvMapPipeline::SpecMapPlGetData(material)) {
                if (specData->Specularity > 0.0f) {
                    specularStrength = std::min(1.0f, specData->Specularity * 2.0f);
                    specularPower    = std::max(specData->Specularity * 100.0f, 1.0f);
                }
            }
        }

        SetUniformVec4(s_uFlags, lighting ? 1.0f : 0.0f, 1.0f, (float)envMode, specularStrength > 0.0f ? 1.0f : 0.0f);
        SetUniformVec4(s_uEnvMap, envScaleX, envScaleY, envOffsetU, envOffsetV);
        SetUniformVec4(s_uSpecular, specularStrength, specularPower, envShininess, 0.0f);

        rw::gl3::setMaterial(
            atomic->geometry->flags,
            rw::RGBA{ material->color.red, material->color.green, material->color.blue, material->color.alpha },
            rw::SurfaceProperties{ material->surfaceProps.ambient, material->surfaceProps.specular, material->surfaceProps.diffuse }
        );
        BindTexture(0, material->texture);
        BindTexture(1, envMode ? envTexture : nullptr);
        rw::SetRenderState(rw::VERTEXALPHA, cache.meshes[i].vertexAlpha || material->color.alpha != 0xFF);

        shader->use();
        rw::gl3::drawInst(&header, &header.inst[i]);
    }
}

void RenderPed(RpAtomic* atomic, GeometryCache& cache) {
    auto& pipeline = s_pipelines[(uint8)Kind::Ped];
    const bool alphaTest = rw::gl3::getAlphaTest() != 0;
    rw::gl3::Shader* shader = SelectShader(pipeline, alphaTest);
    if (!shader) {
        return;
    }

    UploadCommonUniforms();
    UploadBones(atomic);
    SetUniformVec4(s_uFlags, LightingEnabled(atomic->geometry) ? 1.0f : 0.0f, 1.0f, 0.0f, 0.0f);

    auto& header = cache.header;
    for (uint32 i = 0; i < header.numMeshes; i++) {
        RpMaterial* material = cache.meshes[i].material;
        if (!material) {
            continue;
        }

        const float specular = material->surfaceProps.specular;
        SetUniformVec4(s_uSpecular, specular, 32.0f, 0.0f, 0.0f);

        rw::gl3::setMaterial(
            atomic->geometry->flags,
            rw::RGBA{ material->color.red, material->color.green, material->color.blue, material->color.alpha },
            rw::SurfaceProperties{ material->surfaceProps.ambient, material->surfaceProps.specular, material->surfaceProps.diffuse }
        );
        BindTexture(0, material->texture);
        rw::SetRenderState(rw::VERTEXALPHA, cache.meshes[i].vertexAlpha || material->color.alpha != 0xFF);

        shader->use();
        rw::gl3::drawInst(&header, &header.inst[i]);
    }
}

//! Retail's D3D9 pipelines enable only `rpLIGHTLIGHTATOMICS` lights for atomics
//! (`_rwD3D9LightsGlobalEnable(rpLIGHTLIGHTATOMICS)` in the custom object pipelines).
//! librw's `lightingCB()` enumerates the whole world, which would also feed the shaders the
//! three directionals `app_light.cpp` creates with flags `0` (they only drive render state).
//! Build the `WorldLights` here with that flag test applied.
void ApplyWorldLights() {
    rw::Light* directionals[8];
    rw::Light* locals[8];
    rw::WorldLights lights;

    lights.numAmbients     = 0;
    lights.ambient.red     = 0.0f;
    lights.ambient.green   = 0.0f;
    lights.ambient.blue    = 0.0f;
    lights.ambient.alpha   = 1.0f;
    lights.numDirectionals = 0;
    lights.directionals    = directionals;
    lights.numLocals       = 0;
    lights.locals          = locals;

    rw::World* const world = rw::engine->currentWorld;

    FORLIST(lnk, world->globalLights) {
        rw::Light* const light = rw::Light::fromWorld(lnk);
        if (!(light->getFlags() & rw::Light::LIGHTATOMICS)) {
            continue;
        }
        if (light->getType() == rw::Light::AMBIENT) {
            lights.ambient.red   += light->color.red;
            lights.ambient.green += light->color.green;
            lights.ambient.blue  += light->color.blue;
            lights.numAmbients++;
        } else if (light->getType() == rw::Light::DIRECTIONAL) {
            if (lights.numDirectionals < 8) {
                lights.directionals[lights.numDirectionals++] = light;
            }
        }
    }

    lights.numLocals = 0;
    FORLIST(lnk, world->localLights) {
        rw::Light* const light = rw::Light::fromWorld(lnk);
        if (!(light->getFlags() & rw::Light::LIGHTATOMICS)) {
            continue;
        }
        if (lights.numLocals >= 8) {
            break;
        }
        lights.locals[lights.numLocals++] = light;
    }

    rw::gl3::setLights(&lights);
}

} // namespace

//-----------------------------------------------------------------------------
// Public API
//-----------------------------------------------------------------------------

static void DestroyShaders() {
    for (auto& pipeline : s_pipelines) {
        if (pipeline.shader) {
            pipeline.shader->destroy();
        }
        if (pipeline.shaderNoAT) {
            pipeline.shaderNoAT->destroy();
        }
        pipeline = {};
    }
    // librw caches the bound program here; it would dangle otherwise.
    rw::gl3::currentShader = nullptr;
}

void RegisterSAPipelines() {
    // A later plugin may have registered more uniforms since the shaders were built; librw
    // looks uniforms up by index, so rebuild the programs to pick the new entries up.
    if (s_registered && s_shaderUniformCount == rw::gl3::uniformRegistry.numUniforms) {
        return;
    }

    if (!rw::gl3::shaderDecl) {
        NOTSA_LOG_ERR("[SAPipelines] no GL context (call RegisterSAPipelines after RwEngineStart)");
        return;
    }

    DestroyShaders();

    // The `sa` uniforms. `u_boneMatrices` is shared with librw's skin plugin (same name, type
    // and count), so registering it twice does not consume more uniform storage.
    s_uFlags          = rw::gl3::registerUniform("u_saFlags", rw::gl3::UNIFORM_VEC4);
    s_uDNBalance      = rw::gl3::registerUniform("u_saDNBalance", rw::gl3::UNIFORM_VEC4);
    s_uEnvMap         = rw::gl3::registerUniform("u_saEnvMap", rw::gl3::UNIFORM_VEC4);
    s_uEnvMapMatrix   = rw::gl3::registerUniform("u_saEnvMapMatrix", rw::gl3::UNIFORM_MAT4);
    s_uSpecular       = rw::gl3::registerUniform("u_saSpecular", rw::gl3::UNIFORM_VEC4);
    s_uWetRoad        = rw::gl3::registerUniform("u_saWetRoad", rw::gl3::UNIFORM_VEC4);
    s_uVehicleColours = rw::gl3::registerUniform("u_saVehicleColours", rw::gl3::UNIFORM_VEC4, 4);
    s_uBones          = rw::gl3::registerUniform("u_boneMatrices", rw::gl3::UNIFORM_MAT4, (int32)kMaxBones);

    if (s_uFlags < 0 || s_uDNBalance < 0 || s_uEnvMap < 0 || s_uEnvMapMatrix < 0
        || s_uSpecular < 0 || s_uWetRoad < 0 || s_uVehicleColours < 0 || s_uBones < 0
    ) {
        NOTSA_LOG_ERR("[SAPipelines] out of uniform slots; the SA pipelines will not render");
        return;
    }

    // Building (plain).
    s_pipelines[(uint8)Kind::Building].shader = CreateShaderPair(
        "CustomBuilding",
        kBuildingVert,
        kBuildingFrag,
        "",
        &s_pipelines[(uint8)Kind::Building].shaderNoAT
    );

    // Building (day/night vertex colours).
    s_pipelines[(uint8)Kind::BuildingDayNight].shader = CreateShaderPair(
        "CustomBuildingDN",
        kBuildingVert,
        kBuildingFrag,
        "#define SA_DAYNIGHT\n",
        &s_pipelines[(uint8)Kind::BuildingDayNight].shaderNoAT
    );

    // Vehicle (env map + specular).
    s_pipelines[(uint8)Kind::Vehicle].shader = CreateShaderPair(
        "CustomCarEnvMap",
        kVehicleVert,
        kVehicleFrag,
        "",
        &s_pipelines[(uint8)Kind::Vehicle].shaderNoAT
    );

    // Skinned peds with specular.
    s_pipelines[(uint8)Kind::Ped].shader = CreateShaderPair(
        "PedSkinSpecular",
        kPedVert,
        kPedFrag,
        "",
        &s_pipelines[(uint8)Kind::Ped].shaderNoAT
    );

    for (uint32 i = 0; i < kNumKinds; i++) {
        if (!s_pipelines[i].shader || !s_pipelines[i].shaderNoAT) {
            NOTSA_LOG_ERR("[SAPipelines] pipeline {} is unavailable", i);
        }
    }

    // The attempt is final for this uniform set: shader failures are deterministic and
    // already logged, so retrying would only leak the programs that did compile.
    s_shaderUniformCount = rw::gl3::uniformRegistry.numUniforms;
    s_registered = true;

    // Default uniform values (wet road, env matrix, paint colours).
    UploadCommonUniforms();
}

void ShutdownSAPipelines() {
    for (auto& [geometry, cache] : s_cache) {
        UNUSED(geometry);
        DestroyCache(cache);
    }
    s_cache.clear();
    DestroyShaders();
    s_registered = false;
    s_shaderUniformCount = -1;
}

RxPipeline* GetPipeline(Kind kind) {
    const auto index = (uint8)kind;
    if (index >= kNumKinds || !s_pipelines[index].shader) {
        return nullptr;
    }
    return reinterpret_cast<RxPipeline*>(&s_pipelines[index]);
}

bool GetPipelineKind(const RxPipeline* pipeline, Kind& kind) {
    if (!pipeline) {
        return false;
    }
    for (uint32 i = 0; i < kNumKinds; i++) {
        if (pipeline == reinterpret_cast<const RxPipeline*>(&s_pipelines[i])) {
            kind = (Kind)i;
            return true;
        }
    }
    return false;
}

void InstanceAtomic(RpAtomic* atomic, bool reinstance) {
    if (!atomic || !atomic->geometry) {
        return;
    }

    Kind kind;
    if (!GetPipelineKind(atomic->pipeline, kind)) {
        return;
    }

    RpGeometry* geometry = atomic->geometry;
    auto it = s_cache.find(geometry);
    if (it == s_cache.end()) {
        auto& cache = s_cache[geometry];
        cache.kind = kind;
        Build(geometry, cache, kind);
    } else {
        GeometryCache& cache = it->second;
        const bool meshChanged = !geometry->mesh || cache.meshSerial != geometry->mesh->serialNum;
        if (cache.kind != kind || meshChanged) {
            DestroyCache(cache);
            cache.kind = kind;
            Build(geometry, cache, kind);
        } else if (reinstance || geometry->lockedSinceLastInst) {
            Reinstance(geometry, cache);
        }
    }

    geometry->lockedSinceLastInst = 0;
}

void RenderAtomic(RpAtomic* atomic) {
    if (!atomic || !atomic->geometry) {
        return;
    }

    // Cheap and idempotent: also rebuilds the programs if a plugin registered uniforms late.
    RegisterSAPipelines();

    Kind kind;
    if (!GetPipelineKind(atomic->pipeline, kind)) {
        return;
    }

    InstanceAtomic(atomic, false);

    const auto it = s_cache.find(atomic->geometry);
    if (it == s_cache.end() || !it->second.valid) {
        return;
    }
    GeometryCache& cache = it->second;

    // World matrix + lights (shared with librw's own pipelines).
    rw::Matrix world;
    world.setIdentity();
    if (RwFrame* frame = RpAtomicGetFrame(atomic)) {
        if (RwMatrix* ltm = RwFrameGetLTM(frame)) {
            world = notsa::fakerw::ToLibrw(*ltm);
        }
    }
    rw::gl3::setWorldMatrix(&world);

    if (rw::engine && rw::engine->currentWorld) {
        ApplyWorldLights();
    } else {
        static const rw::WorldLights emptyLights{};
        rw::gl3::setLights(const_cast<rw::WorldLights*>(&emptyLights));
    }

    rw::gl3::setupVertexInput(&cache.header);
    switch (kind) {
    case Kind::Building:
    case Kind::BuildingDayNight:
        RenderBuilding(atomic, cache, kind);
        break;
    case Kind::Vehicle:
        RenderVehicle(atomic, cache);
        break;
    case Kind::Ped:
        RenderPed(atomic, cache);
        break;
    }
    rw::gl3::teardownVertexInput(&cache.header);
}

void ForgetGeometry(RpGeometry* geometry) {
    if (const auto it = s_cache.find(geometry); it != s_cache.end()) {
        DestroyCache(it->second);
        s_cache.erase(it);
    }
}

//-----------------------------------------------------------------------------
// Setters
//-----------------------------------------------------------------------------

void SetDNBalanceParam(float balance) {
    CCustomBuildingDNPipeline::m_fDNBalanceParam = balance;
}

float GetDNBalanceParam() {
    return CCustomBuildingDNPipeline::m_fDNBalanceParam;
}

void SetEnvMapMatrix(const RwMatrix& matrix) {
    const rw::Matrix converted = notsa::fakerw::ToLibrw(matrix);
    std::memcpy(s_envMapMatrix, &converted, sizeof(s_envMapMatrix));
}

void SetEnvMapMatrix(const float matrix[16]) {
    std::memcpy(s_envMapMatrix, matrix, sizeof(s_envMapMatrix));
}

void SetWetRoadLevel(float level) {
    CWeather::WetRoads = level;
}

float GetWetRoadLevel() {
    return CWeather::WetRoads;
}

void SetVehicleColours(uint8_t prim, uint8_t sec, uint8_t tert, uint8_t quat) {
    CVehicleModelInfo::ms_currentCol[0] = prim;
    CVehicleModelInfo::ms_currentCol[1] = sec;
    CVehicleModelInfo::ms_currentCol[2] = tert;
    CVehicleModelInfo::ms_currentCol[3] = quat;
}

} // namespace notsa::native::sapipelines

#endif // NOTSA_NATIVE
