#include "testing.h"

using namespace rhi;
using namespace rhi::testing;

static ComPtr<IBuffer> createBuffer(IDevice* device, uint32_t content)
{
    ComPtr<IBuffer> buffer;
    BufferDesc bufferDesc = {};
    bufferDesc.size = sizeof(uint32_t);
    bufferDesc.format = Format::Unknown;
    bufferDesc.elementSize = sizeof(float);
    bufferDesc.usage = BufferUsage::ShaderResource | BufferUsage::UnorderedAccess | BufferUsage::CopyDestination |
                       BufferUsage::CopySource;
    bufferDesc.defaultState = ResourceState::UnorderedAccess;
    bufferDesc.memoryType = MemoryType::DeviceLocal;

    ComPtr<IBuffer> numbersBuffer;
    REQUIRE_CALL(device->createBuffer(bufferDesc, (void*)&content, buffer.writeRef()));

    return buffer;
}
void testSamplerArray(GpuTestContext* ctx, DeviceType deviceType)
{
    if (deviceType == DeviceType::Vulkan && SLANG_APPLE_FAMILY)
        SKIP("not supported on MoltenVK");

    ComPtr<IDevice> device = createTestingDevice(ctx, deviceType);

    ComPtr<IShaderProgram> shaderProgram;
    slang::ProgramLayout* slangReflection;
    REQUIRE_CALL(loadComputeProgram(device, shaderProgram, "test-sampler-array", "computeMain", slangReflection));

    ComputePipelineDesc pipelineDesc = {};
    pipelineDesc.program = shaderProgram.get();
    ComPtr<IPipeline> pipeline;
    REQUIRE_CALL(device->createComputePipeline(pipelineDesc, pipeline.writeRef()));

    std::vector<ComPtr<ISampler>> samplers;
    ComPtr<ITexture> texture;
    ComPtr<IBuffer> buffer = createBuffer(device, 0);

    {
        TextureDesc textureDesc = {};
        textureDesc.type = TextureType::Texture2D;
        textureDesc.format = Format::R8G8B8A8_UNORM;
        textureDesc.size.width = 2;
        textureDesc.size.height = 2;
        textureDesc.size.depth = 1;
        textureDesc.mipLevelCount = 2;
        textureDesc.memoryType = MemoryType::DeviceLocal;
        textureDesc.usage = TextureUsage::ShaderResource | TextureUsage::CopyDestination;
        textureDesc.defaultState = ResourceState::ShaderResource;
        uint32_t data[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
        SubresourceData subResourceData[2] = {{data, 8, 16}, {data, 8, 16}};
        REQUIRE_CALL(device->createTexture(textureDesc, subResourceData, texture.writeRef()));
    }

    for (uint32_t i = 0; i < 32; i++)
    {
        SamplerDesc desc = {};
        ComPtr<ISampler> sampler;
        REQUIRE_CALL(device->createSampler(desc, sampler.writeRef()));
        samplers.push_back(sampler);
    }

    ComPtr<IShaderObject> s1 =
        device->createShaderObject(slangReflection->findTypeByName("S1"), ShaderObjectContainerType::None);
    {
        auto cursor = ShaderCursor(s1);
        for (uint32_t i = 0; i < 32; i++)
        {
            cursor["samplers"][i].setBinding(samplers[i]);
            cursor["tex"][i].setBinding(texture);
        }
        cursor["data"].setData(1.0f);
    }
    s1->finalize();

    ComPtr<IShaderObject> g =
        device->createShaderObject(slangReflection->findTypeByName("S0"), ShaderObjectContainerType::None);
    {
        auto cursor = ShaderCursor(g);
        cursor["s"].setObject(s1);
        cursor["data"].setData(2.0f);
    }
    g->finalize();

    ComPtr<IShaderObject> rootObject = device->createRootShaderObject(pipeline);
    {
        auto cursor = ShaderCursor(rootObject);
        cursor["g"].setObject(g);
        cursor["buffer"].setBinding(buffer);
    }
    rootObject->finalize();

    {
        auto queue = device->getQueue(QueueType::Graphics);
        auto encoder = queue->createCommandEncoder();

        ComputeState state;
        state.pipeline = pipeline;
        state.rootObject = rootObject;
        encoder->setComputeState(state);
        encoder->setComputeState(state);
        encoder->dispatchCompute(1, 1, 1);

        queue->submit(encoder->finish());
        queue->waitOnHost();
    }

    compareComputeResult(device, buffer, makeArray<float>(4.0f));
}

TEST_CASE("sampler-array")
{
    runGpuTests(
        testSamplerArray,
        {
            DeviceType::D3D12,
            DeviceType::Vulkan,
            // DeviceType::WGPU,
        }
    );
}
