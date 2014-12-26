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
bool beColormap = false;
bool beNormalmap = false;
bool beSpecularmap = false;
bool beLightmap = false;
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


// normal map data   
texture bumpMap;
sampler2D bumpMapSampler = sampler_state
{
   Texture = <bumpMap>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
   AddressU = Wrap;
   AddressV = Wrap;
};


// specular map data   
texture specularMap;
sampler2D specularMapSampler = sampler_state
{
   Texture = <specularMap>;
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
                      float2 inTex1 : TEXCOORD1, 
                      float3 inTangU : TANGENT)
{
   float4 wPos;
   VS_OUTPUT out1 = (VS_OUTPUT) 0;
      
   wPos = mul(inPos, mWorld);
   out1.pos = mul(inPos, mWVP);
   out1.norm = mul(inNorm, (float3x3) mWorld);

   // setup lighting direction
   float3 lgtW;
   if (mainLightType == 1) {
      lgtW = normalize(mainLightPosition.xyz - wPos.xyz);
   }
   else {
      lgtW = -mainLightDirection.xyz;
   }

   // compute the 3x3 transformation matrix for world space to the tangent space
   float3x3 wToTangent;
   wToTangent[0] = mul(inTangU, mWorld);
   wToTangent[1] = mul(cross(inTangU, inNorm), mWorld);
   wToTangent[2] = mul(inNorm, mWorld);

   out1.lgt = normalize(mul(wToTangent, lgtW));
   out1.cam = normalize(mul(wToTangent, camPosition.xyz - wPos.xyz));
   out1.tex0 = inTex0;
   out1.tex1 = inTex1;

   return out1;
}
   

// pixel shader
void PhongTex3Light1(in float2 vTex0 : TEXCOORD0,
                     in float2 vTex1 : TEXCOORD1,
                     in float3 vNorm : TEXCOORD2,
                     in float3 vLgt  : TEXCOORD3,
                     in float3 vCam  : TEXCOORD4,
                    out float4 oCol  : COLOR0)
{
   float3 skyDir = float3(0.0f, 0.0f, 1.0f);
   
   // calculate all vectors
   float3 normDir;
   if (beNormalmap) {
      normDir = normalize(2.0f*tex2D(bumpMapSampler, vTex0) - 1.0f);
   }
   else {
      normDir = skyDir;
   }

   float3 lgtDir = normalize(vLgt);
   float3 hDir = normalize(normalize(vCam) + lgtDir);
      
   float diff = dot(normDir, lgtDir)*0.5f + 0.5f;   // half Lambert
   float spec = pow(saturate(dot(hDir, normDir)), shiness);

   float4 colorT = 1.0;
   if (beColormap) {
      colorT = tex2D(colorMapSampler, vTex0) + float4(difGain.xxx, 0.0f);
   }

   // the ambient term
   float3 ambient = lerp(groundColor, skyColor, dot(normDir, skyDir)*0.5 + 0.5)*ka;
      
   oCol.rgb = (ambient + diff*mainLightColor*kd)*colorT*colWeight + spec*mainLightColor*ks;
   if (beLightmap) {
      float4 colorLM = tex2D(lightMapSampler, vTex1);
      oCol *= colorLM;
   }
   oCol.a = kd.a*colorT.a;
}


technique PhongTex3LM
{
   pass P0
   {
      VertexShader = compile vs_3_0 PhongLight1();
      PixelShader = compile ps_3_0 PhongTex3Light1();
   }
}


technique PhongTex3LMAlpha
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
      PixelShader = compile ps_3_0 PhongTex3Light1();
   }
}
