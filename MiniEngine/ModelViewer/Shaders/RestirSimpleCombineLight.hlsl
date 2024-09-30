#define GROUP_SIZE 16
#include "ReSTIRCommon.hlsl"

Texture2D<float4> scene_color_read : register(t0);//xyz only
Texture2D<float4> diffuse_indirect : register(t1);//xyz only
Texture2D<float4> specular_indirect : register(t2);//xyz only
Texture2D<float4> gbufferc : register(t3);//xyz only

RWTexture2D<float4> scene_color_dst : register(u0);//xyz only

cbuffer CBRestirSceneInfo : register(b0)
{
    RESTIR_SCENE_INFO_COMMON
};

float3 min3(float3 A, float3 B)
{
    return float3(min(A.x,B.x),min(A.y,B.y),min(A.z,B.z));
}

float3 max3(float3 A, float3 B)
{
    return float3(max(A.x,B.x),max(A.y,B.y),max(A.z,B.z));
}

[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void ReSTIRSimpleCombineCS(uint2 dispatch_thread_id : SV_DispatchThreadID)
{
    uint2 current_pixel_pos = dispatch_thread_id;
    if(all(current_pixel_pos < g_full_screen_texsize))
    {
        // hack here!!
        float3 basecolor = gbufferc[current_pixel_pos].xyz;
        float3 metalic = 0.35;
        float3 diffuse = basecolor * (1.0 - metalic);
        float3 specular = basecolor * (metalic);

        float3 diffuse_indirect_color = diffuse_indirect[current_pixel_pos].xyz;
        float3 sepcular_indirect_color = specular_indirect[current_pixel_pos].xyz;

        if(any(diffuse_indirect_color > float3(16,16,16)) || any(diffuse_indirect_color < float3(0.0,0.0,0.0)))
        {
            // todo fix me! inf and nan pixels
            diffuse_indirect_color = float3(0,0,0);
        }

        if(any(sepcular_indirect_color > float3(16,16,16)) || any(sepcular_indirect_color < float3(0.0,0.0,0.0)))
        {
            // todo fix me! inf and nan pixels
            sepcular_indirect_color = float3(0,0,0);
        }

        float3 simple_combine_indirect_light = 
        min3(max3(diffuse_indirect_color,float3(0.0,0.0,0.0)),float3(5,5,5)) * diffuse * (1.0 / 3.1415926535);
        min3(max3(sepcular_indirect_color,float3(0.0,0.0,0.0)),float3(5,5,5)) * specular;
        scene_color_dst[current_pixel_pos] = float4(scene_color_read[current_pixel_pos].xyz + simple_combine_indirect_light,0.0);
    }
}