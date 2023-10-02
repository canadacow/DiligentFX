"#ifndef _EPIPOLAR_LIGHT_SCATTERING_STRCUTURES_FXH_\n"
"#define _EPIPOLAR_LIGHT_SCATTERING_STRCUTURES_FXH_\n"
"\n"
"#ifdef __cplusplus\n"
"\n"
"#   ifndef BOOL\n"
"#      define BOOL int32_t // Do not use bool, because sizeof(bool)==1 !\n"
"#   endif\n"
"\n"
"#   ifndef TRUE\n"
"#      define TRUE 1\n"
"#   endif\n"
"\n"
"#   ifndef FALSE\n"
"#      define FALSE 0\n"
"#   endif\n"
"\n"
"#   ifndef CHECK_STRUCT_ALIGNMENT\n"
"        // Note that defining empty macros causes GL shader compilation error on Mac, because\n"
"        // it does not allow standalone semicolons outside of main.\n"
"        // On the other hand, adding semicolon at the end of the macro definition causes gcc error.\n"
"#       define CHECK_STRUCT_ALIGNMENT(s) static_assert( sizeof(s) % 16 == 0, \"sizeof(\" #s \") is not multiple of 16\" )\n"
"#   endif\n"
"\n"
"#   ifndef DEFAULT_VALUE\n"
"#       define DEFAULT_VALUE(x) =x\n"
"#   endif\n"
"\n"
"#else\n"
"\n"
"#   ifndef BOOL\n"
"#       define BOOL bool\n"
"#   endif\n"
"\n"
"#   ifndef DEFAULT_VALUE\n"
"#       define DEFAULT_VALUE(x)\n"
"#   endif\n"
"\n"
"#endif\n"
"\n"
"\n"
"// Epipolar light scattering\n"
"#define LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING  0\n"
"// High-quality brute-force ray marching for every pixel without any optimizations\n"
"#define LIGHT_SCTR_TECHNIQUE_BRUTE_FORCE        1\n"
"\n"
"\n"
"// Shadow map cascade processing mode\n"
"\n"
"// Process all shadow map cascades in a single pass\n"
"#define CASCADE_PROCESSING_MODE_SINGLE_PASS     0\n"
"// Process every shadow map cascade in a separate pass\n"
"#define CASCADE_PROCESSING_MODE_MULTI_PASS      1\n"
"// Process every shadow map cascade in a separate pass, but use single instanced draw command\n"
"#define CASCADE_PROCESSING_MODE_MULTI_PASS_INST 2\n"
"\n"
"\n"
"// Epipolar sampling refinement criterion\n"
"\n"
"// Use depth difference to refine epipolar sampling\n"
"#define REFINEMENT_CRITERION_DEPTH_DIFF  0\n"
"// Use inscattering difference to refine epipolar sampling\n"
"#define REFINEMENT_CRITERION_INSCTR_DIFF 1\n"
"\n"
"\n"
"// Extinction evaluation mode used when attenuating background\n"
"\n"
"// Evaluate extinction for each pixel using analytic formula by Eric Bruneton\n"
"#define EXTINCTION_EVAL_MODE_PER_PIXEL 0\n"
"\n"
"// Render extinction in epipolar space and perform bilateral filtering\n"
"// in the same manner as for inscattering\n"
"#define EXTINCTION_EVAL_MODE_EPIPOLAR  1\n"
"\n"
"\n"
"// Single scattering evaluation mode\n"
"\n"
"// No single scattering\n"
"#define SINGLE_SCTR_MODE_NONE        0\n"
"// Use numerical intergarion\n"
"#define SINGLE_SCTR_MODE_INTEGRATION 1\n"
"// Use single scattering look-up-table\n"
"#define SINGLE_SCTR_MODE_LUT         2\n"
"\n"
"// No higher-order scattering\n"
"#define MULTIPLE_SCTR_MODE_NONE       0\n"
"// Use unoccluded (unshadowed) scattering\n"
"#define MULTIPLE_SCTR_MODE_UNOCCLUDED 1\n"
"// Use occluded (shadowed) scattering\n"
"#define MULTIPLE_SCTR_MODE_OCCLUDED   2\n"
"\n"
"#ifndef EARTH_RADIUS\n"
"    // Average Earth radius at sea level\n"
"#   define EARTH_RADIUS 6371000.0\n"
"#endif\n"
"\n"
"struct EpipolarLightScatteringAttribs\n"
"{\n"
"    // Total number of epipolar slices (or lines). For high quality effect,\n"
"    // set this value to (Screen Width + Screen Height)/2\n"
"    uint uiNumEpipolarSlices                DEFAULT_VALUE(512);\n"
"    // Maximum number of samples on a single epipolar line.\n"
"    // For high quality effect, set this value to max(Screen Width, Screen Height)/2.\n"
"    uint uiMaxSamplesInSlice                DEFAULT_VALUE(256);\n"
"    // Initial ray marching sample spacing on an epipolar line.\n"
"    // Additional samples are added at discontinuities.\n"
"    uint uiInitialSampleStepInSlice         DEFAULT_VALUE(16);\n"
"    // Sample density scale near the epipole where inscattering changes rapidly.\n"
"    // Note that sampling near the epipole is very cheap since only a few steps\n"
"    // are required to perform ray marching.\n"
"    uint uiEpipoleSamplingDensityFactor     DEFAULT_VALUE(2);\n"
"\n"
"    // Refinement threshold controls detection of discontinuities. Smaller values\n"
"    // produce more samples and higher quality, but at a higher performance cost.\n"
"    float fRefinementThreshold              DEFAULT_VALUE(0.03f);\n"
"    // Whether to show epipolar sampling.\n"
"    // Do not use bool, because sizeof(bool)==1 and as a result bool variables\n"
"    // will be incorrectly mapped on GPU constant buffer.\n"
"    BOOL bShowSampling                      DEFAULT_VALUE(FALSE);\n"
"    // Whether to correct inscattering at depth discontinuities. Improves quality\n"
"    // for additional cost.\n"
"    BOOL bCorrectScatteringAtDepthBreaks    DEFAULT_VALUE(FALSE);\n"
"    // Whether to display pixels which are classified as depth discontinuities and which\n"
"    // will be corrected. Only has effect when bCorrectScatteringAtDepthBreaks is TRUE.\n"
"    BOOL bShowDepthBreaks                   DEFAULT_VALUE(FALSE);\n"
"\n"
"    // Whether to show lighting only\n"
"    BOOL bShowLightingOnly                  DEFAULT_VALUE(FALSE);\n"
"    // Optimize sample locations to avoid oversampling. This should generally be TRUE.\n"
"    BOOL bOptimizeSampleLocations           DEFAULT_VALUE(TRUE);\n"
"    // Whether to enable light shafts or render unshadowed inscattering.\n"
"    // Setting this to FALSE increases performance, but reduces visual quality.\n"
"    BOOL bEnableLightShafts                 DEFAULT_VALUE(TRUE);\n"
"    // Number of inscattering integral steps taken when computing unshadowed inscattering (default is OK).\n"
"    uint uiInstrIntegralSteps               DEFAULT_VALUE(30);\n"
"\n"
"    // Size of the shadowmap texel (1/width, 1/height)\n"
"    float2 f2ShadowMapTexelSize             DEFAULT_VALUE(float2(0,0));\n"
"    // Maximum number of ray marching samples on a single ray. Typically this value should match the maximum\n"
"    // shadow map cascade resolution. Using lower value will improve performance but may result\n"
"    // in moire patterns. Note that in most cases significantly less samples are actually taken.\n"
"    uint uiMaxSamplesOnTheRay               DEFAULT_VALUE(512);\n"
"    // The number of ray marching samples on a ray when running scattering correction pass.\n"
"    // This value should typically be much lower than the maximum number of samples on a single ray\n"
"    // because 1D min-max optimization is not available during the correction pass.\n"
"    uint uiNumSamplesOnTheRayAtDepthBreak   DEFAULT_VALUE(32);\n"
"\n"
"    // This defines the number of samples at the lowest level of min-max binary tree\n"
"    // and should match the maximum cascade shadow map resolution\n"
"    uint uiMinMaxShadowMapResolution        DEFAULT_VALUE(0);\n"
"    // Number of shadow map cascades\n"
"    int iNumCascades                        DEFAULT_VALUE(0);\n"
"    // First cascade to use for ray marching. Usually first few cascades are small, and ray\n"
"    // marching them is inefficient.\n"
"    int iFirstCascadeToRayMarch             DEFAULT_VALUE(2);\n"
"    // Cap on the maximum shadow map step in texels. Can be increased for higher shadow map\n"
"    // resolutions.\n"
"    float fMaxShadowMapStep                 DEFAULT_VALUE(16.f);\n"
"\n"
"    // Whether to use 1D min/max binary tree optimization. This improves\n"
"    // performance for higher shadow map resolution. Test it.\n"
"    BOOL bUse1DMinMaxTree                   DEFAULT_VALUE(TRUE);\n"
"    // Whether to use 32-bit float or 16-bit UNORM min-max binary tree.\n"
"    BOOL bIs32BitMinMaxMipMap               DEFAULT_VALUE(FALSE);\n"
"    // Technique used to evaluate light scattering.\n"
"    int  iLightSctrTechnique                DEFAULT_VALUE(LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING);\n"
"    // Shadow map cascades processing mode.\n"
"    int  iCascadeProcessingMode             DEFAULT_VALUE(CASCADE_PROCESSING_MODE_SINGLE_PASS);\n"
"\n"
"    // Epipolar sampling refinement criterion.\n"
"    int  iRefinementCriterion               DEFAULT_VALUE(REFINEMENT_CRITERION_INSCTR_DIFF);\n"
"    // Single scattering evaluation mode.\n"
"    int  iSingleScatteringMode              DEFAULT_VALUE(SINGLE_SCTR_MODE_INTEGRATION);\n"
"    // Higher-order scattering evaluation mode.\n"
"    int  iMultipleScatteringMode            DEFAULT_VALUE(MULTIPLE_SCTR_MODE_UNOCCLUDED);\n"
"    // Atmospheric extinction evaluation mode.\n"
"    int  iExtinctionEvalMode                DEFAULT_VALUE(EXTINCTION_EVAL_MODE_EPIPOLAR);\n"
"\n"
"    // Whether to use Ozone approximation (ignored when custom scattering coefficients are used).\n"
"    BOOL bUseOzoneApproximation             DEFAULT_VALUE(TRUE);\n"
"    // Whether to use custom scattering coefficients.\n"
"    BOOL bUseCustomSctrCoeffs               DEFAULT_VALUE(FALSE);\n"
"    // Aerosol density scale to use for scattering coefficient computation.\n"
"    float fAerosolDensityScale              DEFAULT_VALUE(1.f);\n"
"    // Aerosol absorption scale to use for scattering coefficient computation.\n"
"    float fAerosolAbsorbtionScale           DEFAULT_VALUE(0.1f);\n"
"\n"
"    // Custom Rayleigh coefficients.\n"
"    float4 f4CustomRlghBeta                 DEFAULT_VALUE(float4(5.8e-6f, 13.5e-6f, 33.1e-6f, 0.f));\n"
"    // Custom Mie coefficients.\n"
"    float4 f4CustomMieBeta                  DEFAULT_VALUE(float4(2.e-5f, 2.e-5f, 2.e-5f, 0.f));\n"
"    // Custom Ozone absorption coefficient.\n"
"    float4 f4CustomOzoneAbsorption          DEFAULT_VALUE(float4(0.650f, 1.881f, 0.085f, 0.f) * 1e-6f);\n"
"\n"
"    float4 f4EarthCenter                    DEFAULT_VALUE(float4(0.f, -static_cast<float>(EARTH_RADIUS), 0.f, 0.f));\n"
"\n"
"    // ToneMappingStructures.fxh must be included before EpipolarLightScatteringStructures.fxh\n"
"    ToneMappingAttribs  ToneMapping;\n"
"\n"
"    // Members below are automatically set by the effect. User-provided values are ignored.\n"
"    float4 f4ScreenResolution               DEFAULT_VALUE(float4(0,0,0,0));\n"
"    float4 f4LightScreenPos                 DEFAULT_VALUE(float4(0,0,0,0));\n"
"\n"
"    BOOL   bIsLightOnScreen                 DEFAULT_VALUE(FALSE);\n"
"    float  fNumCascades                     DEFAULT_VALUE(0);\n"
"    float  fFirstCascadeToRayMarch          DEFAULT_VALUE(0);\n"
"    int    Padding0                         DEFAULT_VALUE(0);\n"
"};\n"
"#ifdef CHECK_STRUCT_ALIGNMENT\n"
"    CHECK_STRUCT_ALIGNMENT(EpipolarLightScatteringAttribs);\n"
"#endif\n"
"\n"
"struct AirScatteringAttribs\n"
"{\n"
"    // Angular Rayleigh scattering coefficient contains all the terms exepting 1 + cos^2(Theta):\n"
"    // Pi^2 * (n^2-1)^2 / (2*N) * (6+3*Pn)/(6-7*Pn)\n"
"    float4 f4AngularRayleighSctrCoeff;\n"
"    // Total Rayleigh scattering coefficient is the integral of angular scattering coefficient in all directions\n"
"    // and is the following:\n"
"    // 8 * Pi^3 * (n^2-1)^2 / (3*N) * (6+3*Pn)/(6-7*Pn)\n"
"    float4 f4TotalRayleighSctrCoeff;\n"
"    float4 f4RayleighExtinctionCoeff;\n"
"\n"
"    // Note that angular scattering coefficient is essentially a phase function multiplied by the\n"
"    // total scattering coefficient\n"
"    float4 f4AngularMieSctrCoeff;\n"
"    float4 f4TotalMieSctrCoeff;\n"
"    float4 f4MieExtinctionCoeff;\n"
"\n"
"    float4 f4TotalExtinctionCoeff;\n"
"    // Cornette-Shanks phase function (see Nishita et al. 93) normalized to unity has the following form:\n"
"    // F(theta) = 1/(4*PI) * 3*(1-g^2) / (2*(2+g^2)) * (1+cos^2(theta)) / (1 + g^2 - 2g*cos(theta))^(3/2)\n"
"    float4 f4CS_g; // x == 3*(1-g^2) / (2*(2+g^2))\n"
"                   // y == 1 + g^2\n"
"                   // z == -2*g\n"
"\n"
"    // Earth parameters can\'t be changed at run time\n"
"    float fEarthRadius              DEFAULT_VALUE(static_cast<float>(EARTH_RADIUS));\n"
"    float fAtmBottomAltitude        DEFAULT_VALUE(0.f);     // Altitude of the bottom atmosphere boundary (sea level by default)\n"
"    float fAtmTopAltitude           DEFAULT_VALUE(80000.f); // Altitude of the top atmosphere boundary, 80 km by default\n"
"    float fTurbidity                DEFAULT_VALUE(1.02f);\n"
"\n"
"    float fAtmBottomRadius          DEFAULT_VALUE(fEarthRadius + fAtmBottomAltitude);\n"
"    float fAtmTopRadius             DEFAULT_VALUE(fEarthRadius + fAtmTopAltitude);\n"
"    float fAtmAltitudeRangeInv      DEFAULT_VALUE(1.f / (fAtmTopAltitude - fAtmBottomAltitude));\n"
"    float fAerosolPhaseFuncG        DEFAULT_VALUE(0.76f);\n"
"\n"
"    float4 f4ParticleScaleHeight    DEFAULT_VALUE(float4(7994.f, 1200.f, 1.f/7994.f, 1.f/1200.f));\n"
"};\n"
"#ifdef CHECK_STRUCT_ALIGNMENT\n"
"    CHECK_STRUCT_ALIGNMENT(AirScatteringAttribs);\n"
"#endif\n"
"\n"
"// Internal structure used by the effect\n"
"struct MiscDynamicParams\n"
"{\n"
"    float fMaxStepsAlongRay;   // Maximum number of steps during ray tracing\n"
"    float fCascadeInd;\n"
"    float fElapsedTime;\n"
"    float fDummy;\n"
"\n"
"#ifdef __cplusplus\n"
"    uint ui4SrcMinMaxLevelXOffset;\n"
"    uint ui4SrcMinMaxLevelYOffset;\n"
"    uint ui4DstMinMaxLevelXOffset;\n"
"    uint ui4DstMinMaxLevelYOffset;\n"
"#else\n"
"    uint4 ui4SrcDstMinMaxLevelOffset;\n"
"#endif\n"
"};\n"
"#ifdef CHECK_STRUCT_ALIGNMENT\n"
"    CHECK_STRUCT_ALIGNMENT(MiscDynamicParams);\n"
"#endif\n"
"\n"
"#endif //_EPIPOLAR_LIGHT_SCATTERING_STRCUTURES_FXH_\n"
