#include "RestirResampling.h"
#include "RestirGlobalResource.h"

using namespace Graphics;

void CRestirResamplingPass::Init()
{
	restirTemporalResamplingnSig.Reset(3);
	restirTemporalResamplingnSig[0].InitAsConstantBuffer(0);
	restirTemporalResamplingnSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 15);
	restirTemporalResamplingnSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 5);
	restirTemporalResamplingnSig.Finalize(L"restirTemporalResamplingnSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	{
		std::shared_ptr<SCompiledShaderCode> pCsShaderCode = GetGlobalResource().m_ShaderCompilerFxc.Compile(L"Shaders/ReSTIRTemporalResampling.hlsl", L"TemporalResamplingCS", L"cs_5_1", nullptr, 0);
		restirTemporalResamplingnPso.SetRootSignature(restirTemporalResamplingnSig);
		restirTemporalResamplingnPso.SetComputeShader(pCsShaderCode->GetBufferPointer(), pCsShaderCode->GetBufferSize());
		restirTemporalResamplingnPso.Finalize();
	}

	restirSpatialResamplingnSig.Reset(4);
	restirSpatialResamplingnSig[0].InitAsConstantBuffer(0);
	restirSpatialResamplingnSig[1].InitAsConstantBuffer(1);
	restirSpatialResamplingnSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 8);
	restirSpatialResamplingnSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 5);
	restirSpatialResamplingnSig.Finalize(L"restirTemporalResamplingnSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	{
		std::shared_ptr<SCompiledShaderCode> pCsShaderCode = GetGlobalResource().m_ShaderCompilerFxc.Compile(L"Shaders/ReSTIRSpatialResampling.hlsl", L"SpatialResamplingCS", L"cs_5_1", nullptr, 0);
		restirSpatialResamplingnPso.SetRootSignature(restirSpatialResamplingnSig);
		restirSpatialResamplingnPso.SetComputeShader(pCsShaderCode->GetBufferPointer(), pCsShaderCode->GetBufferSize());
		restirSpatialResamplingnPso.Finalize();
	}

}

void CRestirResamplingPass::RestirResampling(GraphicsContext& gfxContext)
{
	ComputeContext& cptContext = gfxContext.GetComputeContext();

	int spatial_pass_index = 0;
	int cuurentFrameStartIndex = GetGlobalResource().getCurrentFrameBufferIndex();
	
	if (GetGlobalResource().getCurrentFrameIndex() > 1)
	{
		TemporalResampling(cptContext);
	}

	SpatialResampling(cptContext, 0, 1, spatial_pass_index); spatial_pass_index++;
	SpatialResampling(cptContext, 1, 0, spatial_pass_index); spatial_pass_index++;
}

void CRestirResamplingPass::TemporalResampling(ComputeContext& cptContext)
{
	int curr_buf_idx = (GetGlobalResource().getCurrentFrameBufferIndex() + 1) % 2;
	int hist_idx = (GetGlobalResource().getCurrentFrameBufferIndex() + 0) % 2;
	int spatial_pingpong_idx = 0;

	cptContext.SetRootSignature(restirTemporalResamplingnSig);
	cptContext.SetPipelineState(restirTemporalResamplingnPso);

	for (int index = 0; index < 2; index++)
	{
		cptContext.TransitionResource(g_ReservoirRayDirectionHistoryPingPong[index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_ReservoirRayRadianceHistoryPingPong[index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_ReservoirRayDistanceHistoryPingPong[index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_ReservoirRayNormalHistoryPingPong[index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_ReservoirRayWeightsHistoryPingPong[index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_DownSampledWorldPositionHistoryPingPong[index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_DownSampledWorldNormalHistoryPingPong[index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}


	cptContext.TransitionResource(g_ReservoirRayDirectionSpatialPingPong[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_ReservoirRayRadianceSpatialPingPong[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_ReservoirRayDistanceSpatialPingPong[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_ReservoirRayNormalSpatialPingPong[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_ReservoirRayWeightsSpatialPingPong[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	
	cptContext.FlushResourceBarriers();

	// CBV
	cptContext.SetDynamicConstantBufferView(0, sizeof(SRestirSceneInfo), &GetGlobalResource().restirSceneInfo);

	// SRV
	cptContext.SetDynamicDescriptor(1, 0, g_ReservoirRayDirectionHistoryPingPong[hist_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 1, g_ReservoirRayRadianceHistoryPingPong[hist_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 2, g_ReservoirRayDistanceHistoryPingPong[hist_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 3, g_ReservoirRayNormalHistoryPingPong[hist_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 4, g_ReservoirRayWeightsHistoryPingPong[hist_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 5, g_DownSampledWorldPositionHistoryPingPong[hist_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 6, g_DownSampledWorldNormalHistoryPingPong[hist_idx].GetSRV());

	// current frame index
	cptContext.SetDynamicDescriptor(1, 7, g_ReservoirRayDirectionHistoryPingPong[curr_buf_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 8, g_ReservoirRayRadianceHistoryPingPong[curr_buf_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 9, g_ReservoirRayDistanceHistoryPingPong[curr_buf_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 10, g_ReservoirRayNormalHistoryPingPong[curr_buf_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 11, g_ReservoirRayWeightsHistoryPingPong[curr_buf_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 12, g_DownSampledWorldPositionHistoryPingPong[curr_buf_idx].GetSRV());
	cptContext.SetDynamicDescriptor(1, 13, g_DownSampledWorldNormalHistoryPingPong[curr_buf_idx].GetSRV());

	cptContext.SetDynamicDescriptor(1, 14, GetGlobalResource().STBNScalar.GetSRV());

	// UAV
	cptContext.SetDynamicDescriptor(2, 0, g_ReservoirRayDirectionSpatialPingPong[0].GetUAV());
	cptContext.SetDynamicDescriptor(2, 1, g_ReservoirRayRadianceSpatialPingPong[0].GetUAV());
	cptContext.SetDynamicDescriptor(2, 2, g_ReservoirRayDistanceSpatialPingPong[0].GetUAV());
	cptContext.SetDynamicDescriptor(2, 3, g_ReservoirRayNormalSpatialPingPong[0].GetUAV());
	cptContext.SetDynamicDescriptor(2, 4, g_ReservoirRayWeightsSpatialPingPong[0].GetUAV());

	cptContext.Dispatch(DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_restir_texturesize.x, 16), DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_restir_texturesize.y, 16), 1);
}

void CRestirResamplingPass::SpatialResampling(ComputeContext& cptContext, int read_reservoir_index, int write_reservoir_index, int pass_index)
{
	int curr_buf_idx = (GetGlobalResource().getCurrentFrameBufferIndex() + 1) % 2;

	cptContext.SetRootSignature(restirSpatialResamplingnSig);
	cptContext.SetPipelineState(restirSpatialResamplingnPso);

	cptContext.TransitionResource(g_ReservoirRayDirectionSpatialPingPong[read_reservoir_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_ReservoirRayRadianceSpatialPingPong[read_reservoir_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_ReservoirRayDistanceSpatialPingPong[read_reservoir_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_ReservoirRayNormalSpatialPingPong[read_reservoir_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_ReservoirRayWeightsSpatialPingPong[read_reservoir_index], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	
	cptContext.TransitionResource(g_DownSampledWorldPositionHistoryPingPong[curr_buf_idx], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_DownSampledWorldNormalHistoryPingPong[curr_buf_idx], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	cptContext.TransitionResource(g_ReservoirRayDirectionSpatialPingPong[write_reservoir_index], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_ReservoirRayRadianceSpatialPingPong[write_reservoir_index], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_ReservoirRayDistanceSpatialPingPong[write_reservoir_index], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_ReservoirRayNormalSpatialPingPong[write_reservoir_index], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(g_ReservoirRayWeightsSpatialPingPong[write_reservoir_index], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cptContext.FlushResourceBarriers();

	struct SSpatialResamplingIndex
	{
		int spatial_resampling_index;
		int padding[3];
	};

	__declspec(align(16)) SSpatialResamplingIndex spatial_resampling_index;
	spatial_resampling_index.spatial_resampling_index = pass_index;

	cptContext.SetDynamicConstantBufferView(0, sizeof(SRestirSceneInfo), &GetGlobalResource().restirSceneInfo);
	cptContext.SetDynamicConstantBufferView(1, sizeof(SSpatialResamplingIndex), &spatial_resampling_index);

	cptContext.SetDynamicDescriptor(2, 0, g_ReservoirRayDirectionSpatialPingPong[read_reservoir_index].GetSRV());
	cptContext.SetDynamicDescriptor(2, 1, g_ReservoirRayRadianceSpatialPingPong[read_reservoir_index].GetSRV());
	cptContext.SetDynamicDescriptor(2, 2, g_ReservoirRayDistanceSpatialPingPong[read_reservoir_index].GetSRV());
	cptContext.SetDynamicDescriptor(2, 3, g_ReservoirRayNormalSpatialPingPong[read_reservoir_index].GetSRV());
	cptContext.SetDynamicDescriptor(2, 4, g_ReservoirRayWeightsSpatialPingPong[read_reservoir_index].GetSRV());
									
	cptContext.SetDynamicDescriptor(2, 5, g_DownSampledWorldPositionHistoryPingPong[curr_buf_idx].GetSRV());
	cptContext.SetDynamicDescriptor(2, 6, g_DownSampledWorldNormalHistoryPingPong[curr_buf_idx].GetSRV());
									
	cptContext.SetDynamicDescriptor(2, 7, GetGlobalResource().STBNScalar.GetSRV());

	cptContext.SetDynamicDescriptor(3, 0, g_ReservoirRayDirectionSpatialPingPong[write_reservoir_index].GetUAV());
	cptContext.SetDynamicDescriptor(3, 1, g_ReservoirRayRadianceSpatialPingPong[write_reservoir_index].GetUAV());
	cptContext.SetDynamicDescriptor(3, 2, g_ReservoirRayDistanceSpatialPingPong[write_reservoir_index].GetUAV());
	cptContext.SetDynamicDescriptor(3, 3, g_ReservoirRayNormalSpatialPingPong[write_reservoir_index].GetUAV());
	cptContext.SetDynamicDescriptor(3, 4, g_ReservoirRayWeightsSpatialPingPong[write_reservoir_index].GetUAV());

	cptContext.Dispatch(DeviceAndRoundUp(GetGlobalResource().restirSceneInfo.g_restir_texturesize.x , 16), DeviceAndRoundUp( GetGlobalResource().restirSceneInfo.g_restir_texturesize.y , 16), 1);
}
