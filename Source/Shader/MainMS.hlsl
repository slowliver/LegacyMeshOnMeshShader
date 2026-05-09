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

#define ROOT_SIGNATURE_MS ", RootConstants(b1, num32bitconstants = 3)"     \
                          ", SRV(t0, visibility = SHADER_VISIBILITY_MESH)" \
                          ", SRV(t1, visibility = SHADER_VISIBILITY_MESH)"

ConstantBuffer<MeshInfo> g_meshInfo : register(b1);

ByteAddressBuffer g_vertexBuffer : register(t0);
ByteAddressBuffer g_indexBuffer : register(t1);

static const uint k_vertexCount = NUM_THREADS_X / 3 * 3;
static const uint k_primitiveCount = k_vertexCount / 3;

static_assert(k_vertexCount % 3 == 0 && k_vertexCount > 0);
static_assert(k_primitiveCount % 3 == 0 && k_primitiveCount > 0);

[RootSignature(ROOT_SIGNATURE_COMMON ROOT_SIGNATURE_MS)]
[NumThreads(NUM_THREADS_X, 2, 1)]
[OutputTopology("triangle")]
void MainMS(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out indices uint3 tris[k_primitiveCount], out vertices PixelShaderInput verts[k_vertexCount])
{
	SetMeshOutputCounts(k_vertexCount, k_primitiveCount);
	 
	if (gtid < k_vertexCount)
	{
		const uint primitiveIndex = gid * k_vertexCount + gtid;
		uint vertexIndex;
		if (g_meshInfo.m_indexStride == 2)
		{
			const uint location = (primitiveIndex * 2 / 4) * 4;
			const uint2 indexRaw = g_indexBuffer.Load(location);
			const uint bitOffset = (primitiveIndex & 0x1) * 16;
			vertexIndex = (indexRaw.x >> bitOffset) & 0xFFFF;
		}
		else // if (g_meshInfo.m_indexStride == 4)
		{
			const uint location = primitiveIndex * 4;
			vertexIndex = g_indexBuffer.Load(location);
		}
		
		VertexShaderInput vertexInput = (VertexShaderInput)0;
		{
			const uint location = vertexIndex * sizeof(VertexShaderInput);
			vertexInput = g_vertexBuffer.Load < VertexShaderInput > (location);
		}
		verts[gtid].m_position = mul(float4(vertexInput.m_position, 1.0f), g_sceneData.m_worldViewProjectionMatrix);
		verts[gtid].m_normal = mul(vertexInput.m_normal, (float3x3) transpose(g_sceneData.m_worldInvMatrix));
		verts[gtid].m_texcoord = vertexInput.m_texcoord;
	}
	
	if (gtid < k_primitiveCount)
	{
		const uint primitiveIndexBegin = 3 * gtid;
		tris[gtid] = uint3(primitiveIndexBegin, primitiveIndexBegin + 1, primitiveIndexBegin + 2);
	}
}
