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

#include "Common.h"

#define ROOT_SIGNATURE_COMMON "CBV(b0)"

struct SceneData
{
	float4x4 m_world;
	float4x4 m_worldInv;
	float4x4 m_worldView;
	float4x4 m_worldViewProjection;
	uint DrawMeshlets;
};

ConstantBuffer<SceneData> g_sceneData : register(b0);

struct VertexShaderOutputToPixelShaderInput
{
	float4 m_position : SV_Position;
	float3 m_normal : NORMAL0;
	float2 m_texcoord : TEXCOORD0;
};
