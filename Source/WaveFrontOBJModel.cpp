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
#if 1
	const auto vertexBufferSize = m_vertexCount * sizeof(WaveFrontReaderOBJReader::Vertex);
	const auto indexBufferSize = m_indexCount * (m_indexBufferFormat == DXGI_FORMAT_R16_UINT ? sizeof(uint16_t) : sizeof(uint32_t));
	auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_gpuResources.m_vertexBuffer)));
	ThrowIfFailed(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &indexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_gpuResources.m_indexBuffer)));

	// Create upload resources
	ComPtr<ID3D12Resource> vertexBufferUpload;
	ComPtr<ID3D12Resource> indexBufferUpload;

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

    for (uint32_t j = 0; j < m.Vertices.size(); ++j)
    {
        cmdList->CopyResource(m.VertexResources[j].Get(), vertexUploads[j].Get());
        const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m.VertexResources[j].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrier);
    }

    D3D12_RESOURCE_BARRIER postCopyBarriers[6];

    cmdList->CopyResource(m.IndexResource.Get(), indexUpload.Get());
    postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m.IndexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cmdList->CopyResource(m.MeshletResource.Get(), meshletUpload.Get());
    postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m.MeshletResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cmdList->CopyResource(m.CullDataResource.Get(), cullDataUpload.Get());
    postCopyBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m.CullDataResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cmdList->CopyResource(m.UniqueVertexIndexResource.Get(), uniqueVertexIndexUpload.Get());
    postCopyBarriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m.UniqueVertexIndexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cmdList->CopyResource(m.PrimitiveIndexResource.Get(), primitiveIndexUpload.Get());
    postCopyBarriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(m.PrimitiveIndexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cmdList->CopyResource(m.MeshInfoResource.Get(), meshInfoUpload.Get());
    postCopyBarriers[5] = CD3DX12_RESOURCE_BARRIER::Transition(m.MeshInfoResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    cmdList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);

    ThrowIfFailed(cmdList->Close());

    ID3D12CommandList* ppCommandLists[] = { cmdList };
    cmdQueue->ExecuteCommandLists(1, ppCommandLists);

    // Create our sync fence
    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    cmdQueue->Signal(fence.Get(), 1);

    // Wait for GPU
    if (fence->GetCompletedValue() != 1)
    {
        HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        fence->SetEventOnCompletion(1, event);

        WaitForSingleObjectEx(event, INFINITE, false);
        CloseHandle(event);
    }
#endif
    return S_OK;
}
