#include "../../ext/Blur/CBlurPerformer.h"

#include "../../source/Irrlicht/COpenGLBuffer.h"
#include "../../source/Irrlicht/COpenGLDriver.h"
#include "../../source/Irrlicht/COpenGLExtensionHandler.h"
#include "../../source/Irrlicht/COpenGL2DTexture.h"

using namespace irr;
using namespace ext;
using namespace Blur;

namespace // traslation-unit-local things
{
constexpr const char* CS_CONVERSIONS = R"XDDD(
uvec2 encodeRgb(vec3 _rgb)
{
    uvec2 ret;
    ret.x = packHalf2x16(_rgb.rg);
    ret.y = packHalf2x16(vec2(_rgb.b, 0.f));

    return ret;
}
vec3 decodeRgb(uvec2 _rgb)
{
    vec3 ret;
    ret.rg = unpackHalf2x16(_rgb.x);
    ret.b = unpackHalf2x16(_rgb.y).x;

    return ret;
}
)XDDD";
constexpr const char* CS_DOWNSAMPLE_SRC = R"XDDD(
#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;

layout(std430, binding = 0) restrict writeonly buffer b {
	uvec2 ssbo[];
};

layout(binding = 0, location = 0) uniform sampler2D in_tex;

%s

void main()
{
    const uvec2 IDX = gl_GlobalInvocationID.xy; // each index corresponds to one pixel in downsampled texture
	
    const ivec2 OUT_SIZE = ivec2(512, 512);
	
    vec2 coords = vec2(IDX) / vec2(OUT_SIZE);
    vec4 avg = (
        texture(in_tex, coords) +
        textureOffset(in_tex, coords, ivec2(1, 0)) +
        textureOffset(in_tex, coords, ivec2(0, 1)) +
        textureOffset(in_tex, coords, ivec2(1, 1))
    ) / 4.f;

    const uint HBUF_IDX = IDX.y * OUT_SIZE.x + IDX.x;

    ssbo[HBUF_IDX] = encodeRgb(avg.xyz);
}
)XDDD";
constexpr const char* CS_PSUM_SRC = R"XDDD(
#version 430 core
layout(local_size_x = 256) in;

#define LC_IDX gl_LocalInvocationIndex
#define G_IDX gl_GlobalInvocationID.x

layout(std430, binding = 0) restrict readonly buffer Samples {
    uvec2 inSamples[];
};
layout(std430, binding = 1) restrict writeonly buffer Psum {
    vec3 outPsum[];
};

layout(location = 2) uniform uint bufoff; // start offset in `samples` buffer

%s

shared vec3 smem[512];

void main()
{
    const uint START_IDX = gl_WorkGroupID.x*512;
    
    smem[LC_IDX*2] = decodeRgb(inSamples[START_IDX + bufoff + LC_IDX*2]);
    smem[LC_IDX*2+1] = decodeRgb(inSamples[START_IDX + bufoff + LC_IDX*2 + 1]);
    
    memoryBarrierShared();
    barrier();

    const vec3 LAST = smem[511];

    uint offset = 1;
    for (int d = 512/2; d > 0; d /= 2)
    {
        memoryBarrierShared();
        barrier();

        if (LC_IDX < d)
        {
            uint ai = offset*(2*LC_IDX+1)-1;
            uint bi = offset*(2*LC_IDX+2)-1;

            smem[bi] += smem[ai];   
        }
        offset *= 2;
    }

    if (LC_IDX == 0) { smem[511] = vec3(0); }
    memoryBarrierShared();
    barrier();
    
    for (int d = 1; d < 512; d *= 2)
    {
        offset /= 2;
        memoryBarrierShared();
        barrier();

        if (LC_IDX < d)
        {
            uint ai = offset*(2*LC_IDX+1)-1;
            uint bi = offset*(2*LC_IDX+2)-1;
            
            vec3 tmp = smem[ai];
            smem[ai] = smem[bi];
            smem[bi] += tmp;
        }
    }

    memoryBarrierShared();
    barrier();

    const vec3 tmp0 = smem[2*LC_IDX];
    const vec3 tmp1 = smem[2*LC_IDX+1];
    if (LC_IDX > 0)
    {
        smem[2*LC_IDX-1] = tmp0;
        smem[2*LC_IDX] = tmp1;
    }
    else
    {
        smem[0] = tmp1;
        smem[511] += LAST;
    }

    memoryBarrierShared();
    barrier();

    outPsum[START_IDX + LC_IDX*2] = smem[LC_IDX*2];
    outPsum[START_IDX + LC_IDX*2+1] = smem[LC_IDX*2+1];
}
)XDDD";
constexpr const char* CS_BLUR_SRC = R"XDDD(
#version 430 core
layout(local_size_x = 512) in;

layout(std430, binding = 0) restrict writeonly buffer Samples {
	uvec2 outSamples[];
};
layout(std430, binding = 1) restrict readonly buffer Psum {
    vec3 inPsum[];
};

#define LC_IDX gl_LocalInvocationIndex
#define FINAL_PASS %d
#define RADIUS %u

#if FINAL_PASS
layout(location = 1, binding = 0, rgba16f) uniform writeonly image2D out_img;
#endif

layout(location = 3) uniform uint outOffset;
layout(location = 4) uniform uvec2 inMlt;
layout(location = 5) uniform uvec2 outMlt;

shared float smem_r[512];
shared float smem_g[512];
shared float smem_b[512];

%s

void storeShared(uint _idx, vec3 _val)
{
    smem_r[_idx] = _val.r;
    smem_g[_idx] = _val.g;
    smem_b[_idx] = _val.b;
}
vec3 loadShared(uint _idx)
{
    return vec3(smem_r[_idx], smem_g[_idx], smem_b[_idx]);
}

void main()
{
	uvec2 IDX = gl_GlobalInvocationID.xy;
    if (inMlt == uvec2(512, 1))
        IDX = IDX.yx;

    const uint READ_IDX = uint(dot(IDX, inMlt));
        
    storeShared(LC_IDX, inPsum[READ_IDX]);

    memoryBarrierShared();
    barrier();

    const int FIRST_IDX = 0;
    const uint LAST_IDX = 511;
    const int L_IDX = int(LC_IDX) - RADIUS - 1;
    const uint R_IDX = LC_IDX + RADIUS;
    const uint R_EDGE_IDX = min(R_IDX, LAST_IDX);
    
    vec3 res = 
        loadShared(R_EDGE_IDX)
        + (R_IDX - R_EDGE_IDX)*(loadShared(LAST_IDX) - loadShared(LAST_IDX-1)) // right overflow
        - ((L_IDX < FIRST_IDX) ? ((L_IDX - FIRST_IDX) * loadShared(FIRST_IDX)) : loadShared(L_IDX)); // left overflow
	res /= (float(2*RADIUS) + 1.f);
#if FINAL_PASS
    imageStore(out_img, ivec2(IDX), vec4(res, 1.f));
#else
    outSamples[uint(dot(IDX, outMlt)) + outOffset] = encodeRgb(res);
#endif
}
)XDDD";

inline unsigned createComputeShader(const char* _src)
{
    unsigned program = video::COpenGLExtensionHandler::extGlCreateProgram();
	unsigned cs = video::COpenGLExtensionHandler::extGlCreateShader(GL_COMPUTE_SHADER);

	video::COpenGLExtensionHandler::extGlShaderSource(cs, 1, const_cast<const char**>(&_src), NULL);
	video::COpenGLExtensionHandler::extGlCompileShader(cs);

	// check for compilation errors
    GLint success;
    GLchar infoLog[0x400];
    video::COpenGLExtensionHandler::extGlGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        video::COpenGLExtensionHandler::extGlGetShaderInfoLog(cs, sizeof(infoLog), nullptr, infoLog);
        os::Printer::log("CS COMPILATION ERROR:\n", infoLog, ELL_ERROR);
        video::COpenGLExtensionHandler::extGlDeleteShader(cs);
        video::COpenGLExtensionHandler::extGlDeleteProgram(program);
        return 0;
	}

	video::COpenGLExtensionHandler::extGlAttachShader(program, cs);
	video::COpenGLExtensionHandler::extGlLinkProgram(program);

	//check linking errors
	success = 0;
    video::COpenGLExtensionHandler::extGlGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        video::COpenGLExtensionHandler::extGlGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        os::Printer::log("CS LINK ERROR:\n", infoLog, ELL_ERROR);
        video::COpenGLExtensionHandler::extGlDeleteShader(cs);
        video::COpenGLExtensionHandler::extGlDeleteProgram(program);
        return 0;
    }

	return program;
}
} // anon ns end

uint32_t CBlurPerformer::s_texturesEverCreatedCount{};
core::vector2d<size_t> CBlurPerformer::s_outTexSize(512u, 512); // if changing here, remember to also change in dsample shader source

CBlurPerformer* CBlurPerformer::instantiate(video::IVideoDriver* _driver, uint32_t _radius, core::vector2d<uint32_t> _outSize)
{
    unsigned ds{}, ps{}, gblur{}, fblur{};

    const size_t bufSize = std::max(strlen(CS_BLUR_SRC), std::max(strlen(CS_PSUM_SRC), strlen(CS_DOWNSAMPLE_SRC))) + strlen(CS_CONVERSIONS) + 100u;
    char* src = (char*)malloc(bufSize);

    auto doCleaning = [&, ds, ps, gblur, fblur, src]() {
        for (unsigned s : { ds, ps, gblur, fblur })
            video::COpenGLExtensionHandler::extGlDeleteProgram(s);
        free(src);
        return nullptr;
    };

    if (!genDsampleCs(src, bufSize))
        return doCleaning();
    //printf("build DSAMPLE\n");
    ds = createComputeShader(src);

    if (!genPsumCs(src, bufSize))
        return doCleaning();
    //printf("build PSUM\n");
    ps = createComputeShader(src);
    
    if (!genBlurPassCs(src, bufSize, _radius, 0))
        return doCleaning();
    //printf("build GBLUR\n");
    gblur = createComputeShader(src);
    
    if (!genBlurPassCs(src, bufSize, _radius, 1))
        return doCleaning();
    //printf("build FBLUR\n");
    fblur = createComputeShader(src);

    for (unsigned s : {ds, ps, gblur, fblur})
    {
        if (!s)
            return doCleaning();
    }
    free(src);

    return new CBlurPerformer(_driver, ds, ps, gblur, fblur, _radius, _outSize);
}

video::ITexture* CBlurPerformer::createBlurredTexture(video::ITexture* _inputTex) const
{
    GLuint prevSsbo[2];
    GLint prevProgram{};
    video::COpenGLExtensionHandler::extGlGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, E_SAMPLES_SSBO_BINDING, (GLint*)prevSsbo);
    video::COpenGLExtensionHandler::extGlGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, E_SAMPLES_SSBO_BINDING, (GLint*)(prevSsbo+1));
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    video::COpenGLExtensionHandler::extGlBindBuffersBase(GL_SHADER_STORAGE_BUFFER, E_SAMPLES_SSBO_BINDING, 1, &static_cast<video::COpenGLBuffer*>(m_ssbo)->getOpenGLName());
    video::COpenGLExtensionHandler::extGlBindBuffersBase(GL_SHADER_STORAGE_BUFFER, E_PSUM_SSBO_BINDING, 1, &static_cast<video::COpenGLBuffer*>(m_psumSsbo)->getOpenGLName());

    {
    video::STextureSamplingParams params;
    params.UseMipmaps = 0;
    params.MaxFilter = params.MinFilter = video::ETFT_LINEAR_NO_MIP;
    params.TextureWrapU = params.TextureWrapV = video::ETC_CLAMP_TO_EDGE;
    const_cast<video::COpenGLDriver::SAuxContext*>(reinterpret_cast<video::COpenGLDriver*>(m_driver)->getThreadContext())->setActiveTexture(0, _inputTex, params);
    }

    const uint32_t size[]{ s_outTexSize.X, s_outTexSize.Y};
    video::ITexture* outputTex = m_driver->addTexture(video::ITexture::ETT_2D, size, 1, ("blur_out" + std::to_string(s_texturesEverCreatedCount++)).c_str(), video::ECF_A16B16G16R16F);

    auto prevImgBinding = getCurrentImageBinding(0);
    video::COpenGLExtensionHandler::extGlBindImageTexture(0, static_cast<const video::COpenGL2DTexture*>(outputTex)->getOpenGLName(),
        0, GL_FALSE, 0, GL_WRITE_ONLY, video::COpenGLTexture::getOpenGLFormatAndParametersFromColorFormat(static_cast<const video::COpenGL2DTexture*>(outputTex)->getColorFormat()));



    video::COpenGLExtensionHandler::extGlUseProgram(m_dsampleCs);

    video::COpenGLExtensionHandler::extGlDispatchCompute(32u, 32u, 1u);
    video::COpenGLExtensionHandler::extGlMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    unsigned inOffset{}, outOffset{};
    const core::vector2d<unsigned> HMLT(1u, s_outTexSize.X), VMLT(s_outTexSize.Y, 1u);
    const core::vector2d<unsigned> imultipliers[5]{ HMLT, HMLT, HMLT, VMLT, VMLT };
    const core::vector2d<unsigned> omultipliers[5]{ HMLT, HMLT, VMLT, VMLT, VMLT };

    inOffset = 0u;
    outOffset = s_outTexSize.X * s_outTexSize.Y;

    for (size_t i = 0u; i < 5u; ++i)
    {
        video::COpenGLExtensionHandler::extGlUseProgram(m_psumCs);
        video::COpenGLExtensionHandler::extGlProgramUniform1uiv(m_psumCs, E_IN_OFFSET_LOC, 1, &inOffset);

        video::COpenGLExtensionHandler::extGlDispatchCompute((i >= 3u ? s_outTexSize.X : s_outTexSize.Y), 1u, 1u);
        video::COpenGLExtensionHandler::extGlMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


        video::COpenGLExtensionHandler::extGlUseProgram(m_blurGeneralCs);
        video::COpenGLExtensionHandler::extGlProgramUniform2uiv(m_blurGeneralCs, E_IN_MLT_LOC, 1, &imultipliers[i].X);
        video::COpenGLExtensionHandler::extGlProgramUniform2uiv(m_blurGeneralCs, E_OUT_MLT_LOC, 1, &omultipliers[i].X);
        video::COpenGLExtensionHandler::extGlProgramUniform1uiv(m_blurGeneralCs, E_OUT_OFFSET_LOC, 1, &outOffset);

        video::COpenGLExtensionHandler::extGlDispatchCompute(1u, (i >= 3u ? s_outTexSize.X : s_outTexSize.Y), 1u);
        video::COpenGLExtensionHandler::extGlMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        std::swap(inOffset, outOffset);
    }

    video::COpenGLExtensionHandler::extGlUseProgram(m_psumCs);
    video::COpenGLExtensionHandler::extGlProgramUniform1uiv(m_psumCs, E_IN_OFFSET_LOC, 1, &inOffset);

    video::COpenGLExtensionHandler::extGlDispatchCompute(s_outTexSize.X, 1u, 1u);
    video::COpenGLExtensionHandler::extGlMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


    video::COpenGLExtensionHandler::extGlUseProgram(m_blurFinalCs);

    video::COpenGLExtensionHandler::extGlProgramUniform2uiv(m_blurFinalCs, E_IN_MLT_LOC, 1, &VMLT.X);
    video::COpenGLExtensionHandler::extGlProgramUniform1uiv(m_blurFinalCs, E_OUT_OFFSET_LOC, 1, &outOffset);

    video::COpenGLExtensionHandler::extGlDispatchCompute(1u, s_outTexSize.X, 1u);
    video::COpenGLExtensionHandler::extGlMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    video::COpenGLExtensionHandler::extGlUseProgram(prevProgram);
    video::COpenGLExtensionHandler::extGlBindBuffersBase(GL_SHADER_STORAGE_BUFFER, E_SAMPLES_SSBO_BINDING, 2u, prevSsbo);
    bindImage(0, prevImgBinding);

    return outputTex;
}

CBlurPerformer::~CBlurPerformer()
{
    m_ssbo->drop();
    video::COpenGLExtensionHandler::extGlDeleteProgram(m_dsampleCs);
    video::COpenGLExtensionHandler::extGlDeleteProgram(m_psumCs);
    video::COpenGLExtensionHandler::extGlDeleteProgram(m_blurGeneralCs);
    video::COpenGLExtensionHandler::extGlDeleteProgram(m_blurFinalCs);
}

bool CBlurPerformer::genDsampleCs(char* _out, size_t _bufSize)
{
    return snprintf(_out, _bufSize, CS_DOWNSAMPLE_SRC, CS_CONVERSIONS) > 0;
}
bool CBlurPerformer::genBlurPassCs(char* _out, size_t _bufSize, uint32_t _radius, int _finalPass)
{
    return snprintf(_out, _bufSize, CS_BLUR_SRC, _finalPass, _radius, CS_CONVERSIONS) > 0;
}

bool CBlurPerformer::genPsumCs(char * _out, size_t _bufSize)
{
    return snprintf(_out, _bufSize, CS_PSUM_SRC, CS_CONVERSIONS) > 0;
}

auto irr::ext::Blur::CBlurPerformer::getCurrentImageBinding(unsigned _imgUnit) const -> ImageBindingData
{
    using gl = video::COpenGLExtensionHandler;

    ImageBindingData data;
    gl::extGlGetIntegeri_v(GL_IMAGE_BINDING_NAME, _imgUnit, (GLint*)&data.name);
    gl::extGlGetIntegeri_v(GL_IMAGE_BINDING_LEVEL, _imgUnit, &data.level);
    gl::extGlGetBooleani_v(GL_IMAGE_BINDING_LAYERED, _imgUnit, &data.layered);
    gl::extGlGetIntegeri_v(GL_IMAGE_BINDING_LAYER, _imgUnit, &data.layer);
    gl::extGlGetIntegeri_v(GL_IMAGE_BINDING_ACCESS, _imgUnit, (GLint*)&data.access);
    gl::extGlGetIntegeri_v(GL_IMAGE_BINDING_FORMAT, _imgUnit, (GLint*)&data.format);

    return data;
}

void irr::ext::Blur::CBlurPerformer::bindImage(unsigned _imgUnit, const ImageBindingData& _data) const
{
    video::COpenGLExtensionHandler::extGlBindImageTexture(_imgUnit, _data.name, _data.level, _data.layered, _data.layer, _data.access, _data.format);
}
