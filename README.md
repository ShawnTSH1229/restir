# ReSTIR

[<u>**ReSTIR Development Blog**</u>](https://shawntsh1229.github.io/2024/10/01/ReSTIR-SVGF-Sim-Spatiotemporal-Reservoir-Resampling-With-Spatiotemporal-Filtering-In-MiniEngine/)

# Introduction

We implement a global algorithm combining ReSTIR and spatiotemporal filtering based on Unreal Engine and ReSTIR GI(SiG 20). At first, we initial the reservoir samples using hardware raytracing at half resolution. Then we perform resampled importance sampling (RIS) to draw samples generated from sample importance resampling (SIR). To reduce time complexity O(M) and space complexity O(M), we use weighted reservoir sampling (WRS) to improve performance. After that, we reuse reservoir samples spatially and generate diffuse/specular lighting based on those samples. Finally, we perform temporal filter and spatial filter (joint bilateral filter) to denoise the indirect lighting results.

# Getting Started

1.Cloning the repository with `git clone https://github.com/ShawnTSH1229/restir.git`.

2.Compile and run the visual studio solution located in `restir\MiniEngine\ModelViewer\Restir.sln`

# Screenshot

Initial Sample Ray Direction:
<p align="left">
    <img src="/Resources/reservoir_ray_dir.png" width="60%" height="60%">
</p>

Initial Sample Ray Hit Distance:
<p align="left">
    <img src="/Resources/reservoir_ray_hit_dist.png" width="60%" height="60%">
</p>

Initial Sample Ray Hit Normal:
<p align="left">
    <img src="/Resources/reservoir_ray_normal.png" width="60%" height="60%">
</p>

Initial Sample Ray Hit Radiance:
<p align="left">
    <img src="/Resources/reservoir_ray_radiance.png" width="60%" height="60%">
</p>

Reservoir(Weight, M, Inverse SIR PDF):
<p align="left">
    <img src="/Resources/reservoir_ray_weights.png" width="60%" height="60%">
</p>

Reservoir picked sample radiance(before temporal resuing VS after temporal reuse):

<p align="left">
    <img src="/Resources/reservoir_ray_radiance.png" width="60%" height="60%">
</p>

<p align="left">
    <img src="/Resources/reservoir_ray_radiance_after_t_resuse.png" width="60%" height="60%">
</p>

Reservoir picked sample radiance(before spatial resuing VS after spatial reuse):

<p align="left">
    <img src="/Resources/before_sp_reused.png" width="60%" height="60%">
</p>

<p align="left">
    <img src="/Resources/after_sp_reused.png" width="60%" height="60%">
</p>

Diffuse Indirect Lighting(Scaled) and Specular Indirect Lighting(Scaled)

<p align="left">
    <img src="/Resources/diff_indirect.png" width="60%" height="60%">
</p>

<p align="left">
    <img src="/Resources/spec_indirect.png" width="60%" height="60%">
</p>

Denoised Diffuse Indirect Lighting(Scaled) and Denoised Specular Indirect Lighting(Scaled)

<p align="left">
    <img src="/Resources/denoise_diff.png" width="60%" height="60%">
</p>

<p align="left">
    <img src="/Resources/denoise_spec.png" width="60%" height="60%">
</p>

Enable ReSTIR VS Disable ReSTIR(Direct Lighting Only)

<p align="left">
    <img src="/Resources/final_result.png" width="60%" height="60%">
</p>

<p align="left">
    <img src="/Resources/direct_lighting_only.png" width="60%" height="60%">
</p>





