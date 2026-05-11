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

// Get the vertex input from a primitive index.
uint GetIndex(uint primitiveIndex)
{
	uint vertexIndex = 0;
	if (g_meshInfo.m_indexStride == 2)
	{
		const uint2 indexRaw = g_indexBuffer.Load((primitiveIndex * 2 / 4) * 4);
		const uint bitOffset = (primitiveIndex & 0x1) * 16;
		vertexIndex = (indexRaw.x >> bitOffset) & 0xFFFF;
	}
	else // if (g_meshInfo.m_indexStride == 4)
	{
		vertexIndex = g_indexBuffer.Load(primitiveIndex * 4);
	}
	return vertexIndex;
}

// Get the vertex from a vertex index.
VertexShaderInput GetVertex(uint vertexIndex)
{
	VertexShaderInput vertexInput = (VertexShaderInput)0;
	return g_vertexBuffer.Load<VertexShaderInput>(vertexIndex * sizeof(VertexShaderInput));
}

[RootSignature(ROOT_SIGNATURE_COMMON ROOT_SIGNATURE_MS)]
[NumThreads(NUM_THREADS, 1, 1)]
[OutputTopology("triangle")]
void MainMS
(
	uint gtid : SV_GroupThreadID,
	uint gid : SV_GroupID,
	out indices uint3 tris[NUM_VERTEX_COUNT_PER_THREAD_GROUP / 3],
	out vertices PixelShaderInput verts[NUM_VERTEX_COUNT_PER_THREAD_GROUP]
)
{
	const uint threadGroupCount = GetThreadGroupCount(g_meshInfo.m_indexCount);
	const uint instanceID = gid / threadGroupCount;
	
	const uint maxVertexCount = max(0, min(NUM_VERTEX_COUNT_PER_THREAD_GROUP, g_meshInfo.m_indexCount - (gid % threadGroupCount) * NUM_VERTEX_COUNT_PER_THREAD_GROUP));
	const uint maxPrimitiveCount = maxVertexCount / 3;
	
	SetMeshOutputCounts(maxVertexCount, maxPrimitiveCount);
	
	if (gtid < maxVertexCount)
	{
		const uint primitiveIndex = (gid % threadGroupCount) * NUM_VERTEX_COUNT_PER_THREAD_GROUP + gtid;
		const uint vertexIndex = GetIndex(primitiveIndex);
		const VertexShaderInput vertex = GetVertex(vertexIndex);
		verts[gtid].m_position = mul(float4(vertex.m_position * 100 + float3(instanceID * 50.0f, 0, 0), 1.0f), g_sceneData.m_worldViewProjectionMatrix);
		verts[gtid].m_normal = mul(vertex.m_normal, (float3x3) transpose(g_sceneData.m_worldInvMatrix));
		verts[gtid].m_texcoord = vertex.m_texcoord;
	}
	
	if (gtid < maxPrimitiveCount)
	{
		const uint primitiveIndexBegin = 3 * gtid;
		tris[gtid] = uint3(primitiveIndexBegin, primitiveIndexBegin + 1, primitiveIndexBegin + 2);
	}
}
