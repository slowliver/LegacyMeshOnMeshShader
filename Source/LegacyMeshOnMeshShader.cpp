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

#include "stdafx.h"
#include "LegacyMeshOnMeshShader.h"
#include "ShaderCommon.h"

static constexpr const wchar_t* k_meshFilename = L"Bunny.obj";
static constexpr const wchar_t* k_vertexShaderFilename = L"MainVS.cso";
static constexpr const wchar_t* k_meshShaderFilename = L"MainMS.cso";
static constexpr const wchar_t* k_pixelShaderFilename = L"MainPS.cso";

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 619; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

LegacyMeshOnMeshShader::LegacyMeshOnMeshShader(UINT width, UINT height, std::wstring name)
	: DXSample(width, height, name)
	, m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))
	, m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
	, m_rtvDescriptorSize(0)
	, m_dsvDescriptorSize(0)
	, m_cbvDataBegin(nullptr)
	, m_frameIndex(0)
	, m_frameCounter(0)
	, m_fenceEvent{}
	, m_fenceValues{}
{}

void LegacyMeshOnMeshShader::OnInit()
{
	wchar_t modulePath[MAX_PATH];
	if (!::GetModuleFileNameW(nullptr, modulePath, std::extent_v<decltype(modulePath)>))
	{
		throw HrException(E_FAIL);
	}

	std::wstring workingDirectory = std::filesystem::path(modulePath).parent_path().wstring();
	if (!::SetCurrentDirectoryW(workingDirectory.c_str()))
	{
		throw HrException(E_FAIL);
	}

	m_camera.Init({ 0, 75, 150 });
	m_camera.SetMoveSpeed(150.0f);

	LoadPipeline();
	LoadAssets();
}

// Load the rendering pipeline dependencies.
void LegacyMeshOnMeshShader::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
		));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter, true);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
		));
	}

	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_7 };
	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
		|| (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_7))
	{
		OutputDebugStringA("ERROR: Shader Model 6.7 is not supported\n");
		throw std::exception("Shader Model 6.7 is not supported");
	}

	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features)))
			|| (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
		{
			OutputDebugStringA("ERROR: Mesh Shaders aren't supported!\n");
			throw std::exception("Mesh Shaders aren't supported!");
		}
	}

	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS22 features = {};
		if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS22, &features, sizeof(features))))
		{
			m_max1DDispatchMeshSize = std::max<uint32_t>(D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION, features.Max1DDispatchMeshSize);
		}
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		Win32Application::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

		m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV and a command allocator for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
		}
	}

	// Create the depth stencil view.
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		const CD3DX12_HEAP_PROPERTIES depthStencilHeapProps(D3D12_HEAP_TYPE_DEFAULT);
		const CD3DX12_RESOURCE_DESC depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&depthStencilHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilTextureDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&m_depthStencil)
		));

		NAME_D3D12_OBJECT(m_depthStencil);

		m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Create the constant buffer.
	{
		const UINT64 constantBufferSize = sizeof(Shader::SceneInfo) * FrameCount;

		const CD3DX12_HEAP_PROPERTIES constantBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&constantBufferHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&constantBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer)));

		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvDataBegin)));
	}

	{
		const CD3DX12_HEAP_PROPERTIES instanceDataHeapProps(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC instanceDataDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Shader::InstanceData) * k_maxInstanceCount);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&instanceDataHeapProps,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&instanceDataDesc,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&m_instanceData)));
	}
}

// Load the sample assets.
void LegacyMeshOnMeshShader::LoadAssets()
{
	// Create the pipeline state, which includes compiling and loading shaders.
	{
		struct Buffer
		{
			byte* m_data = nullptr;
			uint32_t m_size = 0;
		};
		Buffer vertexShader = {};
		Buffer meshShader = {};
		Buffer  pixelShader = {};

		ThrowIfFailed(ReadDataFromFile(GetAssetFullPath(k_vertexShaderFilename).c_str(), &vertexShader.m_data, &vertexShader.m_size));
		ThrowIfFailed(ReadDataFromFile(GetAssetFullPath(k_meshShaderFilename).c_str(), &meshShader.m_data, &meshShader.m_size));
		ThrowIfFailed(ReadDataFromFile(GetAssetFullPath(k_pixelShaderFilename).c_str(), &pixelShader.m_data, &pixelShader.m_size));

		// Pull root signature from the precompiled mesh shader.
		ThrowIfFailed(m_device->CreateRootSignature(0, vertexShader.m_data, vertexShader.m_size, IID_PPV_ARGS(&m_rootSignatureVSPS)));
		ThrowIfFailed(m_device->CreateRootSignature(0, meshShader.m_data, meshShader.m_size, IID_PPV_ARGS(&m_rootSignatureMSPS)));

		// Legacy VS-PS Pipeline.
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.pRootSignature = m_rootSignatureVSPS.Get();
			psoDesc.VS = { vertexShader.m_data, vertexShader.m_size };
			psoDesc.PS = { pixelShader.m_data, pixelShader.m_size };
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
			psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
			psoDesc.InputLayout = m_model.GetInputLayout();
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = m_renderTargets[0]->GetDesc().Format;
			psoDesc.DSVFormat = m_depthStencil->GetDesc().Format;
			psoDesc.SampleDesc = DefaultSampleDesc();

			auto psoStream = CD3DX12_PIPELINE_STATE_STREAM(psoDesc);

			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
			streamDesc.pPipelineStateSubobjectStream = &psoStream;
			streamDesc.SizeInBytes = sizeof(psoStream);

			ThrowIfFailed(m_device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pipelineStateVSPS)));
		}

		{
			D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.pRootSignature = m_rootSignatureMSPS.Get();
			psoDesc.MS = { meshShader.m_data, meshShader.m_size };
			psoDesc.PS = { pixelShader.m_data, pixelShader.m_size };
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = m_renderTargets[0]->GetDesc().Format;
			psoDesc.DSVFormat = m_depthStencil->GetDesc().Format;
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
			psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.SampleDesc = DefaultSampleDesc();

			auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
			streamDesc.pPipelineStateSubobjectStream = &psoStream;
			streamDesc.SizeInBytes = sizeof(psoStream);

			ThrowIfFailed(m_device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pipelineStateMSPS)));
		}
	}

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(m_commandList->Close());

	ThrowIfFailed(m_model.LoadFromFile(k_meshFilename));
	ThrowIfFailed(m_model.UploadGPUResources(m_device.Get(), m_commandQueue.Get(), m_commandAllocators[m_frameIndex].Get(), m_commandList.Get()));

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValues[m_frameIndex]++;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		WaitForGpu();
	}
}

// Update frame-based values.
void LegacyMeshOnMeshShader::OnUpdate()
{
	m_timer.Tick(NULL);

	if (m_frameCounter++ % 30 == 0)
	{
		// Update window text with FPS value.
		wchar_t fps[64];
		swprintf_s(fps, L"%ufps", m_timer.GetFramesPerSecond());
		SetCustomWindowText(fps);
	}

	m_camera.Update(static_cast<float>(m_timer.GetElapsedSeconds()));

	XMMATRIX world = XMMATRIX(g_XMIdentityR0, g_XMIdentityR1, g_XMIdentityR2, g_XMIdentityR3);
	XMMATRIX worldInv = XMMatrixInverse(nullptr, world);
	XMMATRIX view = m_camera.GetViewMatrix();
	XMMATRIX proj = m_camera.GetProjectionMatrix(XM_PI / 3.0f, m_aspectRatio);

	Shader::SceneInfo sceneInfo ={};
	XMStoreFloat4x4(&sceneInfo.m_worldMatrix, XMMatrixTranspose(world));
	XMStoreFloat4x4(&sceneInfo.m_worldInvMatrix, XMMatrixTranspose(worldInv));
	XMStoreFloat4x4(&sceneInfo.m_worldViewProjectionMatrix, XMMatrixTranspose(world * view * proj));

	memcpy(m_cbvDataBegin + sizeof(sceneInfo) * m_frameIndex, &sceneInfo, sizeof(sceneInfo));

	if (m_instanceCountDirty)
	{
		Shader::InstanceData* instanceDataBegin = nullptr;
		D3D12_RANGE range = { 0, sizeof(Shader::InstanceData) * m_instanceCount };
		ThrowIfFailed(m_instanceData->Map(0, &range, reinterpret_cast<void**>(&instanceDataBegin)));
		memset(instanceDataBegin, 0, sizeof(Shader::InstanceData) * k_maxInstanceCount);
		uint32_t dimX = (uint32_t)std::sqrtf((float)m_instanceCount);
		uint32_t dimY = m_instanceCount / dimX;
		for (uint32_t i = 0; i < m_instanceCount; ++i)
		{
			uint32_t x = i % dimX;
			uint32_t y = i / dimX;
			instanceDataBegin[i].m_position.x = x * 4.0f;
			instanceDataBegin[i].m_position.y = y * 4.0f;
			instanceDataBegin[i].m_position.z = 0.0f;
			instanceDataBegin[i].m_position.w = 0.0f;
		}
		m_instanceData->Unmap(0, &range);
		m_instanceCountDirty = false;
	}
}

// Render the scene.
void LegacyMeshOnMeshShader::OnRender()
{
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	MoveToNextFrame();
}

void LegacyMeshOnMeshShader::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForGpu();

	CloseHandle(m_fenceEvent);
}

void LegacyMeshOnMeshShader::OnKeyDown(UINT8 key)
{
	switch (key)
	{
	case 'M':
		m_useMeshShaderPass = true;
		break;
	case 'V':
		m_useMeshShaderPass = false;
		break;
	case 'Z':
		++m_instanceCount;
		m_instanceCountDirty = true;
		break;
	case 'X':
		--m_instanceCount;
		m_instanceCountDirty = true;
		break;
	}
	m_instanceCount = std::min<uint32_t>(k_maxInstanceCount, std::max<uint32_t>(1, m_instanceCount));
	m_camera.OnKeyDown(key);
}

void LegacyMeshOnMeshShader::OnKeyUp(UINT8 key)
{
	m_camera.OnKeyUp(key);
}

void LegacyMeshOnMeshShader::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

	// Set necessary state.
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	const auto toRenderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &toRenderTargetBarrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (m_useMeshShaderPass)
	{
		RenderMeshShaderPass();
	}
	else
	{
		RenderVertexShaderPass();
	}

	// Indicate that the back buffer will now be used to present.
	const auto toPresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &toPresentBarrier);

	ThrowIfFailed(m_commandList->Close());
}

// Wait for pending GPU work to complete.
void LegacyMeshOnMeshShader::WaitForGpu()
{
	// Schedule a Signal command in the queue.
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

	// Wait until the fence has been processed.
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	m_fenceValues[m_frameIndex]++;
}

// Prepare to render the next frame.
void LegacyMeshOnMeshShader::MoveToNextFrame()
{
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	// Update the frame index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

void LegacyMeshOnMeshShader::RenderMeshShaderPass()
{
	m_commandList->SetPipelineState(m_pipelineStateMSPS.Get());
	m_commandList->SetGraphicsRootSignature(m_rootSignatureMSPS.Get());
	m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + sizeof(Shader::SceneInfo) * m_frameIndex);
	m_commandList->SetGraphicsRootShaderResourceView(1, m_instanceData->GetGPUVirtualAddress());

	Shader::MeshInfo meshInfo =
	{
		m_model.GetIndexStride(),
		m_model.GetIndexCount()
	};
	m_commandList->SetGraphicsRoot32BitConstants(3, sizeof(Shader::MeshInfo) / sizeof(uint32_t), &meshInfo, 0); 

	// Change buffers state for rendering.
	{
		D3D12_RESOURCE_BARRIER barriers[] =
		{
			CD3DX12_RESOURCE_BARRIER::Transition(m_model.GetVertexBuffer().Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_model.GetIndexBuffer().Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		};
		m_commandList->ResourceBarrier(std::extent_v<decltype(barriers)>, barriers);
	}

	m_commandList->SetGraphicsRootShaderResourceView(4, m_model.GetVertexBuffer()->GetGPUVirtualAddress());
	m_commandList->SetGraphicsRootShaderResourceView(5, m_model.GetIndexBuffer()->GetGPUVirtualAddress());

	const uint32_t threadGroupCountPerInstance = Shader::GetThreadGroupCount(m_model.GetIndexCount());
	const uint32_t dispatchableInstanceCountOnce = m_max1DDispatchMeshSize / threadGroupCountPerInstance;
	for (uint32_t i = 0; i < (m_instanceCount + (dispatchableInstanceCountOnce - 1)) / dispatchableInstanceCountOnce; ++i)
	{
		const uint32_t instanceIDOffset = i * dispatchableInstanceCountOnce;
		const uint32_t threadGroupCount = threadGroupCountPerInstance * std::min<uint32_t>(dispatchableInstanceCountOnce, std::max<uint32_t>(0, m_instanceCount - instanceIDOffset));
		m_commandList->SetGraphicsRoot32BitConstant(2, instanceIDOffset, 0); // Shader::InstanceInfo::m_instanceIDOffset
		m_commandList->SetGraphicsRoot32BitConstant(2, i, 1); // Shader::InstanceInfo::m_drawID
		m_commandList->DispatchMesh(threadGroupCount, 1, 1);
	}

#if 0
	D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
	commandSignatureDesc.
	m_device->CreateCommandSignature(
		[in]            const D3D12_COMMAND_SIGNATURE_DESC * pDesc,
		[in, optional]  ID3D12RootSignature * pRootSignature,
		REFIID                             riid,
		[out, optional] void** ppvCommandSignature
	);
	ID3D12CommandSignature
	m_commandList->ExecuteIndirect
#endif
}

void LegacyMeshOnMeshShader::RenderVertexShaderPass()
{
	m_commandList->SetPipelineState(m_pipelineStateVSPS.Get());
	m_commandList->SetGraphicsRootSignature(m_rootSignatureVSPS.Get());
	m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + sizeof(Shader::SceneInfo) * m_frameIndex);
	m_commandList->SetGraphicsRootShaderResourceView(1, m_instanceData->GetGPUVirtualAddress());

	// Change buffers state for rendering.
	{
		D3D12_RESOURCE_BARRIER barriers[] =
		{
			CD3DX12_RESOURCE_BARRIER::Transition(m_model.GetVertexBuffer().Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
			CD3DX12_RESOURCE_BARRIER::Transition(m_model.GetIndexBuffer().Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDEX_BUFFER)
		};
		m_commandList->ResourceBarrier(std::extent_v<decltype(barriers)>, barriers);
	}

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[] = { { m_model.GetVertexBuffer()->GetGPUVirtualAddress(), (UINT)m_model.GetVertexBuffer()->GetDesc().Width, m_model.GetVertexStride() } };
	m_commandList->IASetVertexBuffers(0, std::extent_v<decltype(vertexBufferViews)>, vertexBufferViews);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = { m_model.GetIndexBuffer()->GetGPUVirtualAddress(), (UINT)m_model.GetIndexBuffer()->GetDesc().Width, m_model.GetIndexBufferFormat() };
	m_commandList->IASetIndexBuffer(&indexBufferView);

	m_commandList->DrawIndexedInstanced(m_model.GetIndexCount(), m_instanceCount, 0, 0, 0);
}