"#include \"AtmosphereShadersCommon.fxh\"\n"
"\n"
"#ifndef THREAD_GROUP_SIZE\n"
"#   define THREAD_GROUP_SIZE 16\n"
"#endif\n"
"\n"
"cbuffer cbParticipatingMediaScatteringParams\n"
"{\n"
"    AirScatteringAttribs g_MediaParams;\n"
"}\n"
"\n"
"Texture2D<float2> g_tex2DOccludedNetDensityToAtmTop;\n"
"SamplerState g_tex2DOccludedNetDensityToAtmTop_sampler;\n"
"\n"
"#include \"LookUpTables.fxh\"\n"
"#include \"ScatteringIntegrals.fxh\"\n"
"#include \"PrecomputeCommon.fxh\"\n"
"\n"
"RWTexture3D</*format = rgba16f*/float3> g_rwtex3DSingleScattering;\n"
"\n"
"\n"
"// This shader pre-computes the radiance of single scattering at a given point in given\n"
"// direction.\n"
"[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]\n"
"void PrecomputeSingleScatteringCS(uint3 ThreadId  : SV_DispatchThreadID)\n"
"{\n"
"    // Get attributes for the current point\n"
"    float4 f4LUTCoords = LUTCoordsFromThreadID(ThreadId);\n"
"    float fAltitude, fCosViewZenithAngle, fCosSunZenithAngle, fCosSunViewAngle;\n"
"    InsctrLUTCoords2WorldParams(f4LUTCoords,\n"
"                                g_MediaParams.fEarthRadius,\n"
"                                g_MediaParams.fAtmBottomAltitude,\n"
"                                g_MediaParams.fAtmTopAltitude,\n"
"                                fAltitude,\n"
"                                fCosViewZenithAngle,\n"
"                                fCosSunZenithAngle,\n"
"                                fCosSunViewAngle );\n"
"    float3 f3EarthCentre = float3(0.0, -g_MediaParams.fEarthRadius, 0.0);\n"
"    float3 f3RayStart    = float3(0.0, fAltitude, 0.0);\n"
"    float3 f3ViewDir     = ComputeViewDir(fCosViewZenithAngle);\n"
"    float3 f3DirOnLight  = ComputeLightDir(f3ViewDir, fCosSunZenithAngle, fCosSunViewAngle);\n"
"\n"
"    // Intersect view ray with the atmosphere boundaries\n"
"    float4 f4Isecs;\n"
"    GetRaySphereIntersection2( f3RayStart, f3ViewDir, f3EarthCentre,\n"
"                               float2(g_MediaParams.fAtmBottomRadius, g_MediaParams.fAtmTopRadius),\n"
"                               f4Isecs);\n"
"    float2 f2RayEarthIsecs  = f4Isecs.xy;\n"
"    float2 f2RayAtmTopIsecs = f4Isecs.zw;\n"
"\n"
"    if(f2RayAtmTopIsecs.y <= 0.0)\n"
"    {\n"
"        // This is just a sanity check and should never happen\n"
"        // as the start point is always under the top of the\n"
"        // atmosphere (look at InsctrLUTCoords2WorldParams())\n"
"        g_rwtex3DSingleScattering[ThreadId] = float3(0.0, 0.0, 0.0);\n"
"        return;\n"
"    }\n"
"\n"
"    // Set the ray length to the distance to the top of the atmosphere\n"
"    float fRayLength = f2RayAtmTopIsecs.y;\n"
"    // If ray hits Earth, limit the length by the distance to the surface\n"
"    if(f2RayEarthIsecs.x > 0.0)\n"
"        fRayLength = min(fRayLength, f2RayEarthIsecs.x);\n"
"\n"
"    float3 f3RayEnd = f3RayStart + f3ViewDir * fRayLength;\n"
"\n"
"    // Integrate single-scattering\n"
"    float3 f3Extinction, f3Inscattering;\n"
"    IntegrateUnshadowedInscattering(f3RayStart,\n"
"                                    f3RayEnd,\n"
"                                    f3ViewDir,\n"
"                                    f3EarthCentre,\n"
"                                    g_MediaParams.fEarthRadius,\n"
"                                    g_MediaParams.fAtmBottomAltitude,\n"
"                                    g_MediaParams.fAtmAltitudeRangeInv,\n"
"                                    g_MediaParams.f4ParticleScaleHeight,\n"
"                                    f3DirOnLight.xyz,\n"
"                                    100u,\n"
"                                    f3Inscattering,\n"
"                                    f3Extinction);\n"
"\n"
"    g_rwtex3DSingleScattering[ThreadId] = f3Inscattering;\n"
"}\n"
