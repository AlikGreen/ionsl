ConstantBuffer<SceneGlobals> scene : register(b0, space0);

struct SceneGlobals
{
   float3 cameraPosition;
   uint lightCount;
   TonemapMode tonemap;
};

enum TonemapMode
{
   Linear = 0,
   Reinhard = 1,
   ACES = 2,
}

ConstantBuffer<SurfaceMaterial> material : register(b1, space0);

struct SurfaceMaterial
{
   float3 albedo;// = float3(0.8, 0.8, 0.8)
   float roughness;// = 0.4
   float metallic;// = 0
   SurfaceFlags flags;// = SurfaceFlags::ReceivesShadow
};

enum SurfaceFlags
{
   None = 0,
   ReceivesShadow = 1,
   HasAO = 2,
}

StructuredBuffer<LightData> lights : register(t2, space0);

struct LightData
{
   float3 position;
   float3 intensity;
   float radius;
};

struct VertexOutput
{
   float4 position;
   float3 worldPos;
   float3 normal;
};

struct SimpleSpecularBRDF
{
   float3 evaluate(float3 normal, float3 viewDir, float3 lightDir, float roughness)
   {
      float3 halfway = normalize(viewDir + lightDir);
      float nDotH = max(0, dot(normal, halfway));
      float power = max(1, ((1 - roughness) * 64));
      float spec = pow(nDotH, power);
      return float3(spec, spec, spec);
   }

};

float4 fragmentMain(VertexOutput input)
{
   float3 viewDir = normalize(scene.cameraPosition - input.worldPos);
   float3 totalLight = float3(0, 0, 0);
   SimpleSpecularBRDF brdf;
   for(uint i = 0u; i < scene.lightCount; i += 1u)
   {
      LightData light = lights[i];
      float3 toLight = light.position - input.worldPos;
      float distSq = dot(toLight, toLight);
      float radiusSq = light.radius * light.radius;
      if(distSq < radiusSq)
      {
         float3 lightDir = normalize(toLight);
         float atten = 1 - (distSq / radiusSq);
         float3 incoming = light.intensity * atten;
         totalLight += evaluateLighting(brdf, input.normal, viewDir, lightDir, material.roughness, incoming);
      }
   }
   float3 hdrColor = material.albedo * totalLight;
   float3 ldrColor = applyTonemap(hdrColor, scene.tonemap);
   return float4(ldrColor, 1);
}

