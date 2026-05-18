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

#pragma once

class WaveFrontOBJModel
{
public:
	using WaveFrontReaderOBJReader = DX::WaveFrontReader<uint32_t>;

	HRESULT LoadFromFile(const wchar_t* filename);
	HRESULT UploadGPUResources(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);

	D3D12_INPUT_LAYOUT_DESC GetInputLayout() const { return { k_inputElements, std::extent_v<decltype(k_inputElements)> }; }

	uint32_t GetVertexStride() const { return sizeof(WaveFrontReaderOBJReader::Vertex); }
	uint32_t GetVertexCount() const { return m_vertexCount; }

	uint32_t GetIndexStride() const { return (m_indexBufferFormat == DXGI_FORMAT_R32_UINT) ? 4 : (m_indexBufferFormat == DXGI_FORMAT_R16_UINT) ? 2 : 0; }
	uint32_t GetIndexCount() const { return m_indexCount; }
	DXGI_FORMAT GetIndexBufferFormat() const { return m_indexBufferFormat; }

	Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexBuffer() const { return m_vertexBuffer; }
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIndexBuffer() const { return m_indexBuffer; }

private:
	static constexpr D3D12_INPUT_ELEMENT_DESC k_inputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	std::unique_ptr<std::byte[]> m_vertices = nullptr;
	uint32_t m_vertexCount = 0;

	std::unique_ptr<std::byte[]> m_indices = nullptr;
	uint32_t m_indexCount = 0;
	DXGI_FORMAT m_indexBufferFormat = DXGI_FORMAT_R32_UINT;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer = nullptr;

};
