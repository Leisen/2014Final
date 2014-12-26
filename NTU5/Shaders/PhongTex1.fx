// This shader implements Phong shading with one color texture
// C.Wang 0927, 2014
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
float colWeight = 1.0;
float difGain = 0.0;
   
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
   
struct VS_OUTPUT
{
   float4 pos  : POSITION;
   float2 tex0 : TEXCOORD0;
   float3 norm : TEXCOORD1;
   float3 lgt  : TEXCOORD2;
   float3 cam  : TEXCOORD3;
};
   

// vertex shader
VS_OUTPUT PhongLight1(float4 inPos  : POSITION,
                      float3 inNorm : NORMAL,
                      float2 inTex  : TEXCOORD0)
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
   out1.tex0 = inTex;

   return out1;
}
   

// pixel shader
void PhongTex1Light1(in float2 vTex : TEXCOORD0,
                     in float3 vNorm : TEXCOORD1,
                     in float3 vLgt  : TEXCOORD2,
                     in float3 vCam  : TEXCOORD3,
                    out float4 oCol  : COLOR0)
{
   float3 normDir = normalize(vNorm);
   float3 lgtDir = normalize(vLgt);
   float3 hDir = normalize(normalize(vCam) + lgtDir);
   float3 skyDir = float3(0.0f, 0.0f, 1.0f);
      
   float diff = dot(normDir, lgtDir)*0.5f + 0.5f;   // half Lambert
   float spec = pow(saturate(dot(hDir, normDir)), shiness);

   float4 colorT = 1.0;
   if (beColormap) {
      colorT = tex2D(colorMapSampler, vTex) + float4(difGain.xxx, 0.0f);
   }

   // the ambient term
   float3 ambient = lerp(groundColor, skyColor, dot(normDir, skyDir)*0.5 + 0.5)*ka;
      
   oCol.rgb = (ambient + diff*mainLightColor*kd)*colorT*colWeight + spec*mainLightColor*ks;
   oCol.a = kd.a*colorT.a;
}


// pixel shader
void PhongTex1NoL(in float2 vTex : TEXCOORD0,
                  in float3 vNorm : TEXCOORD1,
                  in float3 vLgt  : TEXCOORD2,
                  in float3 vCam  : TEXCOORD3,
                 out float4 oCol  : COLOR0)
{
   oCol = kd*colWeight;
   if (beColormap) {
      float4 colorT = tex2D(colorMapSampler, vTex);
      oCol *= colorT;
   }
}


technique PhongTex1
{
   pass P0
   {
      VertexShader = compile vs_3_0 PhongLight1();
      PixelShader = compile ps_3_0 PhongTex1Light1();
   }
}


technique PhongTex1Alpha
{
   pass P0
   {
      AlphaTestEnable = TRUE;	
      AlphaFunc = GREATER;
      AlphaRef = 6;		
      AlphaBlendEnable = TRUE;	
      SrcBlend = SRCALPHA;
      DestBlend = INVSRCALPHA;

      VertexShader = compile vs_3_0 PhongLight1();
      PixelShader = compile ps_3_0 PhongTex1Light1();
   }
}


technique PhongTex1NoLite
{
   pass P0
   {
      VertexShader = compile vs_3_0 PhongLight1();
      PixelShader = compile ps_3_0 PhongTex1NoL();
   }
}