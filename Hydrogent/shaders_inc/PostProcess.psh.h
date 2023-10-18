"struct PSInput\n"
"{\n"
"    float4 Pos : SV_POSITION;\n"
"};\n"
"\n"
"Texture2D g_ColorBuffer;\n"
"Texture2D g_MeshId;\n"
"\n"
"void main(in PSInput PSIn,\n"
"          out float4 Color : SV_Target0)\n"
"{\n"
"    Color = g_ColorBuffer.Load(int3(PSIn.Pos.xy, 0));\n"
"\n"
"    float IsSelected0 = g_MeshId.Load(int3(PSIn.Pos.xy + float2(-1.0, -1.0), 0)).r < 0.0 ? -1.0 : +1.0;\n"
"    float IsSelected1 = g_MeshId.Load(int3(PSIn.Pos.xy + float2( 0.0, -1.0), 0)).r < 0.0 ? -1.0 : +1.0;\n"
"    float IsSelected2 = g_MeshId.Load(int3(PSIn.Pos.xy + float2(+1.0, -1.0), 0)).r < 0.0 ? -1.0 : +1.0;\n"
"\n"
"    float IsSelected3 = g_MeshId.Load(int3(PSIn.Pos.xy + float2(-1.0,  0.0), 0)).r < 0.0 ? -1.0 : +1.0;\n"
"    float IsSelected4 = g_MeshId.Load(int3(PSIn.Pos.xy + float2( 0.0,  0.0), 0)).r < 0.0 ? -1.0 : +1.0;\n"
"    float IsSelected5 = g_MeshId.Load(int3(PSIn.Pos.xy + float2(+1.0,  0.0), 0)).r < 0.0 ? -1.0 : +1.0;\n"
"\n"
"    float IsSelected6 = g_MeshId.Load(int3(PSIn.Pos.xy + float2(-1.0, +1.0), 0)).r < 0.0 ? -1.0 : +1.0;\n"
"    float IsSelected7 = g_MeshId.Load(int3(PSIn.Pos.xy + float2( 0.0, +1.0), 0)).r < 0.0 ? -1.0 : +1.0;\n"
"    float IsSelected8 = g_MeshId.Load(int3(PSIn.Pos.xy + float2(+1.0, +1.0), 0)).r < 0.0 ? -1.0 : +1.0;\n"
"\n"
"    float Outline = IsSelected0 + IsSelected1 + IsSelected2 + IsSelected3 + IsSelected4 + IsSelected5 + IsSelected6 + IsSelected7 + IsSelected8;\n"
"    //Outline = (9.0 - Outline) * (Outline > 0.0 ? 1.0 / 8.0 : 0.0);\n"
"    Outline = (Outline > 0.0 && Outline < 9.0) ? 1.0 : 0.0;\n"
"\n"
"    Color.rgb += Outline * float3(0.25, 0.25, 0.1f);\n"
"\n"
"#if CONVERT_OUTPUT_TO_SRGB\n"
"    Color.rgb = pow(Color.rgb, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));\n"
"#endif\n"
"}\n"
