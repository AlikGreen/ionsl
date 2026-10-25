ConstantBuffer<CameraData> camera : register(b0, space0);

struct CameraData
{
   float4x4 viewProj;
};

struct VertexOutput
{
   float4 position;
   float3 normal;
   float2 uv;
};

struct VertexInput
{
   float3 position;
   float3 normal;
   float2 uv;
};

VertexOutput vertexMain(VertexInput input)
{
   VertexOutput output;
   output.position = mul(camera.viewProj, float4(input.position, 1));
   output.normal = input.normal;
   output.uv = input.uv;
   return output;
}

float4 fragmentMain(VertexOutput input)
{
   float3 lightDir = normalize(float3(0.3, -1, 0.2));
   float nDotL = max(0, dot(input.normal, lightDir));
   return float4(nDotL, nDotL, nDotL, 1);
}

