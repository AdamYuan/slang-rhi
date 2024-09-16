#include "d3d11-buffer.h"
#include "d3d11-device.h"

namespace rhi::d3d11 {

SLANG_NO_THROW DeviceAddress SLANG_MCALL BufferImpl::getDeviceAddress()
{
    return 0;
}

SLANG_NO_THROW Result SLANG_MCALL BufferImpl::map(MemoryRange* rangeToRead, void** outPointer)
{
    SLANG_UNUSED(rangeToRead);
    SLANG_UNUSED(outPointer);
    return SLANG_FAIL;
}

SLANG_NO_THROW Result SLANG_MCALL BufferImpl::unmap(MemoryRange* writtenRange)
{
    SLANG_UNUSED(writtenRange);
    return SLANG_FAIL;
}

ID3D11ShaderResourceView* BufferImpl::getSRV(Format format, const BufferRange& range)
{
    ViewKey key = {format, range};
    ComPtr<ID3D11ShaderResourceView>& srv = m_srvs[key];
    if (srv)
        return srv.get();

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Format = D3DUtil::getMapFormat(format);

    if (m_desc.elementSize)
    {
        srvDesc.Buffer.FirstElement = UINT(range.offset / m_desc.elementSize);
        srvDesc.Buffer.NumElements = UINT(range.size / m_desc.elementSize);
    }
    else if (format == Format::Unknown)
    {
        // We need to switch to a different member of the `union`,
        // so that we can set the `BufferEx.Flags` member.
        //
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

        srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.BufferEx.FirstElement = UINT(range.offset / 4);
        srvDesc.BufferEx.NumElements = UINT(range.size / 4);
    }
    else
    {
        FormatInfo sizeInfo;
        rhiGetFormatInfo(format, &sizeInfo);
        srvDesc.Buffer.FirstElement = UINT(range.offset / (sizeInfo.blockSizeInBytes / sizeInfo.pixelsPerBlock));
        srvDesc.Buffer.NumElements = UINT(range.size / (sizeInfo.blockSizeInBytes / sizeInfo.pixelsPerBlock));
    }

    ID3D11Device* device = static_cast<DeviceImpl*>(m_device.get())->m_device;
    SLANG_RETURN_NULL_ON_FAIL(device->CreateShaderResourceView(m_buffer, &srvDesc, srv.writeRef()));

    return srv.get();
}

ID3D11UnorderedAccessView* BufferImpl::getUAV(Format format, const BufferRange& range)
{
    ViewKey key = {format, range};
    ComPtr<ID3D11UnorderedAccessView>& uav = m_uavs[key];
    if (uav)
        return uav.get();

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Format = D3DUtil::getMapFormat(format);

    if (m_desc.elementSize)
    {
        uavDesc.Buffer.FirstElement = UINT(range.offset / m_desc.elementSize);
        uavDesc.Buffer.NumElements = UINT(range.size / m_desc.elementSize);
    }
    else if (format == Format::Unknown)
    {
        uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
        uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        uavDesc.Buffer.FirstElement = UINT(range.offset / 4);
        uavDesc.Buffer.NumElements = UINT(range.size / 4);
    }
    else
    {
        FormatInfo sizeInfo;
        rhiGetFormatInfo(format, &sizeInfo);
        uavDesc.Buffer.FirstElement = UINT(range.offset / (sizeInfo.blockSizeInBytes / sizeInfo.pixelsPerBlock));
        uavDesc.Buffer.NumElements = UINT(range.size / (sizeInfo.blockSizeInBytes / sizeInfo.pixelsPerBlock));
    }

    DeviceImpl* device = static_cast<DeviceImpl*>(m_device.get());
    SLANG_RETURN_NULL_ON_FAIL(device->m_device->CreateUnorderedAccessView(m_buffer, &uavDesc, uav.writeRef()));

    return uav.get();
}

} // namespace rhi::d3d11
