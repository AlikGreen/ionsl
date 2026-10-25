ConstantBuffer<CameraParams> camera : register(b0, space0);

struct CameraParams
{
   float4x4 viewProj;
   float bias;
};

StructuredBuffer<float3> positions : register(t1, space0);

StructuredBuffer<float> scales : register(t2, space0);

struct VertexOutput
{
   float4 position;
   float4 color;
};

float applyBias(float value, float bias)
{
   return value + bias;
}

VertexOutput vertexMain_f32(uint vertexId : SV_VertexID)
{
   float3 p = positions[vertexId];
   float2 uv = p.xy;
   float scale = applyBias(scales[vertexId], camera.bias);
   float4 clip = mul(camera.viewProj, float4(p.x * scale, p.y * scale, p.z * scale, 1));
   float4 color = float4(uv.x, uv.y, 0, 1);
   return VertexOutput(clip, color);
}

