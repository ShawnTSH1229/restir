#include "RestirIntegrateAndDenoise.h"
#include "RestirGlobalResource.h"

using namespace Graphics;

void CRestirIntegrateAndDenoise::Init()
{
	restirIntegrateSig.Reset(3);
	restirIntegrateSig[0].InitAsConstantBuffer(0);
	restirIntegrateSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 9);
	restirIntegrateSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
	restirIntegrateSig.Finalize(L"restirIntegrateSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	{
		std::shared_ptr<SCompiledShaderCode> pCsShaderCode = GetGlobalResource().m_ShaderCompilerFxc.Compile(L"Shaders/ReSTIRUpsampleAndIntegrate.hlsl", L"UpscaleAndIntegrateCS", L"cs_5_1", nullptr, 0);
		restirIntegratePso.SetRootSignature(restirIntegrateSig);
		restirIntegratePso.SetComputeShader(pCsShaderCode->GetBufferPointer(), pCsShaderCode->GetBufferSize());
		restirIntegratePso.Finalize();
	}

	restirDenoiseSig.Reset(3);
	restirDenoiseSig[0].InitAsConstantBuffer(0);
	restirDenoiseSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4);
	restirDenoiseSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
	restirDenoiseSig.Finalize(L"restirDenoiseSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	{
		std::shared_ptr<SCompiledShaderCode> pCsShaderCode = GetGlobalResource().m_ShaderCompilerFxc.Compile(L"Shaders/ReSTIRSpatialDenoise.hlsl", L"ReSTIRSpatialDenoiseCS", L"cs_5_1", nullptr, 0);
		restirDenoisePso.SetRootSignature(restirDenoiseSig);
		restirDenoisePso.SetComputeShader(pCsShaderCode->GetBufferPointer(), pCsShaderCode->GetBufferSize());
		restirDenoisePso.Finalize();
	}
}

void CRestirIntegrateAndDenoise::IntegrateAndDenoise(GraphicsContext& gfxContext)
{
	ComputeContext& cptContext = gfxContext.GetComputeContext();
	RestirIntegrate(cptContext);
	RestirDenoise(cptContext);
}

void CRestirIntegrateAndDenoise::RestirIntegrate(ComputeContext& cptContext)
{
	cptContext.SetRootSignature(restirIntegrateSig);
	cptContext.SetPipelineState(restirIntegratePso);

	int curr_buf_idx = (GetGlobalResource().getCurrentFrameBufferIndex() + 1) % 2;
	int read_buffer_index = 0;


	cptContext.TransitionResource(g_ReservoirRayDirectionSpatialPingPong[read_buffer_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_ReservoirRayRadianceSpatialPingPong[read_buffer_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_ReservoirRayDistanceSpatialPingPong[read_buffer_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_ReservoirRayNormalSpatialPingPong[read_buffer_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_ReservoirRayWeightsSpatialPingPong[read_buffer_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	cptContext.TransitionResource(g_SceneGBufferA, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_SceneGBufferB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	
	cptContext.TransitionResource(g_DownSampledWorldPositionHistoryPingPong[curr_buf_idx], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	
	cptContext.TransitionResource(g_RestirDiffuseIndirect[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_RestirSpecularIndirect[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cptContext.FlushResourceBarriers();
	cptContext.ClearUAV(g_RestirDiffuseIndirect[1]);
	cptContext.ClearUAV(g_RestirSpecularIndirect[1]);

	cptContext.SetDynamicConstantBufferView(0, sizeof(SRestirSceneInfo), &GetGlobalResource().restirSceneInfo);

	cptContext.SetDynamicDescriptor(1, 0, g_ReservoirRayDirectionSpatialPingPong[read_buffer_index].GetSRV());
	cptContext.SetDynamicDescriptor(1, 1, g_ReservoirRayRadianceSpatialPingPong[read_buffer_index].GetSRV());
	cptContext.SetDynamicDescriptor(1, 2, g_ReservoirRayDistanceSpatialPingPong[read_buffer_index].GetSRV());
	cptContext.SetDynamicDescriptor(1, 3, g_ReservoirRayNormalSpatialPingPong[read_buffer_index].GetSRV());
	cptContext.SetDynamicDescriptor(1, 4, g_ReservoirRayWeightsSpatialPingPong[read_buffer_index].GetSRV());

	cptContext.SetDynamicDescriptor(1, 5, g_SceneGBufferA.GetSRV());
	cptContext.SetDynamicDescriptor(1, 6, g_SceneGBufferB.GetSRV());

	cptContext.SetDynamicDescriptor(1, 7, GetGlobalResource().STBNScalar.GetSRV());
	cptContext.SetDynamicDescriptor(1, 8, g_DownSampledWorldPositionHistoryPingPong[read_buffer_index].GetSRV());

	cptContext.SetDynamicDescriptor(2, 0, g_RestirDiffuseIndirect[1].GetUAV());
	cptContext.SetDynamicDescriptor(2, 1, g_RestirSpecularIndirect[1].GetUAV());

	cptContext.Dispatch(DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.x , 16), DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.y , 16), 1);
}

void CRestirIntegrateAndDenoise::RestirDenoise(ComputeContext& cptContext)
{
	cptContext.SetRootSignature(restirDenoiseSig);
	cptContext.SetPipelineState(restirDenoisePso);

	cptContext.TransitionResource(g_RestirDiffuseIndirect[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_RestirSpecularIndirect[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	cptContext.TransitionResource(g_RestirDiffuseIndirect[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_RestirSpecularIndirect[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cptContext.TransitionResource(g_SceneGBufferA, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_SceneGBufferB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	cptContext.FlushResourceBarriers();
	cptContext.ClearUAV(g_RestirDiffuseIndirect[0]);
	cptContext.ClearUAV(g_RestirSpecularIndirect[0]);

	cptContext.SetDynamicConstantBufferView(0, sizeof(SRestirSceneInfo), &GetGlobalResource().restirSceneInfo);

	cptContext.SetDynamicDescriptor(1, 0, g_RestirDiffuseIndirect[1].GetSRV());
	cptContext.SetDynamicDescriptor(1, 1, g_RestirSpecularIndirect[1].GetSRV());
	cptContext.SetDynamicDescriptor(1, 2, g_SceneGBufferA.GetSRV());
	cptContext.SetDynamicDescriptor(1, 3, g_SceneGBufferB.GetSRV());

	cptContext.SetDynamicDescriptor(2, 0, g_RestirDiffuseIndirect[0].GetUAV());
	cptContext.SetDynamicDescriptor(2, 1, g_RestirSpecularIndirect[0].GetUAV());

	cptContext.FlushResourceBarriers();

	cptContext.Dispatch(DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.x, 16), DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.y, 16), 1);
}
