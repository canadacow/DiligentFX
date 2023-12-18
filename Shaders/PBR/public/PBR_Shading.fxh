#ifndef _PBR_SHADING_FXH_
#define _PBR_SHADING_FXH_

#include "PBR_Structures.fxh"
#include "PBR_Common.fxh"
#include "ShaderUtilities.fxh"
#include "SRGBUtilities.fxh"

#ifndef TEX_COLOR_CONVERSION_MODE_NONE
#   define TEX_COLOR_CONVERSION_MODE_NONE 0
#endif

#ifndef TEX_COLOR_CONVERSION_MODE_SRGB_TO_LINEAR
#   define TEX_COLOR_CONVERSION_MODE_SRGB_TO_LINEAR 1
#endif

#ifndef TEX_COLOR_CONVERSION_MODE
#   define TEX_COLOR_CONVERSION_MODE TEX_COLOR_CONVERSION_MODE_SRGB_TO_LINEAR
#endif

#if TEX_COLOR_CONVERSION_MODE == TEX_COLOR_CONVERSION_MODE_NONE
#   define  TO_LINEAR(x) (x)
#elif TEX_COLOR_CONVERSION_MODE == TEX_COLOR_CONVERSION_MODE_SRGB_TO_LINEAR
#   define  TO_LINEAR FastSRGBToLinear
#endif

#ifndef USE_IBL_ENV_MAP_LOD
#   define USE_IBL_ENV_MAP_LOD 1
#endif

#ifndef USE_HDR_IBL_CUBEMAPS
#   define USE_HDR_IBL_CUBEMAPS 1
#endif

float GetPerceivedBrightness(float3 rgb)
{
    return sqrt(0.299 * rgb.r * rgb.r + 0.587 * rgb.g * rgb.g + 0.114 * rgb.b * rgb.b);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js#L34
float SolveMetallic(float3 diffuse,
                    float3 specular,
                    float  oneMinusSpecularStrength)
{
    const float c_MinReflectance = 0.04;
    float specularBrightness = GetPerceivedBrightness(specular);
    if (specularBrightness < c_MinReflectance)
    {
        return 0.0;
    }

    float diffuseBrightness = GetPerceivedBrightness(diffuse);

    float a = c_MinReflectance;
    float b = diffuseBrightness * oneMinusSpecularStrength / (1.0 - c_MinReflectance) + specularBrightness - 2.0 * c_MinReflectance;
    float c = c_MinReflectance - specularBrightness;
    float D = b * b - 4.0 * a * c;

    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

float3 ApplyDirectionalLight(float3 lightDir, float3 lightColor, SurfaceReflectanceInfo srfInfo, float3 normal, float3 view)
{
    float3 pointToLight = -lightDir;
    float3 diffuseContrib, specContrib;
    float  NdotL;
    SmithGGX_BRDF(pointToLight, normal, view, srfInfo, diffuseContrib, specContrib, NdotL);
    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
    float3 shade = (diffuseContrib + specContrib) * NdotL;
    return lightColor * shade;
}

struct PerturbNormalInfo
{
    float3 dPos_dx;
    float3 dPos_dy;
    float3 Normal;
};

PerturbNormalInfo GetPerturbNormalInfo(in float3 Pos, in float3 Normal)
{
    PerturbNormalInfo Info;
    Info.dPos_dx = ddx(Pos);
    Info.dPos_dy = ddy(Pos);
    
    float NormalLen = length(Normal);
    if (NormalLen > 1e-5)
    {
        Info.Normal = Normal / NormalLen;
    }
    else
    {
        Info.Normal = normalize(cross(Info.dPos_dx, Info.dPos_dy));
#if (defined(GLSL) || defined(GL_ES)) && !defined(VULKAN)
        // In OpenGL, the screen is upside-down, so we have to invert the vector
        Info.Normal *= -1.0;
#endif
    }
    
    return Info;
}


// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
float3 PerturbNormal(PerturbNormalInfo NormalInfo,
                     in float2 dUV_dx,
                     in float2 dUV_dy,
                     in float3 TSNormal,
                     bool      HasUV,
                     bool      IsFrontFace)
{
    if (HasUV)
    {
        return TransformTangentSpaceNormalGrad(NormalInfo.dPos_dx, NormalInfo.dPos_dy, dUV_dx, dUV_dy, NormalInfo.Normal, TSNormal * (IsFrontFace ? +1.0 : -1.0));
    }
    else
    {
        return NormalInfo.Normal * (IsFrontFace ? +1.0 : -1.0);
    }
}


struct IBL_Contribution
{
    float3 f3Diffuse;
    float3 f3Specular;
};

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
float3 GetIBLRadianceGGX(in SurfaceReflectanceInfo SrfInfo,
                         in float3                 n,
                         in float3                 v,
                         in float                  PrefilteredCubeMipLevels,
                         in Texture2D              BRDF_LUT,
                         in SamplerState           BRDF_LUT_sampler,
                         in TextureCube            PrefilteredEnvMap,
                         in SamplerState           PrefilteredEnvMap_sampler)
{
    float NdotV = clamp(dot(n, v), 0.0, 1.0);

    float  lod        = clamp(SrfInfo.PerceptualRoughness * PrefilteredCubeMipLevels, 0.0, PrefilteredCubeMipLevels);
    float3 reflection = normalize(reflect(-v, n));

    float2 brdfSamplePoint = clamp(float2(NdotV, SrfInfo.PerceptualRoughness), float2(0.0, 0.0), float2(1.0, 1.0));
    // retrieve a scale and bias to F0. See [1], Figure 3
    float2 brdf = BRDF_LUT.Sample(BRDF_LUT_sampler, brdfSamplePoint).rg;

#if USE_IBL_ENV_MAP_LOD
    float3 SpecularSample = PrefilteredEnvMap.SampleLevel(PrefilteredEnvMap_sampler, reflection, lod).rgb;
#else
    float3 SpecularSample = PrefilteredEnvMap.Sample(PrefilteredEnvMap_sampler, reflection).rgb;
#endif

#if USE_HDR_IBL_CUBEMAPS
    // Already linear.
    float3 SpecularLight = SpecularSample.rgb;
#else
    float3 SpecularLight = TO_LINEAR(SpecularSample.rgb);
#endif

    return SpecularLight * (SrfInfo.Reflectance0 * brdf.x + SrfInfo.Reflectance90 * brdf.y);
}

IBL_Contribution GetIBLContribution(in SurfaceReflectanceInfo SrfInfo,
                                    in float3                 n,
                                    in float3                 v,
                                    in float                  PrefilteredCubeMipLevels,
                                    in Texture2D              BRDF_LUT,
                                    in SamplerState           BRDF_LUT_sampler,
                                    in TextureCube            IrradianceMap,
                                    in SamplerState           IrradianceMap_sampler,
                                    in TextureCube            PrefilteredEnvMap,
                                    in SamplerState           PrefilteredEnvMap_sampler)
{    
    IBL_Contribution IBLContrib;
    IBLContrib.f3Specular = GetIBLRadianceGGX(SrfInfo, n, v, PrefilteredCubeMipLevels,
                                              BRDF_LUT,
                                              BRDF_LUT_sampler,
                                              PrefilteredEnvMap,
                                              PrefilteredEnvMap_sampler);
    
    float3 DiffuseSample = IrradianceMap.Sample(IrradianceMap_sampler, n).rgb;
#if USE_HDR_IBL_CUBEMAPS
    // Already linear.
    float3 DiffuseLight  = DiffuseSample.rgb;
#else
    float3 DiffuseLight  = TO_LINEAR(DiffuseSample.rgb);
#endif
    IBLContrib.f3Diffuse  = DiffuseLight * SrfInfo.DiffuseColor;
    
    return IBLContrib;
}

/// Calculates surface reflectance info

/// \param [in]  Workflow     - PBR workflow (PBR_WORKFLOW_SPECULAR_GLOSINESS or PBR_WORKFLOW_METALLIC_ROUGHNESS).
/// \param [in]  BaseColor    - Material base color.
/// \param [in]  PhysicalDesc - Physical material description. For Metallic-roughness workflow,
///                             'g' channel stores roughness, 'b' channel stores metallic.
/// \param [out] Metallic     - Metallic value used for shading.
SurfaceReflectanceInfo GetSurfaceReflectance(int       Workflow,
                                             float4    BaseColor,
                                             float4    PhysicalDesc,
                                             out float Metallic)
{
    SurfaceReflectanceInfo SrfInfo;

    float3 SpecularColor;

    float3 f0 = float3(0.04, 0.04, 0.04);

    // Metallic and Roughness material properties are packed together
    // In glTF, these factors can be specified by fixed scalar values
    // or from a metallic-roughness map
    if (Workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
    {
        SrfInfo.PerceptualRoughness = 1.0 - PhysicalDesc.a; // glossiness to roughness
        f0 = PhysicalDesc.rgb;

        // f0 = specular
        SpecularColor = f0;
        float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
        SrfInfo.DiffuseColor = BaseColor.rgb * oneMinusSpecularStrength;

        // do conversion between metallic M-R and S-G metallic
        Metallic = SolveMetallic(BaseColor.rgb, SpecularColor, oneMinusSpecularStrength);
    }
    else if (Workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
    {
        // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
        // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
        SrfInfo.PerceptualRoughness = PhysicalDesc.g;
        Metallic                    = PhysicalDesc.b;

        SrfInfo.DiffuseColor  = BaseColor.rgb * (float3(1.0, 1.0, 1.0) - f0) * (1.0 - Metallic);
        SpecularColor         = lerp(f0, BaseColor.rgb, Metallic);
    }

//#ifdef ALPHAMODE_OPAQUE
//    baseColor.a = 1.0;
//#endif
//
//#ifdef MATERIAL_UNLIT
//    gl_FragColor = float4(gammaCorrection(baseColor.rgb), baseColor.a);
//    return;
//#endif

    SrfInfo.PerceptualRoughness = clamp(SrfInfo.PerceptualRoughness, 0.0, 1.0);

    // Compute reflectance.
    float3 Reflectance0  = SpecularColor;
    float  MaxR0         = max(max(Reflectance0.r, Reflectance0.g), Reflectance0.b);
    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    float R90 = clamp(MaxR0 * 50.0, 0.0, 1.0);

    SrfInfo.Reflectance0  = Reflectance0;
    SrfInfo.Reflectance90 = float3(R90, R90, R90);

    return SrfInfo;
}

/// Calculates surface reflectance info for Metallic-roughness workflow
SurfaceReflectanceInfo GetSurfaceReflectanceMR(float3 BaseColor,
                                               float  Metallic,
                                               float  Roughness)
{
    SurfaceReflectanceInfo SrfInfo;

    float f0 = 0.04;

    SrfInfo.PerceptualRoughness = Roughness;
    SrfInfo.DiffuseColor        = BaseColor * ((1.0 - f0) * (1.0 - Metallic));

    float3 Reflectance0 = lerp(float3(f0, f0, f0), BaseColor, Metallic);
    float  MaxR0        = max(max(Reflectance0.r, Reflectance0.g), Reflectance0.b);
    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    float R90 = min(MaxR0 * 50.0, 1.0);

    SrfInfo.Reflectance0  = Reflectance0;
    SrfInfo.Reflectance90 = float3(R90, R90, R90);

    return SrfInfo;
}

/// Gets surface reflectance for the clear coat layer
SurfaceReflectanceInfo GetSurfaceReflectanceClearCoat(float Roughness, float  IOR)
{
    SurfaceReflectanceInfo SrfInfo;

    float f0 = (IOR - 1.0) / (IOR + 1.0);
    f0 *= f0;

    SrfInfo.PerceptualRoughness = Roughness;
    SrfInfo.DiffuseColor        = float3(0.0, 0.0, 0.0);

    float R90 = 1.0;
    SrfInfo.Reflectance0  = float3(f0, f0, f0);
    SrfInfo.Reflectance90 = float3(R90, R90, R90);

    return SrfInfo;
}
#endif // _PBR_SHADING_FXH_
