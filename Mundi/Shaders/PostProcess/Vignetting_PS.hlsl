Texture2D g_SceneColorTex : register(t0);

SamplerState g_LinearClampSample : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

cbuffer VignettingCB : register(b2)
{
    float3 VignettingColor;      // Fade 목표 색상 (RGB)
    float Radius;
    
    float Softness;
    float AspectRatio;
    float2 Padding;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.texCoord;

    // uv -> 중심 좌표 변환
    float2 position = uv * 2.0f - 1.0f;
    position.x *= AspectRatio;  // 화면 종횡비 보정
    float dist = length(position);

    // 페이드/감쇠 커브 계산
    float vignette = smoothstep(Radius, Radius - Softness, dist);

    // 원본 색상과의 합성
    float3 sceneColor = g_SceneColorTex.Sample(g_LinearClampSample, input.texCoord).rgb;
    sceneColor *= vignette; // 가장 자리가 어둡게
    return float4(sceneColor, 1.0f);
}
