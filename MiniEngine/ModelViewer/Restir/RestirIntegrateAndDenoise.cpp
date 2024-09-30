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

	restirSpatialDenoiseSig.Reset(3);
	restirSpatialDenoiseSig[0].InitAsConstantBuffer(0);
	restirSpatialDenoiseSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4);
	restirSpatialDenoiseSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
	restirSpatialDenoiseSig.Finalize(L"restirDenoiseSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	{
		std::shared_ptr<SCompiledShaderCode> pCsShaderCode = GetGlobalResource().m_ShaderCompilerFxc.Compile(L"Shaders/ReSTIRSpatialDenoise.hlsl", L"ReSTIRSpatialDenoiseCS", L"cs_5_1", nullptr, 0);
		restirSpatialDenoisePso.SetRootSignature(restirSpatialDenoiseSig);
		restirSpatialDenoisePso.SetComputeShader(pCsShaderCode->GetBufferPointer(), pCsShaderCode->GetBufferSize());
		restirSpatialDenoisePso.Finalize();
	}

	restirCopySig.Reset(3);
	restirCopySig[0].InitAsConstantBuffer(0);
	restirCopySig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	restirCopySig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
	restirCopySig.Finalize(L"restirCopySig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	{
		std::shared_ptr<SCompiledShaderCode> pCsShaderCode = GetGlobalResource().m_ShaderCompilerFxc.Compile(L"Shaders/RestirSceneColorCopy.hlsl", L"ReSTIRSceneColorCopyCS", L"cs_5_1", nullptr, 0);
		restirCopyPso.SetRootSignature(restirCopySig);
		restirCopyPso.SetComputeShader(pCsShaderCode->GetBufferPointer(), pCsShaderCode->GetBufferSize());
		restirCopyPso.Finalize();
	}

	restirCombineSig.Reset(3);
	restirCombineSig[0].InitAsConstantBuffer(0);
	restirCombineSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4);
	restirCombineSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
	restirCombineSig.Finalize(L"restirCombineSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	{
		std::shared_ptr<SCompiledShaderCode> pCsShaderCode = GetGlobalResource().m_ShaderCompilerFxc.Compile(L"Shaders/RestirSimpleCombineLight.hlsl", L"ReSTIRSimpleCombineCS", L"cs_5_1", nullptr, 0);
		restirCombinePso.SetRootSignature(restirCombineSig);
		restirCombinePso.SetComputeShader(pCsShaderCode->GetBufferPointer(), pCsShaderCode->GetBufferSize());
		restirCombinePso.Finalize();
	}

	restirTemporalDenoiseSig.Reset(3,1);
	restirTemporalDenoiseSig.InitStaticSampler(0, SamplerLinearClampDesc);
	restirTemporalDenoiseSig[0].InitAsConstantBuffer(0);
	restirTemporalDenoiseSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
	restirTemporalDenoiseSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
	restirTemporalDenoiseSig.Finalize(L"restirTemporalDenoiseSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	{
		std::shared_ptr<SCompiledShaderCode> pCsShaderCode = GetGlobalResource().m_ShaderCompilerFxc.Compile(L"Shaders/RestirTemporalDenoise.hlsl", L"ReSTIRTemporalDenoiseCS", L"cs_5_1", nullptr, 0);
		restirTemporalDenoisePso.SetRootSignature(restirTemporalDenoiseSig);
		restirTemporalDenoisePso.SetComputeShader(pCsShaderCode->GetBufferPointer(), pCsShaderCode->GetBufferSize());
		restirTemporalDenoisePso.Finalize();
	}
}

void CRestirIntegrateAndDenoise::IntegrateAndDenoise(GraphicsContext& gfxContext)
{
	ComputeContext& cptContext = gfxContext.GetComputeContext();
	RestirIntegrate(cptContext);
	if (GetGlobalResource().getCurrentFrameIndex() > 2)
	{
		RestirTemporalDenoise(cptContext);
		cptContext.CopyBuffer(g_RestirDiffuseIndirectHist, g_RestirDiffuseIndirect[1]);
		cptContext.CopyBuffer(g_RestirSpecularIndirectHist, g_RestirSpecularIndirect[1]);
	}
	
	RestirSpatialDenoise(cptContext);

	if (GetGlobalResource().getCurrentFrameIndex() <= 2)
	{
		cptContext.CopyBuffer(g_RestirDiffuseIndirectHist, g_RestirDiffuseIndirect[1]);
		cptContext.CopyBuffer(g_RestirSpecularIndirectHist, g_RestirSpecularIndirect[1]);
	}
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

void CRestirIntegrateAndDenoise::RestirTemporalDenoise(ComputeContext& cptContext)
{
	cptContext.SetRootSignature(restirTemporalDenoiseSig);
	cptContext.SetPipelineState(restirTemporalDenoisePso);

	cptContext.TransitionResource(g_RestirDiffuseIndirectHist, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_RestirSpecularIndirectHist, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_SceneGBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	cptContext.TransitionResource(g_RestirDiffuseIndirect[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_RestirSpecularIndirect[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cptContext.FlushResourceBarriers();

	cptContext.SetDynamicConstantBufferView(0, sizeof(SRestirSceneInfo), &GetGlobalResource().restirSceneInfo);

	cptContext.SetDynamicDescriptor(1, 0, g_RestirDiffuseIndirectHist.GetSRV());
	cptContext.SetDynamicDescriptor(1, 1, g_RestirSpecularIndirectHist.GetSRV());
	cptContext.SetDynamicDescriptor(1, 2, g_SceneGBufferB.GetSRV());

	cptContext.SetDynamicDescriptor(2, 0, g_RestirDiffuseIndirect[1].GetUAV());
	cptContext.SetDynamicDescriptor(2, 1, g_RestirSpecularIndirect[1].GetUAV());

	cptContext.Dispatch(DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.x, 16), DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.y, 16), 1);
	
	
}

void CRestirIntegrateAndDenoise::RestirSpatialDenoise(ComputeContext& cptContext)
{
	cptContext.SetRootSignature(restirSpatialDenoiseSig);
	cptContext.SetPipelineState(restirSpatialDenoisePso);

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



	cptContext.TransitionResource(g_RestirDiffuseIndirect[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_RestirSpecularIndirect[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void CRestirIntegrateAndDenoise::SimpleCombineLight(GraphicsContext& gfxContext)
{
	ComputeContext& cptContext = gfxContext.GetComputeContext();
	{
		cptContext.SetRootSignature(restirCopySig);
		cptContext.SetPipelineState(restirCopyPso);

		cptContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_SceneColorCombinedBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		cptContext.FlushResourceBarriers();

		cptContext.SetDynamicConstantBufferView(0, sizeof(SRestirSceneInfo), &GetGlobalResource().restirSceneInfo);
		cptContext.SetDynamicDescriptor(1, 0, g_SceneColorBuffer.GetSRV());
		cptContext.SetDynamicDescriptor(2, 0, g_SceneColorCombinedBuffer.GetUAV());

		cptContext.Dispatch(DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.x, 16), DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.y, 16), 1);
	}

	{
		cptContext.SetRootSignature(restirCombineSig);
		cptContext.SetPipelineState(restirCombinePso);

		cptContext.TransitionResource(g_SceneColorCombinedBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cptContext.TransitionResource(g_RestirDiffuseIndirect[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_RestirSpecularIndirect[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_SceneGBufferC, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		cptContext.FlushResourceBarriers();

		cptContext.SetDynamicConstantBufferView(0, sizeof(SRestirSceneInfo), &GetGlobalResource().restirSceneInfo);
		cptContext.SetDynamicDescriptor(1, 0, g_SceneColorCombinedBuffer.GetSRV());
		cptContext.SetDynamicDescriptor(1, 1, g_RestirDiffuseIndirect[0].GetSRV());
		cptContext.SetDynamicDescriptor(1, 2, g_RestirSpecularIndirect[0].GetSRV());
		cptContext.SetDynamicDescriptor(1, 3, g_SceneGBufferC.GetSRV());
		cptContext.SetDynamicDescriptor(2, 0, g_SceneColorBuffer.GetUAV());

		cptContext.Dispatch(DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.x, 16), DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_full_screen_texsize.y, 16), 1);

		cptContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.FlushResourceBarriers();
	}

	
}