#define GROUP_SIZE 16
#include "ReSTIRCommon.hlsl"

Texture2D<float4> scene_color_src : register(t0);//xyz only
RWTexture2D<float4> scene_color_dst : register(u0);//xyz only

cbuffer CBRestirSceneInfo : register(b0)
{
    RESTIR_SCENE_INFO_COMMON
};
[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void ReSTIRSceneColorCopyCS(uint2 dispatch_thread_id : SV_DispatchThreadID)
{
    uint2 current_pixel_pos = dispatch_thread_id;
    if(all(current_pixel_pos < g_full_screen_texsize))
    {
        scene_color_dst[current_pixel_pos] = float4(scene_color_src[current_pixel_pos].xyz,0.0);
    }
}