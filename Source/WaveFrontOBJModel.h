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
#pragma once

#include <type_traits>
#include <WaveFrontReader.h>

class WaveFrontOBJModel
{
public:
	using WaveFrontReaderOBJReader = DX::WaveFrontReader<uint32_t>;

	HRESULT LoadFromFile(const wchar_t* filename);
	HRESULT UploadGPUResources(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);

	constexpr uint32_t GetVertexStride() const { return sizeof(WaveFrontReaderOBJReader::Vertex); }
	constexpr D3D12_INPUT_LAYOUT_DESC GetInputLayout() const { return { k_inputElements, std::extent_v<decltype(k_inputElements)> }; }

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
