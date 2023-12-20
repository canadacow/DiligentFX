"#ifndef _PBR_SHADING_FXH_\n"
"#define _PBR_SHADING_FXH_\n"
"\n"
"#include \"PBR_Structures.fxh\"\n"
"#include \"PBR_Common.fxh\"\n"
"#include \"ShaderUtilities.fxh\"\n"
"#include \"SRGBUtilities.fxh\"\n"
"\n"
"#ifndef TEX_COLOR_CONVERSION_MODE_NONE\n"
"#   define TEX_COLOR_CONVERSION_MODE_NONE 0\n"
"#endif\n"
"\n"
"#ifndef TEX_COLOR_CONVERSION_MODE_SRGB_TO_LINEAR\n"
"#   define TEX_COLOR_CONVERSION_MODE_SRGB_TO_LINEAR 1\n"
"#endif\n"
"\n"
"#ifndef TEX_COLOR_CONVERSION_MODE\n"
"#   define TEX_COLOR_CONVERSION_MODE TEX_COLOR_CONVERSION_MODE_SRGB_TO_LINEAR\n"
"#endif\n"
"\n"
"#if TEX_COLOR_CONVERSION_MODE == TEX_COLOR_CONVERSION_MODE_NONE\n"
"#   define  TO_LINEAR(x) (x)\n"
"#elif TEX_COLOR_CONVERSION_MODE == TEX_COLOR_CONVERSION_MODE_SRGB_TO_LINEAR\n"
"#   define  TO_LINEAR FastSRGBToLinear\n"
"#endif\n"
"\n"
"#ifndef USE_IBL_ENV_MAP_LOD\n"
"#   define USE_IBL_ENV_MAP_LOD 1\n"
"#endif\n"
"\n"
"#ifndef USE_HDR_IBL_CUBEMAPS\n"
"#   define USE_HDR_IBL_CUBEMAPS 1\n"
"#endif\n"
"\n"
"float GetPerceivedBrightness(float3 rgb)\n"
"{\n"
"    return sqrt(0.299 * rgb.r * rgb.r + 0.587 * rgb.g * rgb.g + 0.114 * rgb.b * rgb.b);\n"
"}\n"
"\n"
"// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js#L34\n"
"float SolveMetallic(float3 diffuse,\n"
"                    float3 specular,\n"
"                    float  oneMinusSpecularStrength)\n"
"{\n"
"    const float c_MinReflectance = 0.04;\n"
"    float specularBrightness = GetPerceivedBrightness(specular);\n"
"    if (specularBrightness < c_MinReflectance)\n"
"    {\n"
"        return 0.0;\n"
"    }\n"
"\n"
"    float diffuseBrightness = GetPerceivedBrightness(diffuse);\n"
"\n"
"    float a = c_MinReflectance;\n"
"    float b = diffuseBrightness * oneMinusSpecularStrength / (1.0 - c_MinReflectance) + specularBrightness - 2.0 * c_MinReflectance;\n"
"    float c = c_MinReflectance - specularBrightness;\n"
"    float D = b * b - 4.0 * a * c;\n"
"\n"
"    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);\n"
"}\n"
"\n"
"float3 ApplyDirectionalLightGGX(float3 lightDir, float3 lightColor, SurfaceReflectanceInfo srfInfo, float3 N, float3 V)\n"
"{\n"
"    float3 L = -lightDir;\n"
"    float3 diffuseContrib, specContrib;\n"
"    float  NdotL;\n"
"    SmithGGX_BRDF(L, N, V, srfInfo, diffuseContrib, specContrib, NdotL);\n"
"    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)\n"
"    float3 shade = (diffuseContrib + specContrib) * NdotL;\n"
"    return lightColor * shade;\n"
"}\n"
"\n"
"float3 ApplyDirectionalLightSheen(float3 lightDir, float3 lightColor, float3 SheenColor, float SheenRoughness, float3 N, float3 V)\n"
"{\n"
"    float3 L = -lightDir;\n"
"    float3 H = normalize(V + L);\n"
"    float NdotL = saturate(dot(N, L));\n"
"    float NdotV = saturate(dot(N, V));\n"
"    float NdotH = saturate(dot(N, H));\n"
"\n"
"    return lightColor * NdotL * SheenSpecularBRDF(SheenColor, SheenRoughness, NdotL, NdotV, NdotH);\n"
"}\n"
"\n"
"struct PerturbNormalInfo\n"
"{\n"
"    float3 dPos_dx;\n"
"    float3 dPos_dy;\n"
"    float3 Normal;\n"
"};\n"
"\n"
"PerturbNormalInfo GetPerturbNormalInfo(in float3 Pos, in float3 Normal)\n"
"{\n"
"    PerturbNormalInfo Info;\n"
"    Info.dPos_dx = ddx(Pos);\n"
"    Info.dPos_dy = ddy(Pos);\n"
"\n"
"    float NormalLen = length(Normal);\n"
"    if (NormalLen > 1e-5)\n"
"    {\n"
"        Info.Normal = Normal / NormalLen;\n"
"    }\n"
"    else\n"
"    {\n"
"        Info.Normal = normalize(cross(Info.dPos_dx, Info.dPos_dy));\n"
"#if (defined(GLSL) || defined(GL_ES)) && !defined(VULKAN)\n"
"        // In OpenGL, the screen is upside-down, so we have to invert the vector\n"
"        Info.Normal *= -1.0;\n"
"#endif\n"
"    }\n"
"\n"
"    return Info;\n"
"}\n"
"\n"
"\n"
"// Find the normal for this fragment, pulling either from a predefined normal map\n"
"// or from the interpolated mesh normal and tangent attributes.\n"
"float3 PerturbNormal(PerturbNormalInfo NormalInfo,\n"
"                     in float2 dUV_dx,\n"
"                     in float2 dUV_dy,\n"
"                     in float3 TSNormal,\n"
"                     bool      HasUV,\n"
"                     bool      IsFrontFace)\n"
"{\n"
"    if (HasUV)\n"
"    {\n"
"        return TransformTangentSpaceNormalGrad(NormalInfo.dPos_dx, NormalInfo.dPos_dy, dUV_dx, dUV_dy, NormalInfo.Normal, TSNormal * (IsFrontFace ? +1.0 : -1.0));\n"
"    }\n"
"    else\n"
"    {\n"
"        return NormalInfo.Normal * (IsFrontFace ? +1.0 : -1.0);\n"
"    }\n"
"}\n"
"\n"
"\n"
"struct IBL_Contribution\n"
"{\n"
"    float3 f3Diffuse;\n"
"    float3 f3Specular;\n"
"};\n"
"\n"
"// Calculation of the lighting contribution from an optional Image Based Light source.\n"
"// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].\n"
"// See our README.md on Environment Maps [3] for additional discussion.\n"
"float3 GetIBLRadiance(in SurfaceReflectanceInfo SrfInfo,\n"
"                      in float3                 n,\n"
"                      in float3                 v,\n"
"                      in float                  PrefilteredCubeMipLevels,\n"
"                      in Texture2D              PreintegratedBRDF,\n"
"                      in SamplerState           PreintegratedBRDF_sampler,\n"
"                      in TextureCube            PrefilteredEnvMap,\n"
"                      in SamplerState           PrefilteredEnvMap_sampler)\n"
"{\n"
"    float NdotV = clamp(dot(n, v), 0.0, 1.0);\n"
"\n"
"    float  lod        = clamp(SrfInfo.PerceptualRoughness * PrefilteredCubeMipLevels, 0.0, PrefilteredCubeMipLevels);\n"
"    float3 reflection = normalize(reflect(-v, n));\n"
"\n"
"    float2 brdfSamplePoint = clamp(float2(NdotV, SrfInfo.PerceptualRoughness), float2(0.0, 0.0), float2(1.0, 1.0));\n"
"    // retrieve a scale and bias to F0. See [1], Figure 3\n"
"    float2 brdf = PreintegratedBRDF.Sample(PreintegratedBRDF_sampler, brdfSamplePoint).rg;\n"
"\n"
"#if USE_IBL_ENV_MAP_LOD\n"
"    float3 SpecularSample = PrefilteredEnvMap.SampleLevel(PrefilteredEnvMap_sampler, reflection, lod).rgb;\n"
"#else\n"
"    float3 SpecularSample = PrefilteredEnvMap.Sample(PrefilteredEnvMap_sampler, reflection).rgb;\n"
"#endif\n"
"\n"
"#if USE_HDR_IBL_CUBEMAPS\n"
"    // Already linear.\n"
"    float3 SpecularLight = SpecularSample.rgb;\n"
"#else\n"
"    float3 SpecularLight = TO_LINEAR(SpecularSample.rgb);\n"
"#endif\n"
"\n"
"    return SpecularLight * (SrfInfo.Reflectance0 * brdf.x + SrfInfo.Reflectance90 * brdf.y);\n"
"}\n"
"\n"
"IBL_Contribution GetIBLContribution(in SurfaceReflectanceInfo SrfInfo,\n"
"                                    in float3                 n,\n"
"                                    in float3                 v,\n"
"                                    in float                  PrefilteredCubeMipLevels,\n"
"                                    in Texture2D              PreintegratedBRDF,\n"
"                                    in SamplerState           PreintegratedBRDF_sampler,\n"
"                                    in TextureCube            IrradianceMap,\n"
"                                    in SamplerState           IrradianceMap_sampler,\n"
"                                    in TextureCube            PrefilteredEnvMap,\n"
"                                    in SamplerState           PrefilteredEnvMap_sampler)\n"
"{\n"
"    IBL_Contribution IBLContrib;\n"
"    IBLContrib.f3Specular = GetIBLRadiance(SrfInfo, n, v, PrefilteredCubeMipLevels,\n"
"                                           PreintegratedBRDF,\n"
"                                           PreintegratedBRDF_sampler,\n"
"                                           PrefilteredEnvMap,\n"
"                                           PrefilteredEnvMap_sampler);\n"
"\n"
"    float3 DiffuseSample = IrradianceMap.Sample(IrradianceMap_sampler, n).rgb;\n"
"#if USE_HDR_IBL_CUBEMAPS\n"
"    // Already linear.\n"
"    float3 DiffuseLight  = DiffuseSample.rgb;\n"
"#else\n"
"    float3 DiffuseLight  = TO_LINEAR(DiffuseSample.rgb);\n"
"#endif\n"
"    IBLContrib.f3Diffuse  = DiffuseLight * SrfInfo.DiffuseColor;\n"
"\n"
"    return IBLContrib;\n"
"}\n"
"\n"
"/// Calculates surface reflectance info\n"
"\n"
"/// \\param [in]  Workflow     - PBR workflow (PBR_WORKFLOW_SPECULAR_GLOSINESS or PBR_WORKFLOW_METALLIC_ROUGHNESS).\n"
"/// \\param [in]  BaseColor    - Material base color.\n"
"/// \\param [in]  PhysicalDesc - Physical material description. For Metallic-roughness workflow,\n"
"///                             \'g\' channel stores roughness, \'b\' channel stores metallic.\n"
"/// \\param [out] Metallic     - Metallic value used for shading.\n"
"SurfaceReflectanceInfo GetSurfaceReflectance(int       Workflow,\n"
"                                             float4    BaseColor,\n"
"                                             float4    PhysicalDesc,\n"
"                                             out float Metallic)\n"
"{\n"
"    SurfaceReflectanceInfo SrfInfo;\n"
"\n"
"    float3 SpecularColor;\n"
"\n"
"    float3 f0 = float3(0.04, 0.04, 0.04);\n"
"\n"
"    // Metallic and Roughness material properties are packed together\n"
"    // In glTF, these factors can be specified by fixed scalar values\n"
"    // or from a metallic-roughness map\n"
"    if (Workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)\n"
"    {\n"
"        SrfInfo.PerceptualRoughness = 1.0 - PhysicalDesc.a; // glossiness to roughness\n"
"        f0 = PhysicalDesc.rgb;\n"
"\n"
"        // f0 = specular\n"
"        SpecularColor = f0;\n"
"        float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);\n"
"        SrfInfo.DiffuseColor = BaseColor.rgb * oneMinusSpecularStrength;\n"
"\n"
"        // do conversion between metallic M-R and S-G metallic\n"
"        Metallic = SolveMetallic(BaseColor.rgb, SpecularColor, oneMinusSpecularStrength);\n"
"    }\n"
"    else if (Workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)\n"
"    {\n"
"        // Roughness is stored in the \'g\' channel, metallic is stored in the \'b\' channel.\n"
"        // This layout intentionally reserves the \'r\' channel for (optional) occlusion map data\n"
"        SrfInfo.PerceptualRoughness = PhysicalDesc.g;\n"
"        Metallic                    = PhysicalDesc.b;\n"
"\n"
"        SrfInfo.DiffuseColor  = BaseColor.rgb * (float3(1.0, 1.0, 1.0) - f0) * (1.0 - Metallic);\n"
"        SpecularColor         = lerp(f0, BaseColor.rgb, Metallic);\n"
"    }\n"
"\n"
"//#ifdef ALPHAMODE_OPAQUE\n"
"//    baseColor.a = 1.0;\n"
"//#endif\n"
"//\n"
"//#ifdef MATERIAL_UNLIT\n"
"//    gl_FragColor = float4(gammaCorrection(baseColor.rgb), baseColor.a);\n"
"//    return;\n"
"//#endif\n"
"\n"
"    SrfInfo.PerceptualRoughness = clamp(SrfInfo.PerceptualRoughness, 0.0, 1.0);\n"
"\n"
"    // Compute reflectance.\n"
"    float3 Reflectance0  = SpecularColor;\n"
"    float  MaxR0         = max(max(Reflectance0.r, Reflectance0.g), Reflectance0.b);\n"
"    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to \"Real-Time-Rendering\" 4th editon on page 325.\n"
"    float R90 = clamp(MaxR0 * 50.0, 0.0, 1.0);\n"
"\n"
"    SrfInfo.Reflectance0  = Reflectance0;\n"
"    SrfInfo.Reflectance90 = float3(R90, R90, R90);\n"
"\n"
"    return SrfInfo;\n"
"}\n"
"\n"
"/// Calculates surface reflectance info for Metallic-roughness workflow\n"
"SurfaceReflectanceInfo GetSurfaceReflectanceMR(float3 BaseColor,\n"
"                                               float  Metallic,\n"
"                                               float  Roughness)\n"
"{\n"
"    SurfaceReflectanceInfo SrfInfo;\n"
"\n"
"    float f0 = 0.04;\n"
"\n"
"    SrfInfo.PerceptualRoughness = Roughness;\n"
"    SrfInfo.DiffuseColor        = BaseColor * ((1.0 - f0) * (1.0 - Metallic));\n"
"\n"
"    float3 Reflectance0 = lerp(float3(f0, f0, f0), BaseColor, Metallic);\n"
"    float  MaxR0        = max(max(Reflectance0.r, Reflectance0.g), Reflectance0.b);\n"
"    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to \"Real-Time-Rendering\" 4th editon on page 325.\n"
"    float R90 = min(MaxR0 * 50.0, 1.0);\n"
"\n"
"    SrfInfo.Reflectance0  = Reflectance0;\n"
"    SrfInfo.Reflectance90 = float3(R90, R90, R90);\n"
"\n"
"    return SrfInfo;\n"
"}\n"
"\n"
"/// Gets surface reflectance for the clear coat layer\n"
"SurfaceReflectanceInfo GetSurfaceReflectanceClearCoat(float Roughness, float  IOR)\n"
"{\n"
"    SurfaceReflectanceInfo SrfInfo;\n"
"\n"
"    float f0 = (IOR - 1.0) / (IOR + 1.0);\n"
"    f0 *= f0;\n"
"\n"
"    SrfInfo.PerceptualRoughness = Roughness;\n"
"    SrfInfo.DiffuseColor        = float3(0.0, 0.0, 0.0);\n"
"\n"
"    float R90 = 1.0;\n"
"    SrfInfo.Reflectance0  = float3(f0, f0, f0);\n"
"    SrfInfo.Reflectance90 = float3(R90, R90, R90);\n"
"\n"
"    return SrfInfo;\n"
"}\n"
"\n"
"\n"
"struct BaseLayerShadingInfo\n"
"{\n"
"    SurfaceReflectanceInfo Srf;\n"
"\n"
"    float Metallic;\n"
"\n"
"    // Shading normal in world space\n"
"    float3 Normal;\n"
"\n"
"    float NdotV;\n"
"};\n"
"\n"
"struct ClearcoatShadingInfo\n"
"{\n"
"    SurfaceReflectanceInfo Srf;\n"
"\n"
"    // Clearcoat normal in world space\n"
"    float3 Normal;\n"
"\n"
"    float Factor;\n"
"};\n"
"\n"
"struct SheenShadingInfo\n"
"{\n"
"    float3 Color;\n"
"    float  Roughness;\n"
"};\n"
"\n"
"struct IridescenceShadingInfo\n"
"{\n"
"    float Factor;\n"
"    float Thickness;\n"
"    float ThicknessMinimum;\n"
"    float ThicknessMaximum;\n"
"};\n"
"\n"
"struct SurfaceShadingInfo\n"
"{\n"
"    // Camera view direction in world space\n"
"    float3 View;\n"
"\n"
"    float  Occlusion;\n"
"    float3 Emissive;\n"
"\n"
"    BaseLayerShadingInfo BaseLayer;\n"
"\n"
"#if ENABLE_CLEAR_COAT\n"
"    ClearcoatShadingInfo Clearcoat;\n"
"#endif\n"
"\n"
"#if ENABLE_SHEEN\n"
"    SheenShadingInfo Sheen;\n"
"#endif\n"
"\n"
"#if ENABLE_ANISOTROPY\n"
"    float3 Anisotropy;\n"
"#endif\n"
"\n"
"#if ENABLE_IRIDESCENCE\n"
"    IridescenceShadingInfo Iridescence;\n"
"#endif\n"
"\n"
"#if ENABLE_TRANSMISSION\n"
"    float Transmission;\n"
"#endif\n"
"\n"
"#if ENABLE_VOLUME\n"
"    float VolumeThickness;\n"
"#endif\n"
"\n"
"    float IBLScale;\n"
"    float OcclusionStrength;\n"
"    float EmissionScale;\n"
"};\n"
"\n"
"struct LayerLightingInfo\n"
"{\n"
"    float3 Punctual;\n"
"\n"
"    IBL_Contribution IBL;\n"
"};\n"
"\n"
"struct SurfaceLightingInfo\n"
"{\n"
"    LayerLightingInfo Base;\n"
"\n"
"#if ENABLE_SHEEN\n"
"    LayerLightingInfo Sheen;\n"
"#endif\n"
"\n"
"#if ENABLE_CLEAR_COAT\n"
"    LayerLightingInfo Clearcoat;\n"
"#endif\n"
"};\n"
"\n"
"LayerLightingInfo GetDefaultLayerLightingInfo()\n"
"{\n"
"    LayerLightingInfo Lighting;\n"
"    Lighting.Punctual       = float3(0.0, 0.0, 0.0);\n"
"    Lighting.IBL.f3Diffuse  = float3(0.0, 0.0, 0.0);\n"
"    Lighting.IBL.f3Specular = float3(0.0, 0.0, 0.0);\n"
"    return Lighting;\n"
"}\n"
"\n"
"SurfaceLightingInfo GetDefaultSurfaceLightingInfo()\n"
"{\n"
"    SurfaceLightingInfo Lighting;\n"
"    Lighting.Base = GetDefaultLayerLightingInfo();\n"
"\n"
"#if ENABLE_SHEEN\n"
"    Lighting.Sheen = GetDefaultLayerLightingInfo();\n"
"#endif\n"
"\n"
"#if ENABLE_CLEAR_COAT\n"
"    Lighting.Clearcoat = GetDefaultLayerLightingInfo();\n"
"#endif\n"
"\n"
"    return Lighting;\n"
"}\n"
"\n"
"void ApplyPunctualLights(in    SurfaceShadingInfo  Shading,\n"
"                         in    PBRLightAttribs     Light,\n"
"#if ENABLE_SHEEN\n"
"                         in    Texture2D           AlbedoScalingLUT,\n"
"                         in    SamplerState        AlbedoScalingLUT_sampler,\n"
"#endif\n"
"                         inout SurfaceLightingInfo SrfLighting)\n"
"{\n"
"    //#ifdef USE_PUNCTUAL\n"
"    //    for (int i = 0; i < LIGHT_COUNT; ++i)\n"
"    //    {\n"
"    //        Light light = u_Lights[i];\n"
"    //        if (light.type == LightType_Directional)\n"
"    //        {\n"
"    //            color += applyDirectionalLight(light, materialInfo, normal, view);\n"
"    //        }\n"
"    //        else if (light.type == LightType_Point)\n"
"    //        {\n"
"    //            color += applyPointLight(light, materialInfo, normal, view);\n"
"    //        }\n"
"    //        else if (light.type == LightType_Spot)\n"
"    //        {\n"
"    //            color += applySpotLight(light, materialInfo, normal, view);\n"
"    //        }\n"
"    //    }\n"
"    //#endif\n"
"\n"
"    float NdotV = Shading.BaseLayer.NdotV;\n"
"    float NdotL = saturate(dot(Shading.BaseLayer.Normal, -Light.Direction.xyz));\n"
"\n"
"    float3 BasePunctual = ApplyDirectionalLightGGX(Light.Direction.xyz, Light.Intensity.rgb, Shading.BaseLayer.Srf, Shading.BaseLayer.Normal, Shading.View);\n"
"\n"
"#if ENABLE_SHEEN\n"
"    {\n"
"        SrfLighting.Sheen.Punctual += ApplyDirectionalLightSheen(Light.Direction.xyz, Light.Intensity.rgb, Shading.Sheen.Color, Shading.Sheen.Roughness, Shading.BaseLayer.Normal, Shading.View);\n"
"\n"
"        float MaxFactor = max(max(Shading.Sheen.Color.r, Shading.Sheen.Color.g), Shading.Sheen.Color.b);\n"
"        float AlbedoScaling =\n"
"            min(1.0 - MaxFactor * AlbedoScalingLUT.Sample(AlbedoScalingLUT_sampler, float2(NdotV, Shading.Sheen.Roughness)).r,\n"
"                1.0 - MaxFactor * AlbedoScalingLUT.Sample(AlbedoScalingLUT_sampler, float2(NdotL, Shading.Sheen.Roughness)).r);\n"
"        BasePunctual *= AlbedoScaling;\n"
"    }\n"
"#endif\n"
"\n"
"    SrfLighting.Base.Punctual += BasePunctual;\n"
"\n"
"#if ENABLE_CLEAR_COAT\n"
"    {\n"
"        SrfLighting.Clearcoat.Punctual += ApplyDirectionalLightGGX(Light.Direction.xyz, Light.Intensity.rgb, Shading.Clearcoat.Srf, Shading.Clearcoat.Normal, Shading.View);\n"
"    }\n"
"#endif\n"
"}\n"
"\n"
"#if USE_IBL\n"
"void ApplyIBL(in SurfaceShadingInfo Shading,\n"
"              in float              PrefilteredCubeMipLevels,\n"
"              in Texture2D          PreintegratedGGX,\n"
"              in SamplerState       PreintegratedGGX_sampler,\n"
"              in TextureCube        IrradianceMap,\n"
"              in SamplerState       IrradianceMap_sampler,\n"
"              in TextureCube        PrefilteredEnvMap,\n"
"              in SamplerState       PrefilteredEnvMap_sampler,\n"
"#   if ENABLE_SHEEN\n"
"              in Texture2D    PreintegratedCharlie,\n"
"              in SamplerState PreintegratedCharlie_sampler,\n"
"#   endif\n"
"              inout SurfaceLightingInfo SrfLighting)\n"
"{\n"
"    SrfLighting.Base.IBL =\n"
"        GetIBLContribution(Shading.BaseLayer.Srf, Shading.BaseLayer.Normal, Shading.View, PrefilteredCubeMipLevels,\n"
"                           PreintegratedGGX,  PreintegratedGGX_sampler,\n"
"                           IrradianceMap,     IrradianceMap_sampler,\n"
"                           PrefilteredEnvMap, PrefilteredEnvMap_sampler);\n"
"#   if ENABLE_SHEEN\n"
"    {\n"
"        SurfaceReflectanceInfo SheenSrf;\n"
"        SheenSrf.PerceptualRoughness = Shading.Sheen.Roughness;\n"
"        SheenSrf.Reflectance0        = Shading.Sheen.Color;\n"
"        SheenSrf.Reflectance90       = float3(0.0, 0.0, 0.0);\n"
"\n"
"        // NOTE: to be accurate, we need to use another environment map here prefiltered with the Charlie BRDF.\n"
"        SrfLighting.Sheen.IBL.f3Specular =\n"
"             GetIBLRadiance(SheenSrf, Shading.BaseLayer.Normal, Shading.View, PrefilteredCubeMipLevels,\n"
"                            PreintegratedCharlie, PreintegratedCharlie_sampler,\n"
"                            PrefilteredEnvMap,    PrefilteredEnvMap_sampler);\n"
"    }\n"
"#   endif\n"
"\n"
"#   if ENABLE_CLEAR_COAT\n"
"    {\n"
"        SrfLighting.Clearcoat.IBL.f3Specular =\n"
"            GetIBLRadiance(Shading.Clearcoat.Srf, Shading.Clearcoat.Normal, Shading.View, PrefilteredCubeMipLevels,\n"
"                           PreintegratedGGX,  PreintegratedGGX_sampler,\n"
"                           PrefilteredEnvMap, PrefilteredEnvMap_sampler);\n"
"    }\n"
"#   endif\n"
"}\n"
"#endif\n"
"\n"
"float3 GetBaseLayerLighting(in SurfaceShadingInfo  Shading,\n"
"                            in SurfaceLightingInfo SrfLighting)\n"
"{\n"
"    float Occlusion = lerp(1.0, Shading.Occlusion, Shading.OcclusionStrength);\n"
"\n"
"    return SrfLighting.Base.Punctual +\n"
"           (SrfLighting.Base.IBL.f3Diffuse + SrfLighting.Base.IBL.f3Specular) * Shading.IBLScale * Occlusion;\n"
"}\n"
"\n"
"#if ENABLE_SHEEN\n"
"float3 GetSheenLighting(in SurfaceShadingInfo  Shading,\n"
"                        in SurfaceLightingInfo SrfLighting)\n"
"{\n"
"    float Occlusion = lerp(1.0, Shading.Occlusion, Shading.OcclusionStrength);\n"
"\n"
"    return SrfLighting.Sheen.Punctual +\n"
"           SrfLighting.Sheen.IBL.f3Specular * Shading.IBLScale * Occlusion;\n"
"}\n"
"#endif\n"
"\n"
"#if ENABLE_CLEAR_COAT\n"
"float3 GetClearcoatLighting(in SurfaceShadingInfo  Shading,\n"
"                            in SurfaceLightingInfo SrfLighting)\n"
"{\n"
"    float Occlusion = lerp(1.0, Shading.Occlusion, Shading.OcclusionStrength);\n"
"\n"
"    return SrfLighting.Clearcoat.Punctual +\n"
"           SrfLighting.Clearcoat.IBL.f3Specular * Shading.IBLScale * Occlusion;\n"
"}\n"
"#endif\n"
"\n"
"float3 ResolveLighting(in SurfaceShadingInfo  Shading,\n"
"                       in SurfaceLightingInfo SrfLighting)\n"
"{\n"
"    float Occlusion = lerp(1.0, Shading.Occlusion, Shading.OcclusionStrength);\n"
"\n"
"    float3 Color =\n"
"        GetBaseLayerLighting(Shading, SrfLighting) +\n"
"        Shading.Emissive * Shading.EmissionScale;\n"
"\n"
"#if ENABLE_SHEEN\n"
"    {\n"
"        Color += GetSheenLighting(Shading, SrfLighting);\n"
"    }\n"
"#endif\n"
"\n"
"#if ENABLE_CLEAR_COAT\n"
"    {\n"
"        // Clear coat layer is applied on top of everything\n"
"\n"
"        float ClearcoatFresnel = SchlickReflection(saturate(dot(Shading.Clearcoat.Normal, Shading.View)), Shading.Clearcoat.Srf.Reflectance0.x, Shading.Clearcoat.Srf.Reflectance90.x);\n"
"        Color =\n"
"            Color * (1.0 - Shading.Clearcoat.Factor * ClearcoatFresnel) +\n"
"            GetClearcoatLighting(Shading, SrfLighting);\n"
"    }\n"
"#endif\n"
"\n"
"    return Color;\n"
"}\n"
"\n"
"float3 GetDebugColor(in SurfaceShadingInfo  Shading,\n"
"                     in SurfaceLightingInfo SrfLighting)\n"
"{\n"
"#if (DEBUG_VIEW == DEBUG_VIEW_OCCLUSION)\n"
"    {\n"
"        return Shading.Occlusion * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_EMISSIVE)\n"
"    {\n"
"        return Shading.Emissive.rgb;\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_METALLIC)\n"
"    {\n"
"        return Shading.BaseLayer.Metallic * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_ROUGHNESS)\n"
"    {\n"
"        return Shading.BaseLayer.Srf.PerceptualRoughness * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_DIFFUSE_COLOR)\n"
"    {\n"
"        return Shading.BaseLayer.Srf.DiffuseColor;\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_SPECULAR_COLOR)\n"
"    {\n"
"        return Shading.BaseLayer.Srf.Reflectance0;\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_REFLECTANCE90)\n"
"    {\n"
"        return Shading.BaseLayer.Srf.Reflectance90;\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_SHADING_NORMAL)\n"
"    {\n"
"        return Shading.BaseLayer.Normal * float3(0.5, 0.5, 0.5) + float3(0.5, 0.5, 0.5);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_NDOTV)\n"
"    {\n"
"        return Shading.BaseLayer.NdotV * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_PUNCTUAL_LIGHTING)\n"
"    {\n"
"        return SrfLighting.Base.Punctual;\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_DIFFUSE_IBL && USE_IBL)\n"
"    {\n"
"        return SrfLighting.Base.IBL.f3Diffuse;\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_SPECULAR_IBL && USE_IBL)\n"
"    {\n"
"        return SrfLighting.Base.IBL.f3Specular;\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_CLEAR_COAT && ENABLE_CLEAR_COAT)\n"
"    {\n"
"        return GetClearcoatLighting(Shading, SrfLighting);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_CLEAR_COAT_FACTOR && ENABLE_CLEAR_COAT)\n"
"    {\n"
"        return Shading.Clearcoat.Factor * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_CLEAR_COAT_ROUGHNESS && ENABLE_CLEAR_COAT)\n"
"    {\n"
"        return Shading.Clearcoat.Srf.PerceptualRoughness * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_CLEAR_COAT_NORMAL && ENABLE_CLEAR_COAT)\n"
"    {\n"
"        return Shading.Clearcoat.Normal * float3(0.5, 0.5, 0.5) + float3(0.5, 0.5, 0.5);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_SHEEN && ENABLE_SHEEN)\n"
"    {\n"
"        return GetSheenLighting(Shading, SrfLighting);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_SHEEN_COLOR && ENABLE_SHEEN)\n"
"    {\n"
"        return Shading.Sheen.Color.rgb;\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_SHEEN_ROUGHNESS && ENABLE_SHEEN)\n"
"    {\n"
"        return Shading.Sheen.Roughness * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_ANISOTROPY && ENABLE_ANISOTROPY)\n"
"    {\n"
"        return Shading.Anisotropy.xyz;\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_IRIDESCENCE && ENABLE_IRIDESCENCE)\n"
"    {\n"
"        return Shading.Iridescence.Factor * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_IRIDESCENCE_THICKNESS && ENABLE_IRIDESCENCE)\n"
"    {\n"
"        return (Shading.Iridescence.Thickness - Shading.Iridescence.ThicknessMinimum) / max(Shading.Iridescence.ThicknessMaximum - Shading.Iridescence.ThicknessMinimum, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_TRANSMISSION && ENABLE_TRANSMISSION)\n"
"    {\n"
"        return Shading.Transmission * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#elif (DEBUG_VIEW == DEBUG_VIEW_THICKNESS && ENABLE_VOLUME)\n"
"    {\n"
"        return Shading.VolumeThickness * float3(1.0, 1.0, 1.0);\n"
"    }\n"
"#endif\n"
"\n"
"    return float3(0.0, 0.0, 0.0);\n"
"}\n"
"\n"
"#endif // _PBR_SHADING_FXH_\n"
