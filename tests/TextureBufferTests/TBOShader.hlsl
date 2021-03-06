
// Texture Buffer Object (TBO) Test HLSL Vertex Shader

struct SVertexIn
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	uint Id			: SV_VertexID;
};

struct SVertexOut
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD0;
	//float4 Color	: TEXCOORD1;
};

/*tbuffer BufTex : register(t0)
{
	float4 BufferColors[100];
};*/

#if 0

struct SBlock
{
	float4 Color;
};

StructuredBuffer<SBlock> BufferColors : register(t0);

#else

Buffer<float4> BufferColors : register(t0);

#endif

cbuffer BufferVertex : register(c0)
{
	float4x4 WorldViewProjectionMatrix;
};

SVertexOut VertexMain(SVertexIn In)
{
	SVertexOut Out = (SVertexOut)0;
	
	Out.Position	= mul(WorldViewProjectionMatrix, float4(In.Position, 1.0));
	Out.TexCoord	= In.TexCoord;
	//Out.Color		= BufferColors[In.Id].Color;// + BufferColors2[0];
	
	return Out;
};


// Texture Buffer Object (TBO) Test HLSL Pixel Shader

float4 PixelMain(SVertexOut In) : SV_Target0
{
	float4 Color = (float4)0;
	
	#if 0
	
	//Color = float4(In.TexCoord.x, In.TexCoord.y, 0.0, 1.0);
	//Color = In.Color;
	
	#else
	
	uint2 Id = uint2(
		(int)(In.TexCoord.x * 4.0),
		(int)(In.TexCoord.y * 4.0)
	);
	uint i = Id.y*4 + Id.x;
	Color = BufferColors[i];//.Color;
	
	#endif
	
	return Color;
};




