ConstantBuffer<MaterialProperties> material : register(b0, space0);

struct MaterialProperties
{
   float roughness;// = 0.5
   float3 baseColor;// = float3(0.8, 0.8, 0.8)
};

ConstantBuffer<LightRig> rig : register(b1, space0);

struct LightRig
{
   uint count;
   PointLight[4] lights;
};

struct PointLight
{
   float3 position;
   float3 intensity;
   float radius;
   float attenuation(float distSq)
   {
      float radiusSq = radius * radius;
      if(distSq >= radiusSq)
      {
         return 0;
      }
      return 1 - (distSq / radiusSq);
   }

};

struct VertexOutput
{
   float4 position;
   float3 worldPos;
   float3 normal;
};

float4 fragmentMain(VertexOutput input)
{
   float3 totalLight = float3(0, 0, 0);
   for(uint i = 0u; i < rig.count; i += 1u)
   {
      PointLight light = rig.lights[i];
      float3 toLight = light.position - input.worldPos;
      float distSq = dot(toLight, toLight);
      float atten = light.attenuation(distSq);
      if(atten > 0)
      {
         float3 lightDir = normalize(toLight);
         float nDotL = max(0, dot(input.normal, lightDir));
         totalLight += light.intensity * (nDotL * atten);
      }
   }
   float3 finalColor = material.baseColor * totalLight;
   return float4(finalColor, 1);
}

