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

#include "Framework/DXSample.h"
#include "Framework/StepTimer.h"
#include "Framework/SimpleCamera.h"
#include "WaveFrontOBJModel.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

template <class T>
requires std::is_enum_v<T>
constexpr std::underlying_type_t<T> ToUnderlying(T value) noexcept {  return static_cast<std::underlying_type_t<T>>(value); }

class LegacyMeshOnMeshShader : public DXSample
{
public:
	LegacyMeshOnMeshShader(UINT width, UINT height, std::wstring name);

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();
	virtual void OnKeyDown(UINT8 key);
	virtual void OnKeyUp(UINT8 key);

private:
	static const UINT FrameCount = 2;
	static constexpr uint32_t k_maxInstanceCount = 4096;

	enum class PrimitivePipelineMode
	{
		Vertex,
		Mesh,
		Count
	} m_primitivePipelineMode = PrimitivePipelineMode::Mesh;

	// Pipeline objects.
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device2> m_device;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12Resource> m_depthStencil;
	ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignatures[ToUnderlying(PrimitivePipelineMode::Count)];
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineStates[ToUnderlying(PrimitivePipelineMode::Count)];
	ComPtr<ID3D12Resource> m_constantBuffer;
	ComPtr<ID3D12Resource> m_instanceData;
	UINT m_rtvDescriptorSize;
	UINT m_dsvDescriptorSize;

	ComPtr<ID3D12GraphicsCommandList6> m_commandList;
	UINT8* m_cbvDataBegin;

	StepTimer m_timer;
	SimpleCamera m_camera;
	WaveFrontOBJModel m_model;

	uint32_t m_instanceCount = 1024;
	bool m_instanceCountDirty = true;
	uint32_t m_max1DDispatchMeshSize = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;

	// MDI Resources.
	struct ExecuteIndirectCommandMSPS
	{
		uint32_t m_constantArguments;
		D3D12_DISPATCH_ARGUMENTS m_dispatchArguments;
	};
	ComPtr<ID3D12CommandSignature> m_commandSignatureMSPS;
	ComPtr<ID3D12Resource> m_commandBuffersMSPS;

	// Synchronization objects.
	UINT m_frameIndex;
	UINT m_frameCounter;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValues[FrameCount];

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void MoveToNextFrame();
	void WaitForGpu();

	void RenderMeshShaderPass();
	void RenderVertexShaderPass();
};
