#pragma once
#include "RestirCommon.h"

class CRestirIntegrateAndDenoise
{
public:
	void Init();
	void IntegrateAndDenoise(GraphicsContext& cptContext);
	void SimpleCombineLight(GraphicsContext& cptContext);
private:

	void RestirIntegrate(ComputeContext& cptContext);

	void RestirTemporalDenoise(ComputeContext& cptContext);
	void RestirSpatialDenoise(ComputeContext& cptContext);

	RootSignature restirIntegrateSig;
	ComputePSO restirIntegratePso;

	RootSignature restirTemporalDenoiseSig;
	ComputePSO restirTemporalDenoisePso;

	RootSignature restirSpatialDenoiseSig;
	ComputePSO restirSpatialDenoisePso;

	RootSignature restirCopySig;
	ComputePSO restirCopyPso;

	RootSignature restirCombineSig;
	ComputePSO restirCombinePso;
};