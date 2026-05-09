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

#ifndef COMMON_H
#define COMMON_H

#define NUM_THREADS_X (128)

#if defined(__hlsl_dx_compiler)
#define static_assert(condition) _Static_assert((condition), #condition);
#endif

#if defined(__cplusplus)
_declspec(align(256))
#endif
struct SceneData
{
#if defined(__cplusplus)
	using float4x4 = DirectX::XMFLOAT4X4;
#endif
	float4x4 m_worldMatrix;
	float4x4 m_worldInvMatrix;
	float4x4 m_worldViewProjectionMatrix;
};

struct MeshInfo
{
#if defined(__cplusplus)
	using uint = uint32_t;
#endif
	uint m_vertexCount;
	uint m_indexStride; // 2 (means R16_UINT) or 4 (means R32_UINT).
	uint m_indexCount;
};
static_assert(sizeof(MeshInfo) % 4 == 0);

#endif // COMMON_H
