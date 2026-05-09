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



#if 0
ConstantBuffer<MeshInfo>  MeshInfo            : register(b1);

StructuredBuffer<Vertex>  Vertices            : register(t0);
StructuredBuffer<Meshlet> Meshlets            : register(t1);
StructuredBuffer<uint>    PrimitiveIndices    : register(t3);
#endif

struct MeshInfo
{
	uint m_vertexCount;
	uint m_indexStride; // 2 (means R16_UINT) or 4 (means R32_UINT).
	uint m_indexCount;
};
ConstantBuffer<MeshInfo> g_meshInfo : register(b1);

ByteAddressBuffer g_vertexBuffer : register(t0);
ByteAddressBuffer g_indexBuffer : register(t1);

struct VertexShaderInput
{
	float3 m_position;
	float3 m_normal;
	float2 m_texcoord;
};
static_assert(sizeof(VertexShaderInput) % 4 == 0);

#if 0
static const uint k_maxVertexCount = NUM_THREADS_X / 2;
static const uint k_maxPrimitiveCount = NUM_THREADS_X;
#else
#define k_maxVertexCount (NUM_THREADS_X)
#define k_maxPrimitiveCount (NUM_THREADS_X)
#endif

[RootSignature(ROOT_SIGNATURE_COMMON ROOT_SIGNATURE_MS)]
[NumThreads(NUM_THREADS_X, 1, 1)]
[OutputTopology("triangle")]
//void MainMS(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, uint dtid : SV_DispatchThreadID, out indices uint3 tris[256], out vertices PixelShaderInput verts[128])
void MainMS(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, uint dtid : SV_DispatchThreadID, out indices uint3 tris[k_maxPrimitiveCount], out vertices PixelShaderInput verts[k_maxVertexCount])
{
	SetMeshOutputCounts(k_maxVertexCount, k_maxPrimitiveCount);

#if 0
    if (gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid);
    }

    if (gtid < m.VertCount)
    {
        uint vertexIndex = GetVertexIndex(m, gtid);
        verts[gtid] = GetVertexAttributes(gid, vertexIndex);
    }
#endif
	 
#if 0
	const uint vertexIndex = dtid;
	if (vertexIndex < g_meshInfo.m_vertexCount)
	{
		PixelShaderInput output = (PixelShaderInput)0;
		const uint location = vertexIndex * sizeof(VertexShaderInput);
		VertexShaderInput vertex = g_vertexBuffer.Load<VertexShaderInput>(location);
		output.m_position = mul(float4(vertex.m_position, 1.0f), g_sceneData.m_worldViewProjection);
		output.m_normal = mul(vertex.m_normal, (float3x3) transpose(g_sceneData.m_worldInv));
		output.m_texcoord = vertex.m_texcoord;
		verts[gtid] = output;
	}
	
	
	const uint primitiveIndex = dtid * 3;
	if (primitiveIndex < g_meshInfo.m_indexCount)
	{
		uint3 index;
		if (g_meshInfo.m_indexStride == 2)
		{
			const uint location = (primitiveIndex * 2 / 4) * 4;
			const uint2 indexRaw = g_indexBuffer.Load2(location);
			index.x = ((primitiveIndex % 2 == 0) ? indexRaw.x : (indexRaw.x >> 16)) & 0xFFFF;
			index.y = ((primitiveIndex % 2 == 0) ? (indexRaw.x >> 16) : indexRaw.y) & 0xFFFF;
			index.z = ((primitiveIndex % 2 == 0) ? indexRaw.y : (indexRaw.y >> 16)) & 0xFFFF;
		}
		else // if (g_meshInfo.m_indexStride == 4)
		{
			const uint location = primitiveIndex * 4;
			index = g_indexBuffer.Load3(location);
		}

		PixelShaderInput output = (PixelShaderInput) 0;
		{
			const uint location = index.x * sizeof(VertexShaderInput);
			VertexShaderInput vertex = g_vertexBuffer.Load<VertexShaderInput>(location);
			output.m_position = mul(float4(vertex.m_position, 1.0f), g_sceneData.m_worldViewProjection);
			output.m_normal = mul(vertex.m_normal, (float3x3) transpose(g_sceneData.m_worldInv));
			output.m_texcoord = vertex.m_texcoord;
		}
		verts[gtid] = output;
	}
#endif
	
#if 0
	const uint primitiveIndex = dtid * 3;
	if (primitiveIndex < g_meshInfo.m_indexCount)
	{
		uint3 index;
		if (g_meshInfo.m_indexStride == 2)
		{
			const uint location = (primitiveIndex * 2 / 4) * 4;
			const uint2 indexRaw = g_indexBuffer.Load2(location);
			index.x = ((primitiveIndex % 2 == 0) ? indexRaw.x : (indexRaw.x >> 16)) & 0xFFFF;
			index.y = ((primitiveIndex % 2 == 0) ? (indexRaw.x >> 16) : indexRaw.y) & 0xFFFF;
			index.z = ((primitiveIndex % 2 == 0) ? indexRaw.y : (indexRaw.y >> 16)) & 0xFFFF;
		}
		else // if (g_meshInfo.m_indexStride == 4)
		{
			const uint location = primitiveIndex * 4;
			index = g_indexBuffer.Load3(location);
		}
	}
#endif

#if 0
	//if (gtid < k_maxVertexCount)
	{
		uint index;
		if (g_meshInfo.m_indexStride == 2)
		{
			const uint location = (dtid * 2 / 4) * 4;
			const uint2 indexRaw = g_indexBuffer.Load(location);
			index = ((dtid % 2 == 0) ? indexRaw.x : (indexRaw.x >> 16)) & 0xFFFF;
		}
		else // if (g_meshInfo.m_indexStride == 4)
		{
			const uint location = dtid * 4;
			index = g_indexBuffer.Load(location);
		}

		PixelShaderInput output = (PixelShaderInput) 0;
		{
			const uint location = index * sizeof(VertexShaderInput);
			VertexShaderInput vertex = g_vertexBuffer.Load < VertexShaderInput > (location);
			output.m_position = mul(float4(vertex.m_position, 1.0f), g_sceneData.m_worldViewProjection);
			output.m_normal = mul(vertex.m_normal, (float3x3) transpose(g_sceneData.m_worldInv));
			output.m_texcoord = vertex.m_texcoord;
		}
		verts[gtid] = output;
	}
	
	//if (gtid < m.VertCount)
	{
		const uint id = gtid * 3;
		tris[gtid] = uint3(id, id + 1, id + 2);
	}
#endif
	
#if 0
	if (gtid < k_maxVertexCount)
	{
		if (dtid < g_meshInfo.m_vertexCount)
		{
			PixelShaderInput output = (PixelShaderInput) 0;
			const uint location = dtid * sizeof(VertexShaderInput);
			VertexShaderInput vertex = g_vertexBuffer.Load < VertexShaderInput > (location);
			output.m_position = mul(float4(vertex.m_position, 1.0f), g_sceneData.m_worldViewProjection);
			output.m_normal = mul(vertex.m_normal, (float3x3) transpose(g_sceneData.m_worldInv));
			output.m_texcoord = vertex.m_texcoord;
			verts[gtid] = output;
		}
	}
	
	if (gtid < k_maxPrimitiveCount)
	{
		const uint primitiveIndex = dtid * 3;
		if (primitiveIndex < g_meshInfo.m_indexCount)
		{
			uint3 index;
			if (g_meshInfo.m_indexStride == 2)
			{
				const uint location = (primitiveIndex * 2 / 4) * 4;
				const uint2 indexRaw = g_indexBuffer.Load2(location);
				index.x = ((primitiveIndex % 2 == 0) ? indexRaw.x : (indexRaw.x >> 16)) & 0xFFFF;
				index.y = ((primitiveIndex % 2 == 0) ? (indexRaw.x >> 16) : indexRaw.y) & 0xFFFF;
				index.z = ((primitiveIndex % 2 == 0) ? indexRaw.y : (indexRaw.y >> 16)) & 0xFFFF;
			}
			else // if (g_meshInfo.m_indexStride == 4)
			{
				const uint location = primitiveIndex * 4;
				index = g_indexBuffer.Load3(location);
			}
			tris[gtid] = index;
		}
	}
#endif
	
	
	if (gtid < k_maxVertexCount)
	{
		uint vertexIndex;
		if (g_meshInfo.m_indexStride == 2)
		{
			const uint location = ((gid * k_maxVertexCount + gtid) * 2 / 4) * 4;
			const uint2 indexRaw = g_indexBuffer.Load(location);
			vertexIndex = (((gid * k_maxVertexCount + gtid) % 2 == 0) ? indexRaw.x : (indexRaw.x >> 16)) & 0xFFFF;
		}
		else // if (g_meshInfo.m_indexStride == 4)
		{
			const uint location = (gid * k_maxVertexCount + gtid) * 4;
			vertexIndex = g_indexBuffer.Load(location);
		}
		
		VertexShaderInput vertexInput = (VertexShaderInput)0;
		{
			const uint location = vertexIndex * sizeof(VertexShaderInput);
			vertexInput = g_vertexBuffer.Load < VertexShaderInput > (location);
		}
		verts[gtid].m_position = mul(float4(vertexInput.m_position, 1.0f), g_sceneData.m_worldViewProjection);
		verts[gtid].m_normal = mul(vertexInput.m_normal, (float3x3) transpose(g_sceneData.m_worldInv));
		verts[gtid].m_texcoord = vertexInput.m_texcoord;
	}
	
	if (gtid < k_maxPrimitiveCount)
	{
		tris[gtid] = uint3(gtid, gtid + 1, gtid + 2);
	}
}
