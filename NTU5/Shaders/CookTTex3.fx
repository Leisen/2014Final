// The Cook & Torrance Reflection Model
// C.Wang 0922, 2014
//

float4x4 mWVP;                 // local to screen projection matrix
float4x4 mWorld;               // local to world matrix
float4 mainLightPosition;      // light position
float4 mainLightDirection;     // light direction for parallel light source
int mainLightType = 1;         // lighting type : 1 for point light, 2 for parallel light
float4 mainLightColor;         // light color
float4 skyColor = { 1.0f, 1.0f, 1.0f, 1.0f};           // sky color for ambient
float4 groundColor = { 0.2f, 0.2f, 0.2f, 0.2f};        // ground color for ambient
float4 ka;                     // ambient color of the material
float4 kd;                     // diffuse color of the material
float4 ks;                     // specular color of the material
float4 camPosition;            // camera position
float diffRatio = 0.8f;        // diffuse ratio
float specRatio = 0.2f;        // specular ratio
float roughness = 0.4f;        // roughness (original demo I use 0.2)
float RI = 0.1f;               // Fresnel refraction factor
float colWeight = 1.0f;        // color texture enhancement
bool beColormap = false;
bool beNormalmap = false;
bool beSpecularmap = false;

// color map data
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

  
struct VS_OUTPUT
{
   float4 pos  : POSITION;
   float3 norm : TEXCOORD0;
   float3 lgt  : TEXCOORD1;
   float3 cam  : TEXCOORD2;
   float2 tex0 : TEXCOORD3;
};
   
VS_OUTPUT CookTVS(float4 inPos  : POSITION,
                  float3 inNorm : NORMAL,
                  float2 inTex  : TEXCOORD0,
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
   out1.tex0 = inTex;
   return out1;
}
   
void CookTPS(in float3 vNorm : TEXCOORD0,
             in float3 vLgt  : TEXCOORD1,
             in float3 vCam  : TEXCOORD2,
             in float2 vTex  : TEXCOORD3,
            out float4 oCol  : COLOR0)
{
   // constants
   const float PI = 3.14159f; // PI
   const float dw = 3.0f;
   float3 skyDir = float3(0.0f, 0.0f, 1.0f);
   
   // calculate all vectors
   float3 N;
   if (beNormalmap) {
      N = normalize(2.0f*tex2D(bumpMapSampler, vTex) - 1.0f);
   }
   else {
      N = skyDir;
   }

   float3 L = normalize(vLgt.xyz);
   float3 V = normalize(vCam);

   float3 H = normalize(V + L);
   float NH = saturate(dot(N, H));
   float VH = saturate(dot(V, H));
   float NV = saturate(dot(N, V));
   float NL = dot(N, L);
   float NL2 = NL*0.5f + 0.5f;
   NL = saturate(NL);
   
   // compute the D
   float NH2 = NH*NH;
   float m2 = roughness*roughness;
   float D = (1.0f/(m2*NH2*NH2))*(exp(-((1.0f - NH2)/(m2*NH2))));
   
   // compute the G
   float G = min(1.0f, 2.0f*NH*min(NV, NL)/VH);
   
   // computer the Fresnel term (using Schlick's approximation)
   float F = RI + (1.0f - RI)*pow((1.0f - NV), 5);
   
   // compute the Rs
   float S = dw*(F*D*G)/(PI*NL*NV);

   // color
   float4 diffuseT;
   if (beColormap) {
      diffuseT = tex2D(colorMapSampler, vTex)*colWeight;
      diffuseT.a *= kd.a;
   }
   else {
      diffuseT = kd;
   }

   // the ambient term
   float3 ambient = lerp(groundColor, skyColor, dot(N, skyDir)*0.5 + 0.5);

   // specular
   float4 specularT;
   if (beSpecularmap) {
      specularT = tex2D(specularMapSampler, vTex);
   }
   else {
      specularT = ks;
   }
   
   // result
   oCol.rgb = (ambient + diffRatio*NL*mainLightColor)*diffuseT + specRatio*S*specularT*mainLightColor;
   oCol.a = diffuseT.a;
}


technique CookTColorBumpSpec
{
   pass P0
   {
      VertexShader = compile vs_3_0 CookTVS();
      PixelShader = compile ps_3_0 CookTPS();
   }
}