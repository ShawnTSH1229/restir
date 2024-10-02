#define GROUP_SIZE 16
#include "ReSTIRCommon.hlsl"

cbuffer CBRestirSceneInfo : register(b0)
{
    RESTIR_SCENE_INFO_COMMON
};

Texture2D<float4> hist_diffuse_indirect : register(t0);//xyz only
Texture2D<float4> hist_specular_indirect : register(t1);//xyz only
Texture2D<float4> gbuffer_world_pos : register(t2); //gbuffer b

SamplerState defaultLinearSampler       	: register(s0);

RWTexture2D<float4> output_diffuse_indirect : register(u0);//xyz only
RWTexture2D<float4> output_specular_indirect : register(u1);//xyz only

[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void ReSTIRTemporalDenoiseCS(uint2 dispatch_thread_id : SV_DispatchThreadID)
{
    uint2 current_pixel_pos = dispatch_thread_id;
    
    float3 filtered_diffuse_indirect = float3(0,0,0);
    float3 filtered_specular_indirect = float3(0,0,0);

    if(all(current_pixel_pos < g_full_screen_texsize))
    {
        float3 world_position = gbuffer_world_pos[current_pixel_pos].xyz;
        if(any(world_position != float3(0,0,0)))
        {
            float4 pre_view_pos = mul(PreViewProjMatrix,float4(world_position, 1.0));
            float2 pre_view_screen_pos = (float2(pre_view_pos.xy / pre_view_pos.w) * 0.5 + float2(0.5,0.5));
            pre_view_screen_pos.y = (1.0 - pre_view_screen_pos.y);
            float2 histUV = pre_view_screen_pos;

            float3 hist_diffuse = hist_diffuse_indirect.SampleLevel(defaultLinearSampler, histUV,0).xyz;
            float3 hist_specular = hist_specular_indirect.SampleLevel(defaultLinearSampler, histUV,0).xyz;

            float3 current_diffuse = output_diffuse_indirect[current_pixel_pos].xyz;
            float3 current_specular = output_specular_indirect[current_pixel_pos].xyz;

            //todo: temporal sample validation, which is similar to svgf
            output_diffuse_indirect[current_pixel_pos] = float4(lerp(hist_diffuse, current_diffuse, 0.05f),1.0);
            output_specular_indirect[current_pixel_pos] = float4(lerp(hist_specular, current_specular, 0.05f),1.0);

        }
    }
}