#define _IRR_STATIC_LIB_
#include <iostream>
#include <cstdio>
#include <irrlicht.h>

//! I advise to check out this file, its a basic input handler
#include "../common/QToQuitEventReceiver.h"
#include "../../ext/FullScreenTriangle/FullScreenTriangle.h"

//#include "../../ext/ScreenShot/ScreenShot.h"


using namespace irr;
using namespace core;

int main()
{
	// create device with full flexibility over creation parameters
	// you can add more parameters if desired, check irr::SIrrlichtCreationParameters
	irr::SIrrlichtCreationParameters params;
	params.Bits = 24; //may have to set to 32bit for some platforms
	params.ZBufferBits = 24; //we'd like 32bit here
	params.DriverType = video::EDT_OPENGL; //! Only Well functioning driver, software renderer left for sake of 2D image drawing
	params.WindowSize = dimension2d<uint32_t>(1280, 720);
	params.Fullscreen = false;
	params.Vsync = true; //! If supported by target platform
	params.Doublebuffer = true;
	params.Stencilbuffer = false; //! This will not even be a choice soon
	auto device = createDeviceEx(params);

	if (!device)
		return 1; // could not create selected driver.


	//! disable mouse cursor, since camera will force it to the middle
	//! and we don't want a jittery cursor in the middle distracting us
	device->getCursorControl()->setVisible(false);

	//! Since our cursor will be enslaved, there will be no way to close the window
	//! So we listen for the "Q" key being pressed and exit the application
	QToQuitEventReceiver receiver;
	device->setEventReceiver(&receiver);


	auto* driver = device->getVideoDriver();
	auto* smgr = device->getSceneManager();
    auto* am = device->getAssetManager();
    auto* fs = am->getFileSystem();

    //
    auto* qnc = am->getMeshManipulator()->getQuantNormalCache();
    //loading cache from file
    qnc->loadNormalQuantCacheFromFile<asset::CQuantNormalCache::E_CACHE_TYPE::ECT_2_10_10_10>(fs,"../../tmp/normalCache101010.sse", true);

    // register the zip
    device->getFileSystem()->addFileArchive("../../media/sponza.zip");

    asset::IAssetLoader::SAssetLoadParams lp;
    auto meshes_bundle = am->getAsset("sponza.obj", lp);
    assert(!meshes_bundle.isEmpty());
    auto mesh = meshes_bundle.getContents().begin()[0];
    auto mesh_raw = static_cast<asset::ICPUMesh*>(mesh.get());

    //saving cache to file
    qnc->saveCacheToFile(asset::CQuantNormalCache::E_CACHE_TYPE::ECT_2_10_10_10,fs,"../../tmp/normalCache101010.sse");
    
    //copy the pipeline
    auto pipeline_cp = core::smart_refctd_ptr_static_cast<asset::ICPURenderpassIndependentPipeline>(mesh_raw->getMeshBuffer(0u)->getPipeline()->clone());
    auto geometryShaderCode = R"===(
#version 440 core
struct DrawIndirectArrays_t
{
	uint  count;
	uint  instanceCount;
	uint  first;
	uint  baseInstance;
};
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout(set=0, binding=0) coherent buffer LineCount
{
  DrawIndirectArrays_t lineDraw;
};
layout(set=0, binding=1) writeonly buffer Lines
{
  float linePoints[]; // 6 floats decribe a line, 3d start, 3d end
};

void main() {    
    uint i;
    vec3 end = vec3(0,0,0);
    vec3 start = vec3(gl_in[0].gl_Position);
    for(i = 0;i < gl_in.length();i++)
    {
        gl_Position = gl_in[i].gl_Position;
        end += vec3(gl_in[i].gl_Position);
        EmitVertex();
    }
    EndPrimitive();

    end /= gl_in.length();
    uint outId = atomicAdd(lineDraw.count,1u);
    outId *= 6u;
    linePoints[outId+0u] = start.x;
    linePoints[outId+1u] = start.y;
    linePoints[outId+2u] = start.z;
    linePoints[outId+3u] = end.x;
    linePoints[outId+4u] = end.y;
    linePoints[outId+5u] = end.z;
} 
)===";

    //get the simple geometry shader data and turn it into ICPUSpecializedShader
    auto len = strlen(geometryShaderCode);
    auto shaderData = core::make_smart_refctd_ptr<asset::ICPUBuffer>(len);
    memcpy(shaderData->getPointer(), geometryShaderCode, len);
    auto unspecializedShader = core::make_smart_refctd_ptr<asset::ICPUShader>(std::move(shaderData), asset::ICPUShader::buffer_contains_glsl);
    auto shader = core::make_smart_refctd_ptr<asset::ICPUSpecializedShader>(std::move(unspecializedShader), asset::ISpecializedShader::SInfo({}, nullptr, "main", asset::ISpecializedShader::ESS_GEOMETRY));
    pipeline_cp->setShaderAtIndex(asset::ICPURenderpassIndependentPipeline::ESSI_GEOMETRY_SHADER_IX, shader.get());

    mesh_raw->getMeshBuffer(0)->setPipeline(std::move(pipeline_cp));


    //create buffers for the geometry shader
    auto lineCountBuffer = driver->createDeviceLocalGPUBufferOnDedMem(roundUp(sizeof(irr::asset::DrawArraysIndirectCommand_t),16ull));
    uint32_t triangleCount;
    if (!asset::IMeshManipulator::getPolyCount(triangleCount, mesh_raw))
        assert(false);

    auto linesBuffer = driver->createDeviceLocalGPUBufferOnDedMem(triangleCount * 6 * sizeof(float));
    
    driver->fillBuffer(lineCountBuffer.get(), offsetof(irr::asset::DrawArraysIndirectCommand_t, instanceCount), sizeof(uint32_t), triangleCount * 6 * sizeof(uint32_t));
    //not sure what first refers to
    driver->fillBuffer(lineCountBuffer.get(), offsetof(irr::asset::DrawArraysIndirectCommand_t, first), sizeof(uint32_t), triangleCount * 6 * sizeof(uint32_t));        
    //not sure what baseInstance does
    driver->fillBuffer(lineCountBuffer.get(), offsetof(irr::asset::DrawArraysIndirectCommand_t, baseInstance), sizeof(uint32_t), triangleCount * 6 * sizeof(uint32_t)); 

    const asset::SBufferBinding<video::IGPUBuffer> bufferBinding = { 0u ,linesBuffer };
    //we can safely assume that all meshbuffers within mesh loaded from OBJ has same DS1 layout (used for camera-specific data)
    //so we can create just one DS
    asset::ICPUDescriptorSetLayout* ds1layout = mesh_raw->getMeshBuffer(0u)->getPipeline()->getLayout()->getDescriptorSetLayout(0u); //set 1u ---> 0u ?
    uint32_t ds1UboBinding = 0u;
    for (const auto& bnd : ds1layout->getBindings())
        if (bnd.type==asset::EDT_UNIFORM_BUFFER)
        {
            ds1UboBinding = bnd.binding;
            break;
        }

    size_t neededDS1UBOsz = 0ull;
    {
        auto pipelineMetadata = static_cast<const asset::IPipelineMetadata*>(mesh_raw->getMeshBuffer(0u)->getPipeline()->getMetadata());
        for (const auto& shdrIn : pipelineMetadata->getCommonRequiredInputs())
            if (shdrIn.descriptorSection.type==asset::IPipelineMetadata::ShaderInput::ET_UNIFORM_BUFFER && shdrIn.descriptorSection.uniformBufferObject.set==1u && shdrIn.descriptorSection.uniformBufferObject.binding==ds1UboBinding)
                neededDS1UBOsz = std::max<size_t>(neededDS1UBOsz, shdrIn.descriptorSection.uniformBufferObject.relByteoffset+shdrIn.descriptorSection.uniformBufferObject.bytesize);
    }

    auto gpuds1layout = driver->getGPUObjectsFromAssets(&ds1layout, &ds1layout+1)->front();

    auto gpuubo = driver->createDeviceLocalGPUBufferOnDedMem(neededDS1UBOsz);
    auto gpuds1 = driver->createGPUDescriptorSet(std::move(gpuds1layout));
    {
        video::IGPUDescriptorSet::SWriteDescriptorSet write;
        write.dstSet = gpuds1.get();
        write.binding = ds1UboBinding;
        write.count = 1u;
        write.arrayElement = 0u;
        write.descriptorType = asset::EDT_UNIFORM_BUFFER;
        video::IGPUDescriptorSet::SDescriptorInfo info;
        {
            info.desc = gpuubo;
            info.buffer.offset = 0ull;
            info.buffer.size = neededDS1UBOsz;
        }
        write.info = &info;
        driver->updateDescriptorSets(1u, &write, 0u, nullptr);
    }
    //DescriptorSetLayout is null
    auto gpumesh = driver->getGPUObjectsFromAssets(&mesh_raw, &mesh_raw+1)->front();

	//! we want to move around the scene and view it from different angles
	scene::ICameraSceneNode* camera = smgr->addCameraSceneNodeFPS(0,100.0f,0.5f);

	camera->setPosition(core::vector3df(-4,0,0));
	camera->setTarget(core::vector3df(0,0,0));
	camera->setNearValue(1.f);
	camera->setFarValue(5000.0f);

    smgr->setActiveCamera(camera);

	uint64_t lastFPSTime = 0;
	while(device->run() && receiver.keepOpen())
	{
		driver->beginScene(true, true, video::SColor(255,255,255,255) );
        
        // zero out buffer LineCount
        driver->fillBuffer(lineCountBuffer.get(),0,sizeof(uint32_t),0);
        driver->fillBuffer(lineCountBuffer.get(),offsetof(irr::asset::DrawArraysIndirectCommand_t,count), roundUp(sizeof(uint32_t), 16ull),0u);

        //! This animates (moves) the camera and sets the transforms
		camera->OnAnimate(std::chrono::duration_cast<std::chrono::milliseconds>(device->getTimer()->getTime()).count());

        //emit "memory barrier" of type GL_SHADER_STORAGE_BITS before scene is drawn - same as pre render? or post invoking render but before it finishes?
        //did you mean GL_SHADER_STORAGE_BARRIER_BIT?
        video::COpenGLExtensionHandler::extGlMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		camera->render();


        core::vector<uint8_t> uboData(gpuubo->getSize());
        auto pipelineMetadata = static_cast<const asset::IPipelineMetadata*>(mesh_raw->getMeshBuffer(0u)->getPipeline()->getMetadata());
        for (const auto& shdrIn : pipelineMetadata->getCommonRequiredInputs())
        {
            if (shdrIn.descriptorSection.type==asset::IPipelineMetadata::ShaderInput::ET_UNIFORM_BUFFER && shdrIn.descriptorSection.uniformBufferObject.set==1u && shdrIn.descriptorSection.uniformBufferObject.binding==ds1UboBinding)
            {
                switch (shdrIn.type)
                {
                case asset::IPipelineMetadata::ECSI_WORLD_VIEW_PROJ:
                {
                    core::matrix4SIMD mvp = camera->getConcatenatedMatrix();
                    memcpy(uboData.data()+shdrIn.descriptorSection.uniformBufferObject.relByteoffset, mvp.pointer(), shdrIn.descriptorSection.uniformBufferObject.bytesize);
                }
                break;
                case asset::IPipelineMetadata::ECSI_WORLD_VIEW:
                {
                    core::matrix3x4SIMD MV = camera->getViewMatrix();
                    memcpy(uboData.data() + shdrIn.descriptorSection.uniformBufferObject.relByteoffset, MV.pointer(), shdrIn.descriptorSection.uniformBufferObject.bytesize);
                }
                break;
                case asset::IPipelineMetadata::ECSI_WORLD_VIEW_INVERSE_TRANSPOSE:
                {
                    core::matrix3x4SIMD MV = camera->getViewMatrix();
                    memcpy(uboData.data()+shdrIn.descriptorSection.uniformBufferObject.relByteoffset, MV.pointer(), shdrIn.descriptorSection.uniformBufferObject.bytesize);
                }
                break;
                }
            }
        }       
        driver->updateBufferRangeViaStagingBuffer(gpuubo.get(), 0ull, gpuubo->getSize(), uboData.data());

        for (uint32_t i = 0u; i < gpumesh->getMeshBufferCount(); ++i)
        {
            video::IGPUMeshBuffer* gpumb = gpumesh->getMeshBuffer(i);
            const video::IGPURenderpassIndependentPipeline* pipeline = gpumb->getPipeline();  
            const video::IGPUDescriptorSet* ds3 = gpumb->getAttachedDescriptorSet();

            driver->bindGraphicsPipeline(pipeline);
            const video::IGPUDescriptorSet* gpuds1_ptr = gpuds1.get();
            driver->bindDescriptorSets(video::EPBP_GRAPHICS, pipeline->getLayout(), 1u, 1u, &gpuds1_ptr, nullptr);
            const video::IGPUDescriptorSet* gpuds3_ptr = gpumb->getAttachedDescriptorSet();
            if (gpuds3_ptr)
                driver->bindDescriptorSets(video::EPBP_GRAPHICS, pipeline->getLayout(), 3u, 1u, &gpuds3_ptr, nullptr);
            driver->pushConstants(pipeline->getLayout(), video::IGPUSpecializedShader::ESS_FRAGMENT, 0u, gpumb->MAX_PUSH_CONSTANT_BYTESIZE, gpumb->getPushConstantsDataPtr());

            driver->drawMeshBuffer(gpumb);
        }
        //emit "memory barrier" of type GL_ALL_BARRIER_BITS after the entire scene finishes drawing
        video::COpenGLExtensionHandler::extGlMemoryBarrier(GL_ALL_BARRIER_BITS);

        
        //invoke driver->drawIndirect() and use linesBuffer
        /*  _vtxBindings[IGPUMeshBuffer::MAX_ATTR_BUF_BINDING_COUNT],           &bufferBinding that has linesBuffer ptr, what do the other 16 do
            asset::E_PRIMITIVE_TOPOLOGY mode,                                   asset::EPT_LINE_LIST
            const IGPUBuffer* indirectDrawBuff,                                 not sure about this one, since linesBuffer is already used in bufferBinding
            size_t offset, size_t maxCount, size_t stride,                      theres no overload that takes DrawArraysIndirectCommand_t
            const IGPUBuffer* countBuffer = nullptr, size_t countOffset = 0u    dont need to use these
            */
        driver->drawArraysIndirect(&bufferBinding, asset::EPT_LINE_LIST, linesBuffer.get(), 0u, triangleCount * 6 * sizeof(uint32_t), 6 * sizeof(uint32_t));
            
        //driver->drawArraysIndirect( asset::SBufferBinding<video::IGPUBuffer>(linesBuffer),);
		driver->endScene();

		// display frames per second in window title
		uint64_t time = device->getTimer()->getRealTime();
		if (time-lastFPSTime > 1000)
		{
			std::wostringstream str;
			str << L"Level Curve Extraction Demo - IrrlichtBAW Engine [" << driver->getName() << "] FPS:" << driver->getFPS() << " PrimitvesDrawn:" << driver->getPrimitiveCountDrawn();

			device->setWindowCaption(str.str().c_str());
			lastFPSTime = time;
		}
	}

	//create a screenshot
	{
		core::rect<uint32_t> sourceRect(0, 0, params.WindowSize.Width, params.WindowSize.Height);
		//ext::ScreenShot::dirtyCPUStallingScreenshot(device, "screenshot.png", sourceRect, asset::EF_R8G8B8_SRGB);
	}

	return 0;
}