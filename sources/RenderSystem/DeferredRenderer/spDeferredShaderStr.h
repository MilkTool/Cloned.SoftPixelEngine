"/*\n"
" * Deferred Cg shader file\n"
" * \n"
" * This file is part of the \"SoftPixel Engine\" (Copyright (c) 2008 by Lukas Hermanns)\n"
" * See \"SoftPixelEngine.hpp\" for license information.\n"
" */\n"
"\n"
"/*\n"
"\n"
"Compilation options:\n"
"\n"
"USE_SHADOW_MAPPING  -> Enables shadow mapping.\n"
"\n"
"*/\n"
"\n"
"#define MAX_LIGHTS              35\n"
"#define MAX_EX_LIGHTS           15\n"
"\n"
"#define LIGHT_POINT             0\n"
"#define LIGHT_SPOT              1\n"
"#define LIGHT_DIRECTIONAL       2\n"
"\n"
"#define AMBIENT_LIGHT_FACTOR    0.1 //!< Should be in the range [0.0 .. 1.0].\n"
"\n"
"\n"
"/*\n"
" * ======= Vertex shader: =======\n"
" */\n"
"\n"
"/* === Structures === */\n"
"\n"
"struct SVertexInput\n"
"{\n"
"    float3 Position : POSITION;\n"
"    float2 TexCoord : TEXCOORD0;\n"
"};\n"
"\n"
"struct SVertexOutput\n"
"{\n"
"    float4 Position : POSITION;\n"
"    float2 TexCoord : TEXCOORD0;\n"
"};\n"
"\n"
"\n"
"/* === Uniforms === */\n"
"\n"
"uniform float4x4 WorldViewProjectionMatrix;\n"
"\n"
"\n"
"/* === Functions === */\n"
"\n"
"SVertexOutput VertexMain(SVertexInput In)\n"
"{\n"
"    SVertexOutput Out;\n"
"    \n"
"    /* Process vertex transformation for position and normal */\n"
"    Out.Position = mul(WorldViewProjectionMatrix, float4(In.Position, 1));\n"
"    Out.TexCoord = In.TexCoord;\n"
"    \n"
"    return Out;\n"
"}\n"
"\n"
"\n"
"/*\n"
" * ======= Pixel shader: =======\n"
" */\n"
"\n"
"/* === Structures === */\n"
"\n"
"struct SPixelInput\n"
"{\n"
"    float2 TexCoord : TEXCOORD0;\n"
"};\n"
"\n"
"struct SPixelOutput\n"
"{\n"
"    float4 Color : COLOR0;\n"
"};\n"
"\n"
"struct SLight\n"
"{\n"
"    float4 PositionAndRadius;   //!< Position (xyz), Radius (w).\n"
"    float3 Color;\n"
"    int Type;                   //!< 0 -> Point light, 1 -> Spot light, 2 -> directional light.\n"
"};\n"
"\n"
"struct SLightEx\n"
"{\n"
"    float3 Direction;       //!< Spot-/ directional light direction.\n"
"    float SpotTheta;        //!< First spot cone angle (in radian).\n"
"    float SpotPhiMinusTheta;    //!< Second minus first spot cone angle (in radian).\n"
"    float4x4 Projection;    //!< Spot-/ directional projection matrix.\n"
"};\n"
"\n"
"\n"
"/* === Uniforms === */\n"
"\n"
"uniform sampler2D DiffuseAndSpecularMap : TEXUNIT0;\n"
"uniform sampler2D NormalMap             : TEXUNIT1;\n"
"uniform sampler2D DepthMap              : TEXUNIT2;\n"
"\n"
"#ifdef USE_SHADOW_MAPPING\n"
"uniform sampler2DARRAY DirLightShadowMaps;\n"
"uniform samplerCUBEARRAY PointLightShadowMaps;\n"
"#endif\n"
"\n"
"uniform SLight Lights[MAX_LIGHTS];\n"
"uniform SLightEx LightsEx[MAX_EX_LIGHTS];\n"
"\n"
"uniform float3 ViewPosition;    //!< Global camera position.\n"
"uniform float4x4 ViewTransform; //!< Global camera transformation.\n"
"\n"
"\n"
"/* === Functions === */\n"
"\n"
"inline float GetAngle(float3 a, float3 b)\n"
"{\n"
"    return acos(dot(a, b));\n"
"}\n"
"\n"
"void ComputeLightShading(\n"
"    SLight Light, SLightEx LightEx,\n"
"    float3 Point, float3 Normal, float Shininess,\n"
"    inout vec3 DiffuseColor, inout vec3 SpecularColor)\n"
"{\n"
"    /* Compute light direction vector */\n"
"    float3 LightDir;\n"
"\n"
"    if (Light.Type != LIGHT_DIRECTIONAL)\n"
"        LightDir = normalize(Point - Light.PositionAndRadius.xyz);\n"
"    else\n"
"        LightDir = LightEx.Direction;\n"
"\n"
"    /* Compute phong shading */\n"
"    float NdotL = max(AMBIENT_LIGHT_FACTOR, -dot(Normal, LightDir));\n"
"\n"
"    /* Compute light attenuation */\n"
"    float Distance = distance(Point, Light.PositionAndRadius.xyz);\n"
"\n"
"    float AttnLinear    = Distance / Light.PositionAndRadius.w;\n"
"    float AttnQuadratic = AttnLinear * Distance;\n"
"\n"
"    float Intensity = 1.0 / (1.0 + AttnLinear + AttnQuadratic);\n"
"\n"
"    if (Light.Type == LIGHT_SPOT)\n"
"    {\n"
"        /* Compute spot light cone */\n"
"        float Angle = GetAngle(LightDir, LightEx.Direction);\n"
"        float ConeAngleLerp = (Angle - LightEx.SpotTheta) / LightEx.SpotPhiMinusTheta;\n"
"\n"
"        Intensity *= saturate(1.0 - ConeAngleLerp);\n"
"    }\n"
"\n"
"    /* Compute diffuse color */\n"
"    DiffuseColor += Light.Color * float3(Intensity * NdotL);\n"
"\n"
"    /* Compute specular color */\n"
"    float3 ViewDir      = normalize(Point - ViewPosition);\n"
"    float3 Reflection   = -reflect(LightDir, Normal);\n"
"\n"
"    float NdotHV = dot(ViewDir, Reflection);\n"
"\n"
"    SpecularColor += Light.Color * (Intensity * pow(max(0.0, NdotHV), Shininess));\n"
"}\n"
"\n"
"SPixelOutput PixelMain(SPixelInput In)\n"
"{\n"
"    SPixelOutput Out;\n"
"    \n"
"    /* Get texture colors */\n"
"    float3 Diffuse  = tex2D(DiffuseAndSpecularMap, In.TexCoord).rgb;\n"
"    float3 Normal   = tex2D(NormalMap, In.TexCoord).xyz * float3(2.0) - float3(1.0);\n"
"    float Depth     = tex2D(DepthMap, In.TexCoord).r;\n"
"\n"
"\n"
"    #if 1//DEBUG\n"
"\n"
"    SLight light = (SLight)0;\n"
"    SLightEx lightEx = (SLightEx)0;\n"
"    \n"
"    light.PositionAndRadius = float4(0.0, 0.0, 0.0, 100.0);\n"
"    light.Color = float3(1.0, 1.0, 1.0);\n"
"    light.Type = LIGHT_POINT;\n"
"\n"
"    float3 Point = float3(0.0, 0.0, 5.0);\n"
"\n"
"    float3 DiffuseLight = (float3)0;\n"
"    float3 SpecularLight = (float3)0;\n"
"\n"
"    ComputeLightShading(light, lightEx, Point, Normal, 90.0, DiffuseLight, SpecularLight);\n"
"\n"
"    Out.Color.rgb   = Diffuse * DiffuseLight + SpecularLight;\n"
"    Out.Color.a     = 1.0;\n"
"\n"
"    #endif\n"
"    \n"
"    return Out;\n"
"}\n"
