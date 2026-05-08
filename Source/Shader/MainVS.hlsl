//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "Main.hlsli"

struct VertexShaderInput
{
	float3 m_position : POSITION0;
	float3 m_normal : NORMAL0;
	float2 m_texcoord : TEXCOORD0;
};

[RootSignature(ROOT_SIGNATURE_COMMON ", RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)")]
VertexShaderOutputToPixelShaderInput MainVS(VertexShaderInput input)
{
	VertexShaderOutputToPixelShaderInput output;
	output.m_position = mul(float4(input.m_position, 1.0f), g_sceneData.m_worldViewProjection);
	output.m_normal = mul(input.m_normal, (float3x3)transpose(g_sceneData.m_worldInv));
	output.m_texcoord = input.m_texcoord;
	return output;
}
