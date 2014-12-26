// This shader implements Phong shading with one color texture
// C.Wang 0624, 2013
//

float4x4 mWVP;
float4x4 mWorld;
float4 mainLightPosition;
float4 mainLightColor;
float4 mainLightDirection;
int mainLightType = 1;
float4 skyColor = { 1.0f, 1.0f, 1.0f, 1.0f};           // sky color for ambient
float4 groundColor = { 0.2f, 0.2f, 0.2f, 0.2f};        // ground color for ambient
float4 ka;
float4 kd;
float4 ks;
float shiness;
float4 camPosition;
bool beColormap = true;
bool beLightmap = true;
   
texture colorMap;
sampler2D colorMapSampler = sampler_state
{
   Texture = <colorMap>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
   AddressU = Wrap;
   AddressV = Wrap;
};


texture lightMap;
sampler2D lightMapSampler = sampler_state
{
   Texture = <lightMap>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
   AddressU = Wrap;
   AddressV = Wrap;
};
   
struct VS_OUTPUT
{
   float4 pos  : POSITION;
   float2 tex0 : TEXCOORD0;
   float2 tex1 : TEXCOORD1;
   float3 norm : TEXCOORD2;
   float3 lgt  : TEXCOORD3;
   float3 cam  : TEXCOORD4;
};
   

// vertex shader
VS_OUTPUT PhongLight1(float4 inPos  : POSITION,
                      float3 inNorm : NORMAL,
                      float2 inTex0 : TEXCOORD0,
                      float2 inTex1 : TEXCOORD1)
{
   float4 wPos;
   VS_OUTPUT out1 = (VS_OUTPUT) 0;
      
   wPos = mul(inPos, mWorld);
   out1.pos = mul(inPos, mWVP);
   out1.norm = mul(inNorm, (float3x3) mWorld);

   // setup lighting direction
   if (mainLightType == 1) {
      out1.lgt = normalize(mainLightPosition.xyz - wPos.xyz);
   }
   else {
      out1.lgt = -mainLightDirection;
   }
   out1.cam = normalize(camPosition.xyz - wPos.xyz);
   out1.tex0 = inTex0;
   out1.tex1 = inTex1;

   return out1;
}
   

// pixel shader
void PhongTex1Light1(in float2 vTex0 : TEXCOORD0,
                     in float2 vTex1 : TEXCOORD1,
                     in float3 vNorm : TEXCOORD2,
                     in float3 vLgt  : TEXCOORD3,
                     in float3 vCam  : TEXCOORD4,
                    out float4 oCol  : COLOR0)
{
   //float3 normDir = normalize(vNorm);
   //float3 lgtDir = normalize(vLgt);
   //float3 hDir = normalize(normalize(vCam) + lgtDir);
   //float3 skyDir = float3(0.0f, 0.0f, 1.0f);
      
   //float diff = dot(normDir, lgtDir)*0.5f + 0.5f;   // half Lambert
   //float spec = pow(saturate(dot(hDir, normDir)), shiness);

   //float4 colorT = 1.0;
   //if (beColormap) {
   //   colorT = tex2D(colorMapSampler, vTex0);
   //}

   // the ambient term
   //float3 ambient = lerp(groundColor, skyColor, dot(normDir, skyDir)*0.5 + 0.5)*ka;
      
   //oCol.rgb = (ambient + diff*mainLightColor*kd)*colorT*colWeight + spec*mainLightColor*ks;
   //if (beLightmap) {
   //   float4 colorLM = tex2D(lightMapSampler, vTex1);
   //   oCol *= colorLM;
   //}
   //oCol.a = kd.a*colorT.a;
   oCol = float4(0, 1, 0, 1);
}

technique PhongTex1LM
{
   pass P0
   {
      VertexShader = compile vs_3_0 PhongLight1();
      PixelShader = compile ps_3_0 PhongTex1Light1();
   }
}