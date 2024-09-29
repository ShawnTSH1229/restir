#pragma once
#include "RestirCommon.h"
#include "ShaderCompile.h"

struct SRestirRayTracingInfo
{
	DirectX::XMFLOAT3 lightDirection;
	float padding1;

	DirectX::XMFLOAT4 restirTextureSize;
	DirectX::XMFLOAT4 fullScreenTextureSize;

	uint32_t currentFrameIndex = 0;
};

struct SRestirSceneInfo
{
	Math::Matrix4 PreViewProjMatrix;
	DirectX::XMFLOAT2 g_restir_texturesize;
	DirectX::XMFLOAT2 g_full_screen_texsize;
	DirectX::XMFLOAT3 g_sun_direction;
	uint32_t g_current_frame_index = 0;
	DirectX::XMFLOAT3 g_sun_intensity;
	float restirpadding0;
	DirectX::XMFLOAT3 g_camera_worldpos;
	float restirpadding1;
};

struct SGlobalResource
{
	CShaderCompiler m_ShaderCompilerDxc;
	CShaderCompiler m_ShaderCompilerFxc;
	__declspec(align(16)) SRestirRayTracingInfo restirSRayTracingInfo;
	__declspec(align(16)) SRestirSceneInfo restirSceneInfo;
	ModelInstance* pModelInst;
	
	TextureRef STBNScalar;
	TextureRef STBNVec2;

	inline int getCurrentFrameBufferIndex() { return (currentFrameIndex % 3); }
	inline void incrementFrameIndex() { currentFrameIndex++;; }
	inline int getCurrentFrameIndex() { return currentFrameIndex; };

private:
	int currentFrameIndex = 0;
};

SGlobalResource& GetGlobalResource();
void InitGlobalResource();
