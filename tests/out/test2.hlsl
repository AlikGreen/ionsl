ConstantBuffer<ComputeParams> params : register(b0, space0);

struct ComputeParams
{
   uint count;
   uint multiplier;
   float threshold;
};

RWStructuredBuffer<uint> outputBuffer : register(u1, space0);

void computeMain(uint3 id : SV_DispatchThreadID)
{
   if(id.x >= params.count)
   {
      return;
   }
   uint accumulator = id.x * params.multiplier;
   for(int i = 0u; i < 16u; i += 1u)
   {
      if((i % 2u) == 0u)
      {
         continue;
      }
      if(i > 10u)
      {
         break;
      }
      accumulator += uint(i);
   }
   uint step = 0u;
   while(step < 4u)
   {
      accumulator = ((accumulator << 1u) ^ 15u);
      step += 1u;
   }
   accumulator &= 65535u;
   accumulator |= 256u;
   outputBuffer[id.x] = accumulator;
}

