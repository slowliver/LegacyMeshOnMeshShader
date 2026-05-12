// --------------------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2026 slowliver All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// --------------------------------------------------------------------------------

#ifndef MAIN_HLSLI
#define MAIN_HLSLI

#include "../ShaderCommon.h"

#define ROOT_SIGNATURE_COMMON "  CBV(b0)" \
                              ", SRV(t0)"

ConstantBuffer<SceneInfo> g_sceneInfo : register(b0);
ByteAddressBuffer g_instanceData : register(t0);

struct PixelShaderInput
{
	float4 m_position : SV_Position;
	float3 m_normal : NORMAL0;
	float2 m_texcoord : TEXCOORD0;
	float3 m_color : COLOR0;
};

#if __SHADER_TARGET_STAGE == __SHADER_STAGE_VERTEX
struct VertexShaderInput
{
	float3 m_position : POSITION0;
	float3 m_normal : NORMAL0;
	float2 m_texcoord : TEXCOORD0;
};
#else
struct VertexShaderInput
{
	float3 m_position;
	float3 m_normal;
	float2 m_texcoord;
};
#endif
static_assert(sizeof(VertexShaderInput) % 4 == 0);

#endif // MAIN_HLSLI
