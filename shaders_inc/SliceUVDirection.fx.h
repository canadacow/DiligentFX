"#include \"BasicStructures.fxh\"\n"
"#include \"AtmosphereShadersCommon.fxh\"\n"
"\n"
"Texture2D<float4> g_tex2DSliceEndPoints;\n"
"\n"
"cbuffer cbLightParams\n"
"{\n"
"    LightAttribs g_LightAttribs;\n"
"}\n"
"\n"
"cbuffer cbCameraAttribs\n"
"{\n"
"    CameraAttribs g_CameraAttribs;\n"
"}\n"
"\n"
"cbuffer cbPostProcessingAttribs\n"
"{\n"
"    EpipolarLightScatteringAttribs g_PPAttribs;\n"
"};\n"
"\n"
"#define f4IncorrectSliceUVDirAndStart float4(-10000, -10000, 0, 0)\n"
"void RenderSliceUVDirInShadowMapTexturePS(in FullScreenTriangleVSOutput VSOut,\n"
"                                          // IMPORTANT: non-system generated pixel shader input\n"
"                                          // arguments must have the exact same name as vertex shader\n"
"                                          // outputs and must go in the same order.\n"
"                                          // Moreover, even if the shader is not using the argument,\n"
"                                          // it still must be declared.\n"
"\n"
"                                          out float4 f4SliceUVDirAndStart : SV_Target )\n"
"{\n"
"    int iSliceInd = int(VSOut.f4PixelPos.x);\n"
"    // Load epipolar slice endpoints\n"
"    float4 f4SliceEndpoints = g_tex2DSliceEndPoints.Load(  int3(iSliceInd,0,0) );\n"
"    // All correct entry points are completely inside the [-1+1/W,1-1/W]x[-1+1/H,1-1/H] area\n"
"    if( !IsValidScreenLocation(f4SliceEndpoints.xy, g_PPAttribs.f4ScreenResolution) )\n"
"    {\n"
"        f4SliceUVDirAndStart = f4IncorrectSliceUVDirAndStart;\n"
"        return;\n"
"    }\n"
"\n"
"    int iCascadeInd = int(VSOut.f4PixelPos.y);\n"
"    matrix mWorldToShadowMapUVDepth = g_LightAttribs.ShadowAttribs.mWorldToShadowMapUVDepth[iCascadeInd];\n"
"\n"
"    // Reconstruct slice exit point position in world space\n"
"    float3 f3SliceExitWS = ProjSpaceXYZToWorldSpace( float3(f4SliceEndpoints.zw, g_LightAttribs.ShadowAttribs.Cascades[iCascadeInd].f4StartEndZ.y), g_CameraAttribs.mProj, g_CameraAttribs.mViewProjInv );\n"
"    // Transform it to the shadow map UV\n"
"    float2 f2SliceExitUV = WorldSpaceToShadowMapUV(f3SliceExitWS, mWorldToShadowMapUVDepth).xy;\n"
"\n"
"    // Compute camera position in shadow map UV space\n"
"    float2 f2SliceOriginUV = WorldSpaceToShadowMapUV(g_CameraAttribs.f4Position.xyz, mWorldToShadowMapUVDepth).xy;\n"
"\n"
"    // Compute slice direction in shadow map UV space\n"
"    float2 f2SliceDir = f2SliceExitUV - f2SliceOriginUV;\n"
"    f2SliceDir /= max(abs(f2SliceDir.x), abs(f2SliceDir.y));\n"
"\n"
"    float4 f4BoundaryMinMaxXYXY = float4(0.0, 0.0, 1.0, 1.0) + float4(0.5, 0.5, -0.5, -0.5)*g_PPAttribs.f2ShadowMapTexelSize.xyxy;\n"
"    if( any( Less( (f2SliceOriginUV.xyxy - f4BoundaryMinMaxXYXY) * float4( 1.0, 1.0, -1.0, -1.0), float4(0.0, 0.0, 0.0, 0.0) ) ) )\n"
"    {\n"
"        // If slice origin in UV coordinates falls beyond [0,1]x[0,1] region, we have\n"
"        // to continue the ray and intersect it with this rectangle\n"
"        //\n"
"        //    f2SliceOriginUV\n"
"        //       *\n"
"        //        \\\n"
"        //         \\  New f2SliceOriginUV\n"
"        //    1   __\\/___\n"
"        //       |       |\n"
"        //       |       |\n"
"        //    0  |_______|\n"
"        //       0       1\n"
"        //\n"
"\n"
"        // First, compute signed distances from the slice origin to all four boundaries\n"
"        bool4 b4IsValidIsecFlag = Greater(abs(f2SliceDir.xyxy), 1e-6 * float4(1.0, 1.0, 1.0, 1.0));\n"
"        float4 f4DistToBoundaries = (f4BoundaryMinMaxXYXY - f2SliceOriginUV.xyxy) / (f2SliceDir.xyxy + BoolToFloat( Not(b4IsValidIsecFlag) ) );\n"
"\n"
"        //We consider only intersections in the direction of the ray\n"
"        b4IsValidIsecFlag = And( b4IsValidIsecFlag, Greater(f4DistToBoundaries, float4(0.0, 0.0, 0.0, 0.0)) );\n"
"        // Compute the second intersection coordinate\n"
"        float4 f4IsecYXYX = f2SliceOriginUV.yxyx + f4DistToBoundaries * f2SliceDir.yxyx;\n"
"\n"
"        // Select only these coordinates that fall onto the boundary\n"
"        b4IsValidIsecFlag = And( b4IsValidIsecFlag, GreaterEqual(f4IsecYXYX, f4BoundaryMinMaxXYXY.yxyx));\n"
"        b4IsValidIsecFlag = And( b4IsValidIsecFlag, LessEqual(f4IsecYXYX, f4BoundaryMinMaxXYXY.wzwz) );\n"
"        // Replace distances to all incorrect boundaries with the large value\n"
"        f4DistToBoundaries = BoolToFloat(b4IsValidIsecFlag) * f4DistToBoundaries +\n"
"                             // It is important to make sure compiler does not use mad here,\n"
"                             // otherwise operations with FLT_MAX will lose all precision\n"
"                             BoolToFloat( Not(b4IsValidIsecFlag) ) * float4(+FLT_MAX, +FLT_MAX, +FLT_MAX, +FLT_MAX);\n"
"        // Select the closest valid intersection\n"
"        float2 f2MinDist = min(f4DistToBoundaries.xy, f4DistToBoundaries.zw);\n"
"        float fMinDist = min(f2MinDist.x, f2MinDist.y);\n"
"\n"
"        // Update origin\n"
"        f2SliceOriginUV = f2SliceOriginUV + fMinDist * f2SliceDir;\n"
"    }\n"
"\n"
"    f2SliceDir *= g_PPAttribs.f2ShadowMapTexelSize;\n"
"\n"
"    f4SliceUVDirAndStart = float4(f2SliceDir, f2SliceOriginUV);\n"
"}\n"
