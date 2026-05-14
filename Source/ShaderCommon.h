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

#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

#if defined(__cplusplus)
namespace Shader
{
#endif

#if defined(__hlsl_dx_compiler)
#define static_assert(condition) _Static_assert((condition), #condition);
#endif

#define NUM_THREADS (128)
#define NUM_VERTEX_COUNT_PER_THREAD_GROUP ((NUM_THREADS / 3) * 3)
static_assert(NUM_VERTEX_COUNT_PER_THREAD_GROUP % 3 == 0 && NUM_VERTEX_COUNT_PER_THREAD_GROUP / 3 > 0);

#if defined(__cplusplus)
_declspec(align(256))
#endif
struct SceneInfo
{
#if defined(__cplusplus)
	using float4x4 = DirectX::XMFLOAT4X4;
#endif
	float4x4 m_worldMatrix;
	float4x4 m_worldInvMatrix;
	float4x4 m_worldViewProjectionMatrix;
};
static_assert(sizeof(SceneInfo) % 16 == 0);

struct MeshInfo
{
	uint32_t m_indexStride; // 2 (means R16_UINT) or 4 (means R32_UINT).
	uint32_t m_indexCount;
};
static_assert(sizeof(MeshInfo) % 4 == 0);

struct InstanceInfo
{
	uint32_t m_instanceIDOffset;
	uint32_t m_drawID;
};
static_assert(sizeof(InstanceInfo) % 4 == 0);

struct InstanceData
{
#if defined(__cplusplus)
	using float4 = DirectX::XMFLOAT4;
#endif
	float4 m_position;
	float4 m_color;
};
static_assert(sizeof(InstanceData) % 4 == 0);

inline uint32_t GetThreadGroupCount(uint32_t indexCount)
{
	return (uint32_t)((uint64_t)indexCount + (NUM_VERTEX_COUNT_PER_THREAD_GROUP - 1)) / NUM_VERTEX_COUNT_PER_THREAD_GROUP;
}

#if defined(__cplusplus)
} // namespace Shader
#endif

#endif // SHADER_COMMON_H
