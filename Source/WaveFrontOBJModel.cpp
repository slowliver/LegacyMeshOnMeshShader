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
#include "stdafx.h"
#include "WaveFrontOBJModel.h"

#include "DXSampleHelper.h"

#include <fstream>
#include <unordered_set>

#include <DirectXMesh.h>
#include <WaveFrontReader.h>

using namespace DX;
using namespace DirectX;
using namespace Microsoft::WRL;

using WaveFrontReaderOBJReader = WaveFrontReader<uint32_t>;

HRESULT WaveFrontOBJModel::LoadFromFile(const wchar_t* filename)
{
	auto mesh = std::make_unique<WaveFrontReaderOBJReader>();
	if (auto hr = mesh->Load(filename); FAILED(hr))
	{
		return hr;
	}

	m_vertexCount = mesh->vertices.size();

	auto reader = std::make_unique<VBReader>();

	if (auto hr = reader->Initialize(k_inputLayout); FAILED(hr))
	{
		return hr;
	}

	if (auto hr = reader->AddStream(mesh->vertices.data(), m_vertexCount, 0, sizeof(WaveFrontReaderOBJReader::Vertex)); FAILED(hr))
	{
		return hr;
	}

	// Re-compute normals
	{
		auto pos = std::make_unique<XMFLOAT3[]>(m_vertexCount);
		if (auto hr = reader->Read(pos.get(), "POSITION", 0, m_vertexCount); FAILED(hr))
		{
			return hr;
		}

		auto normals = std::make_unique<XMFLOAT3[]>(m_vertexCount);
		if (auto hr = ComputeNormals(mesh->indices.data(), mesh->indices.size() / 3, pos.get(), m_vertexCount, CNORM_DEFAULT, normals.get()); FAILED(hr))
		{
			return hr;
		}

		auto writer = std::make_unique<VBWriter>();
		if (auto hr = writer->Initialize(k_inputLayout); FAILED(hr))
		{
			return hr;
		}

		if (auto hr = writer->AddStream(mesh->vertices.data(), m_vertexCount, 0, sizeof(WaveFrontReaderOBJReader::Vertex)); FAILED(hr))
		{
			return hr;
		}

		if (auto hr = writer->Write(normals.get(), "NORMAL", 0, m_vertexCount); FAILED(hr))
		{
			return hr;
		}
	}

	// Copy vertex buffer.
	{
		m_vertices = std::make_unique<std::byte[]>(m_vertexCount * sizeof(WaveFrontReaderOBJReader::Vertex));
		memcpy(m_vertices.get(), mesh->vertices.data(), mesh->vertices.size() * sizeof(WaveFrontReaderOBJReader::Vertex));
	}

	// Copy index buffer.
	{
		if (m_vertexCount <= std::numeric_limits<uint16_t>::max())
		{
			m_indices = std::make_unique<std::byte[]>(mesh->indices.size() * sizeof(uint16_t));
			auto* buffer = reinterpret_cast<uint16_t*>(m_indices.get());
			for (uint32_t i = 0; i < mesh->indices.size(); ++i)
			{
				buffer[i] = mesh->indices[i];
			}
			m_indexBufferFormat = DXGI_FORMAT_R16_UINT;
		}
		else
		{
			m_indices = std::make_unique<std::byte[]>(mesh->indices.size() * sizeof(uint32_t));
			memcpy(m_indices.get(), mesh->indices.data(), mesh->indices.size() * sizeof(uint32_t));
			m_indexBufferFormat = DXGI_FORMAT_R32_UINT;
		}
	}
	return S_OK;
}

HRESULT WaveFrontOBJModel::UploadGpuResources(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList)
{
	const auto vertexBufferSize = m_vertexCount * sizeof(WaveFrontReaderOBJReader::Vertex);
	const auto indexBufferSize = m_indexCount * (m_indexBufferFormat == DXGI_FORMAT_R16_UINT ? sizeof(uint16_t) : sizeof(uint32_t));
	auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_gpuResources.m_vertexBuffer)));
	ThrowIfFailed(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &indexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_gpuResources.m_indexBuffer)));

	// Create upload resources
	ComPtr<ID3D12Resource> vertexBufferUpload = nullptr;
	ComPtr<ID3D12Resource> indexBufferUpload = nullptr;

	auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBufferUpload)));
	ThrowIfFailed(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &indexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBufferUpload)));

	// Map & copy memory to upload heap
	{
		void* memory = nullptr;
		ThrowIfFailed(vertexBufferUpload->Map(0, nullptr, &memory));
		memcpy(memory, m_vertices.get(), vertexBufferSize);
		vertexBufferUpload->Unmap(0, nullptr);
	}

	{
		void* memory = nullptr;
		ThrowIfFailed(indexBufferUpload->Map(0, nullptr, &memory));
		std::memcpy(memory, m_indices.get(), indexBufferSize);
		indexBufferUpload->Unmap(0, nullptr);
	}

    // Populate our command list
    cmdList->Reset(cmdAlloc, nullptr);

	cmdList->CopyResource(m_gpuResources.m_vertexBuffer.Get(), vertexBufferUpload.Get());
	cmdList->CopyResource(m_gpuResources.m_indexBuffer.Get(), indexBufferUpload.Get());

	D3D12_RESOURCE_BARRIER barriers[] =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(m_gpuResources.m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
		CD3DX12_RESOURCE_BARRIER::Transition(m_gpuResources.m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
	};
    cmdList->ResourceBarrier(std::extent_v<decltype(barriers)>, barriers);

    ThrowIfFailed(cmdList->Close());

    ID3D12CommandList* commandLists[] = { cmdList };
    cmdQueue->ExecuteCommandLists(1, commandLists);

    // Create our sync fence
    ComPtr<ID3D12Fence> fence = nullptr;
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	ThrowIfFailed(cmdQueue->Signal(fence.Get(), 1));

    // Wait for GPU
    if (fence->GetCompletedValue() != 1)
    {
        HANDLE event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
        fence->SetEventOnCompletion(1, event);
        WaitForSingleObjectEx(event, INFINITE, false);
        CloseHandle(event);
    }

    return S_OK;
}
