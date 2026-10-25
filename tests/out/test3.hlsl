ConstantBuffer<PipelineConfig> config : register(b0, space0);

struct PipelineConfig
{
   ShadingModel shading;// = ShadingModel::Lit
   CullMode cull;// = CullMode::Back
   ChannelMask mask;// = ChannelMask::All
};

enum ShadingModel
{
   Unlit = 0,
   Lit = 1,
   Subsurface = 2,
}

enum CullMode
{
   None = 0,
   Front = 1,
   Back = 2,
}

enum ChannelMask
{
   None = 0,
   Red = 1,
   Green = 2,
   Blue = 4,
   Alpha = 8,
   RGB = 7,
   All = 15,
}

struct VertexOutput
{
   float4 position;
   float3 normal;
   float2 uv;
};

float4 fragmentMain(VertexOutput input)
{
   float3 lightDir = normalize(float3(0.5, 1, 0.2));
   float3 lit = shadeSurface(config.shading, float3(0.8, 0.2, 0.2), input.normal, lightDir);
   uint rawCode = uint(config.shading);
   if(rawCode == 0u)
   {
      return float4(lit, 1);
   }
   ChannelMask activeMask = config.mask;
   activeMask &= ~ChannelMask::Alpha;
   activeMask |= ChannelMask::Red;
   return applyChannelMask(float4(lit, 1), activeMask);
}

