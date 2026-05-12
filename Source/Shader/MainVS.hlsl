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

#include "Main.hlsli"

#define ROOT_SIGNATURE_VS ", RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)"

[RootSignature(ROOT_SIGNATURE_COMMON ROOT_SIGNATURE_VS)]
PixelShaderInput MainVS(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
	PixelShaderInput output = (PixelShaderInput)0;
	InstanceData instanceData = g_instanceData.Load<InstanceData>(instanceID * sizeof(InstanceData));
	output.m_position = mul(float4(input.m_position + instanceData.m_position.xyz, 1.0f), g_sceneInfo.m_worldViewProjectionMatrix);
	output.m_normal = mul(input.m_normal, (float3x3)transpose(g_sceneInfo.m_worldInvMatrix));
	output.m_texcoord = input.m_texcoord;
	return output;
}
