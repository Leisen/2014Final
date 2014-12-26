//float4x4 就是(4*4)矩陣 裡面值為float
float4x4 mWVP;            //LocalToScreen
float4x4 mWorld;          //Global
float4x4 mWorldInv;       //GlobalInverse 
float4 mainLightPosition;
float4 mainLightColor;   
float4 skyColor; 
float4 groundColor;
float4 camPosition;
float4 amb;              
float4 dif ; // = {1.0f, 1.0f, 1.0f, 1.0f}               
float4 spe;              
float power;            

// 貼圖
texture ColorMap;
sampler2D ColorMapSampler = sampler_state
{
   Texture = <ColorMap>;
   MinFilter = Linear;       // 距離效果
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
};


VS_OUTPUT Basic_ColorVS_2( float4 inPos : POSITION, 
			   float3 inNorm: NORMAL, 
			   float2 inTex : TEXCOORD0 )
{  
   VS_OUTPUT out1 = (VS_OUTPUT) 0;
        
   out1.pos = mul(inPos, mWVP);
   out1.norm = mul(inNorm, (float3x3) mWorld);
   //out1.norm = normalize(mul((float3x3) mWorldInv, inNorm));   
   out1.tex0 = inTex; 

   return out1;
}
   

void Basic_ColorPS(in float2 vTex : TEXCOORD0,
                   in float3 vNorm: TEXCOORD1,
                   out float4 oCol : COLOR0    )
{
   //float3 colorT = tex2D(ColorMapSampler, vTex)*2.5;
   float3 colorT = float3(1.0f, 0.0f, 0.0f);
   oCol.rgb = colorT;//*dif.rgb;
   oCol.a = 0.5f;//dif.a;
}



VS_OUTPUT Basic_ColorVS( float3 inPos : POSITION, 
	                 float3 inNorm: NORMAL, 
			 float2 inTex : TEXCOORD0 )
{  
   VS_OUTPUT out1 = (VS_OUTPUT) 0;

   float3 tmp = inPos.xyz + 0.3f*inNorm.xyz;
   out1.pos = mul(float4(tmp , 1.0f) , mWVP);
   //out1.norm = normalize(mul((float3x3) mWorldInv, inNorm));
   out1.norm = mul(inNorm, (float3x3) mWorld);
   out1.tex0 = inTex; 

   return out1;
}
   

void Basic_ColorPS_2(in float2 vTex : TEXCOORD0,
                     in float3 vNorm: TEXCOORD1,
                     out float4 oCol : COLOR0    )
{
   float3 colorT = tex2D(ColorMapSampler, vTex)*1.5;
   oCol.rgb = colorT;//*dif.rgb;
   oCol.a = 0.5f;
}

// 畫出
technique myOutterGlow
{


   pass P1
   {
      Lighting = TRUE;
      ZEnable = TRUE;
      ZWriteEnable = TRUE;

      AlphaBlendEnable = FALSE;


      //SrcBlend = SrcAlpha;
      //DestBlend = InvSrcAlpha;

      VertexShader = compile vs_3_0 Basic_ColorVS_2();
      PixelShader = compile ps_3_0 Basic_ColorPS_2();      
   }

   pass P0
   {
      Lighting = TRUE;
      ZEnable = TRUE;
      ZWriteEnable = FALSE;

      AlphaBlendEnable = TRUE;
      SrcBlend = SrcAlpha;
      DestBlend = InvSrcAlpha;

      VertexShader = compile vs_3_0 Basic_ColorVS();
      PixelShader = compile ps_3_0 Basic_ColorPS();      
   }


}
