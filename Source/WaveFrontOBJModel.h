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

#include "Span.h"

#include <DirectXCollision.h>

struct Attribute
{
    enum EType : uint32_t
    {
        Position,
        Normal,
        TexCoord,
        Tangent,
        Bitangent,
        Count
    };

    EType    Type;
    uint32_t Offset;
};

struct LegacyMesh
{
    D3D12_INPUT_ELEMENT_DESC   LayoutElems[Attribute::Count];
    D3D12_INPUT_LAYOUT_DESC    LayoutDesc;

    std::vector<Span<uint8_t>> Vertices;
    std::vector<uint32_t>      VertexStrides;
    uint32_t                   VertexCount;
    DirectX::BoundingSphere    BoundingSphere;

    Span<uint8_t>              Indices;
    uint32_t                   IndexSize;
    uint32_t                   IndexCount;

    // D3D resource references
    std::vector<D3D12_VERTEX_BUFFER_VIEW>  VBViews;
    D3D12_INDEX_BUFFER_VIEW                IBView;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> VertexResources;
    Microsoft::WRL::ComPtr<ID3D12Resource>              IndexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              MeshletResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              UniqueVertexIndexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              PrimitiveIndexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              CullDataResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              MeshInfoResource;

#if 0
    // Calculates the number of instances of the last meshlet which can be packed into a single threadgroup.
    uint32_t GetLastMeshletPackCount(uint32_t subsetIndex, uint32_t maxGroupVerts, uint32_t maxGroupPrims) 
    { 
        if (Meshlets.size() == 0)
            return 0;

        auto& subset = MeshletSubsets[subsetIndex];
        auto& meshlet = Meshlets[subset.Offset + subset.Count - 1];

        return min(maxGroupVerts / meshlet.VertCount, maxGroupPrims / meshlet.PrimCount);
    }

    void GetPrimitive(uint32_t index, uint32_t& i0, uint32_t& i1, uint32_t& i2) const
    {
        auto prim = PrimitiveIndices[index];
        i0 = prim.i0;
        i1 = prim.i1;
        i2 = prim.i2;
    }

    uint32_t GetVertexIndex(uint32_t index) const
    {
        const uint8_t* addr = UniqueVertexIndices.data() + index * IndexSize;
        if (IndexSize == 4)
        {
            return *reinterpret_cast<const uint32_t*>(addr);
        }
        else 
        {
            return *reinterpret_cast<const uint16_t*>(addr);
        }
    }
#endif
};

class WaveFrontOBJModel
{
public:
    HRESULT LoadFromFile(const wchar_t* filename);
    HRESULT UploadGpuResources(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);

private:
	static constexpr D3D12_INPUT_ELEMENT_DESC k_inputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	static constexpr D3D12_INPUT_LAYOUT_DESC k_inputLayout = { k_inputElements, std::extent_v<decltype(k_inputElements)> };

	std::unique_ptr<std::byte[]> m_vertices = nullptr;
	uint32_t m_vertexCount = 0;

	std::unique_ptr<std::byte[]> m_indices = nullptr;
	uint32_t m_indexCount = 0;
	DXGI_FORMAT m_indexBufferFormat = DXGI_FORMAT_R32_UINT;

	struct GPUResources
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer = nullptr;
	} m_gpuResources;

};
