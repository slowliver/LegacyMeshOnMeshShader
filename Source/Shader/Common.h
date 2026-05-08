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

#if defined(__cplusplus)
#	define LANG_CPP
#	define ALIGNAS(a) alignas(a)
#elif defined(__hlsl_dx_compiler)
#	define LANG_HLSL
#	define ALIGNAS(a)
#endif

#if defined(LANG_HLSL)
namespace DirectX
{
	using XMFLOAT4X4 = float4x4;
}
#endif
