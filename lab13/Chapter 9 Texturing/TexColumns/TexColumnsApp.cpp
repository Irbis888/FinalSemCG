//***************************************************************************************
// TexColumnsApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
#include "../../Common/Camera.h"
#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include <filesystem>
#include "FrameResource.h"
#include "RenderingSystem.h"
#include "GBuffer.h"
#include <iostream>



Camera cam;
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;



/*struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
	std::string Name;
};*/


// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.


class TexColumnsApp : public D3DApp
{
public:
	TexColumnsApp(HINSTANCE hInstance);
	TexColumnsApp(const TexColumnsApp& rhs) = delete;
	TexColumnsApp& operator=(const TexColumnsApp& rhs) = delete;
	~TexColumnsApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	virtual void MoveBackFwd(float step)override;
	virtual void MoveLeftRight(float step)override;
	virtual void MoveUpDown(float step)override;
	void OnKeyPressed(const GameTimer& gt, WPARAM key) override;
	void OnKeyReleased(const GameTimer& gt, WPARAM key) override;
	std::wstring GetCamSpeed() override;
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	// Работа с данными


	void LoadAllTextures(); // загрузка из файла
	void LoadTexture(const std::string& name); // и это тоже
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildScreenQuadGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void CreateMaterial(std::string _name, int _CBIndex, int _SRVDiffIndex, int _SRVNMapIndex, XMFLOAT4 _DiffuseAlbedo, XMFLOAT3 _FresnelR0, float _Roughness);
	void BuildMaterials();
	void RenderCustomMesh(std::string unique_name, std::string meshname, std::string materialName, XMMATRIX Scale, XMMATRIX Rotation, XMMATRIX Translation, bool terrain);
	void BuildCustomMeshGeometry(std::string name, UINT& meshVertexOffset, UINT& meshIndexOffset, UINT& prevVertSize, UINT& prevIndSize, std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices, MeshGeometry* Geo);
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, bool height);
	void DrawDebugGBuffer(ID3D12GraphicsCommandList* cmdList);

	void BeginFrame();
	void GeometryPass();
	void GeometryTerrainPass();
	void LightingPass();
	void FinalTransitionAndPresent();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:
	std::unordered_map<std::string, unsigned int>ObjectsMeshCount;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	std::unique_ptr<RenderingSystem> mRenderingSystem;
	//
	std::unordered_map<std::string, int>TexOffsets;
	//
	UINT mCbvSrvDescriptorSize = 0;
	UINT mRtvDesctiptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mGeometryRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mTerrainGeometryRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mLightingRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mDebugRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::unique_ptr<MeshGeometry> mScreenQuadGeo = nullptr;
	GBuffer mGBuffer;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mDebugInputLayout;
	int mHeightMapHeapIndex;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;
	std::vector<RenderItem*> mTerrainTiles;

	PassConstants mMainPassCB;
	DirectX::BoundingFrustum mCamFrustum;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = 0.2f * XM_PI;
	float mRadius = 15.0f;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		TexColumnsApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

TexColumnsApp::TexColumnsApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

TexColumnsApp::~TexColumnsApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}
void TexColumnsApp::MoveBackFwd(float step) {
	XMFLOAT3 newPos;
	XMVECTOR fwd = cam.GetLook();
	XMStoreFloat3(&newPos, cam.GetPosition() + fwd * step);
	cam.SetPosition(newPos);
	cam.UpdateViewMatrix();
}
void TexColumnsApp::MoveLeftRight(float step) {
	XMFLOAT3 newPos;
	XMVECTOR right = cam.GetRight();
	XMStoreFloat3(&newPos, cam.GetPosition() + right * step);
	cam.SetPosition(newPos);
	cam.UpdateViewMatrix();
}
void TexColumnsApp::MoveUpDown(float step) {
	XMFLOAT3 newPos;
	XMVECTOR up = cam.GetUp();
	XMStoreFloat3(&newPos, cam.GetPosition() + up * step);
	cam.SetPosition(newPos);
	cam.UpdateViewMatrix();
}

bool TexColumnsApp::Initialize()
{
	// Создаем консольное окно.
	AllocConsole();

	// Перенаправляем стандартные потоки.
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	cam.SetPosition(0, 3, 10);
	cam.RotateY(MathHelper::Pi);
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


	//mRenderingSystem = std::make_unique<RenderingSystem>(md3dDevice.Get(), mCbvSrvDescriptorSize);
	//mRenderingSystem->Initialize(mCommandList.Get());


	LoadAllTextures();
	BuildRootSignature();
	//mRenderingSystem->BuildRootSignature();
	//mRenderingSystem->BuildDescriptorHeaps(mTextures);
	BuildDescriptorHeaps();
	BuildShapeGeometry(); // свои
	BuildScreenQuadGeometry();
	BuildShadersAndInputLayout();
	//mRenderingSystem->BuildShadersAndInputLayout();
	BuildMaterials();
	//mRenderingSystem->mMaterials = &mMaterials;
	BuildPSOs();
	//mRenderingSystem->BuildPSOs(mBackBufferFormat, mDepthStencilFormat, m4xMsaaState, m4xMsaaQuality);
	BuildRenderItems();
	BuildFrameResources();

	mHeightMapHeapIndex = TexOffsets["textures/terrain_hm"];

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();
	return true;
}

void TexColumnsApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.4f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);

	cam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	BoundingFrustum::CreateFromMatrix(mCamFrustum, cam.GetProj());




}

void TexColumnsApp::Update(const GameTimer& gt)
{
	__m128 headpos;
	headpos.m128_f32[0] = 0;
	headpos.m128_f32[1] = 0;
	headpos.m128_f32[2] = 0;


	XMVECTOR lookDir = XMVectorSubtract(cam.GetPosition(), headpos);
	lookDir = XMVector3Normalize(lookDir);

	// Предположим, что голова по умолчанию смотрит вдаль по оси Z. Тогда можно вычислить угол поворота по оси Y (yaw).
	float yaw = atan2f(XMVectorGetX(lookDir), XMVectorGetZ(lookDir));
	// Если нужно добавить угол наклона (pitch), его можно вычислить аналогично.

	// Создаем матрицу поворота головы. Здесь roll = 0, а pitch можно задать, если требуется.
	XMMATRIX headRotation = XMMatrixRotationRollPitchYaw(0.0f, 3.14 + yaw, 0.0f);

	// Итоговая мировая матрица головы:
	XMMATRIX worldHead = headRotation;




	__m128 leftpos;
	leftpos.m128_f32[0] = 0.73;
	leftpos.m128_f32[1] = 3.9;
	leftpos.m128_f32[2] = 1.1;
	__m128 rightpos;
	rightpos.m128_f32[0] = -0.73;
	rightpos.m128_f32[1] = 3.9;
	rightpos.m128_f32[2] = 1.1;
	XMVECTOR leftDir = XMVector3Normalize(cam.GetPosition() - leftpos);
	XMVECTOR rightDir = XMVector3Normalize(cam.GetPosition() - rightpos);

	// Базовое направление для глаз (они смотрят вдаль по Z)
	XMVECTOR defaultForward = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);

	// Для левого глаза:
	XMVECTOR leftAxis = XMVector3Normalize(XMVector3Cross(defaultForward, leftDir));
	float leftDot = XMVectorGetX(XMVector3Dot(defaultForward, leftDir));
	float leftAngle = acosf(leftDot);
	XMVECTOR leftQuat = XMQuaternionRotationAxis(leftAxis, leftAngle);
	leftQuat = XMQuaternionNormalize(leftQuat);
	XMMATRIX leftRotation = XMMatrixRotationQuaternion(leftQuat);

	// Аналогично для правого глаза:
	XMVECTOR rightAxis = XMVector3Normalize(XMVector3Cross(defaultForward, rightDir));
	float rightDot = XMVectorGetX(XMVector3Dot(defaultForward, rightDir));
	float rightAngle = acosf(rightDot);
	XMVECTOR rightQuat = XMQuaternionRotationAxis(rightAxis, rightAngle);
	rightQuat = XMQuaternionNormalize(rightQuat);
	XMMATRIX rightRotation = XMMatrixRotationQuaternion(rightQuat);





	UpdateCamera(gt);
	for (auto& rItem : mAllRitems)
	{
		if (rItem->Name == "eyeL")
		{

			XMStoreFloat4x4(&rItem->World, XMMatrixScaling(3, 3, 3) * XMMatrixTranslation(0.63, 0.9, -1.1) * XMMatrixTranslation(0, 3, 0) * worldHead);
			rItem->NumFramesDirty = gNumFrameResources;
		}
		if (rItem->Name == "eyeR")
		{
			XMStoreFloat4x4(&rItem->World, XMMatrixScaling(3, 3, 3) * XMMatrixTranslation(-0.63, 0.9, -1.1) * XMMatrixTranslation(0, 3, 0) * worldHead);
			rItem->NumFramesDirty = gNumFrameResources;
		}
		if (rItem->Name == "nigga")
		{
			XMStoreFloat4x4(&rItem->World, XMMatrixScaling(3, 3, 3) * XMMatrixTranslation(0, 3, 0) * worldHead);
			rItem->NumFramesDirty = gNumFrameResources;
		}
		if (rItem->Name == "box")
		{
			XMMATRIX a = XMLoadFloat4x4(&rItem->TexTransform);
			XMStoreFloat4x4(&rItem->TexTransform, a * XMMatrixTranslation(-0.5, -0.5, 0) * XMMatrixRotationRollPitchYaw(0, 0, gt.DeltaTime() * 3) * XMMatrixTranslation(0.5, 0.5, 0));
			rItem->NumFramesDirty = gNumFrameResources;
		}
	}
	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
}

void TexColumnsApp::BuildScreenQuadGeometry()
{
	struct Vertex {
		XMFLOAT3 Pos;
		XMFLOAT2 TexC;
	};

	float w = mClientWidth;
	float h = mClientHeight;

	float quadW = 0.25f * 2.0f; // ширина quad'а (NDC: 0.5 экрана)
	float quadH = 0.25f * 2.0f;

	std::array<Vertex, 6 * 4> vertices;

	auto MakeQuad = [&](int quadIndex, float x, float y) {
		int i = quadIndex * 6;
		vertices[i + 0] = { XMFLOAT3(x, y, 0.0f), XMFLOAT2(0.0f, 1.0f) };
		vertices[i + 1] = { XMFLOAT3(x + quadW, y, 0.0f), XMFLOAT2(1.0f, 1.0f) };
		vertices[i + 2] = { XMFLOAT3(x, y + quadH, 0.0f), XMFLOAT2(0.0f, 0.0f) };
		vertices[i + 3] = vertices[i + 2];
		vertices[i + 4] = vertices[i + 1];
		vertices[i + 5] = { XMFLOAT3(x + quadW, y + quadH, 0.0f), XMFLOAT2(1.0f, 0.0f) };
		};

	// 4 quad-а: нижний левый, правый и верхние
	MakeQuad(0, -1.0f, -1.0f);         // GBuffer[0]
	MakeQuad(1, 0.0f, -1.0f);          // GBuffer[1]
	MakeQuad(2, -1.0f, 0.0f);          // GBuffer[2]
	MakeQuad(3, 0.0f, 0.0f);           // GBuffer[3]

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	mScreenQuadGeo = std::make_unique<MeshGeometry>();
	mScreenQuadGeo->Name = "screenQuadGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mScreenQuadGeo->VertexBufferCPU));
	CopyMemory(mScreenQuadGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	mScreenQuadGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, mScreenQuadGeo->VertexBufferUploader);

	mScreenQuadGeo->VertexByteStride = sizeof(Vertex);
	mScreenQuadGeo->VertexBufferByteSize = vbByteSize;

	SubmeshGeometry quad;
	quad.IndexCount = 6;
	quad.StartIndexLocation = 0;
	quad.BaseVertexLocation = 0;

	for (int i = 0; i < 4; ++i) {
		quad.IndexCount = 6;
		quad.StartIndexLocation = i * 6;
		quad.BaseVertexLocation = 0;
		mScreenQuadGeo->DrawArgs["Quad" + std::to_string(i)] = quad;
	}
}


void TexColumnsApp::DrawDebugGBuffer(ID3D12GraphicsCommandList* cmdList)
{
    cmdList->SetPipelineState(mPSOs["debug"].Get());
    cmdList->SetGraphicsRootSignature(mDebugRootSignature.Get());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    auto vbv = mScreenQuadGeo->VertexBufferView();
    cmdList->IASetVertexBuffers(0, 1, &vbv);

    CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    texHandle.Offset(mGBuffer.SrvHeapStartIndex, mCbvSrvDescriptorSize);

    for (int i = 0; i < 4; ++i)
    {
        cmdList->SetGraphicsRootDescriptorTable(0, texHandle);
        cmdList->DrawInstanced(6, 1, i * 6, 0); // рисуем один quad
        texHandle.Offset(1, mCbvSrvDescriptorSize);
    }
}



void TexColumnsApp::BeginFrame()
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Перезапуск command allocator и command list
	ThrowIfFailed(cmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), nullptr));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
}

void TexColumnsApp::GeometryPass()
{
	mCommandList->SetPipelineState(mPSOs["gbuffer"].Get()); // PSO для geometry pass

	// Переход RTV GBuffer в render target state
	mGBuffer.TransitionToRenderTarget(mCommandList.Get());

	// Установка рендер-таргетов (GBuffer RTV + Depth)
	mCommandList->OMSetRenderTargets(GBuffer::NumTextures, mGBuffer.GetRTVHandles().data(), FALSE, &DepthStencilView());

	// Очистка GBuffer и depth
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mGBuffer.ClearRenderTargets(mCommandList.Get(), Colors::LightSteelBlue);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCommandList->SetGraphicsRootSignature(mGeometryRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());

	// Те же рендер айтемы
	DrawRenderItems(mCommandList.Get(), mOpaqueRitems, false);
}



void TexColumnsApp::GeometryTerrainPass()
{
	mCommandList->SetPipelineState(mPSOs["gbufferTerrain"].Get()); // PSO для geometry pass

	// Переход RTV GBuffer в render target state
	//mGBuffer.TransitionToRenderTarget(mCommandList.Get());

	// Установка рендер-таргетов (GBuffer RTV + Depth)
	//mCommandList->OMSetRenderTargets(GBuffer::NumTextures, mGBuffer.GetRTVHandles().data(), FALSE, &DepthStencilView());

	// Очистка GBuffer и depth
	//const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//mGBuffer.ClearRenderTargets(mCommandList.Get(), Colors::LightSteelBlue);
	//mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	mCommandList->SetGraphicsRootSignature(mTerrainGeometryRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());

	// Те же рендер айтемы
	DrawRenderItems(mCommandList.Get(), mTerrainTiles, true);
}




void TexColumnsApp::LightingPass()
{
	// Переход GBuffer SRV и BackBuffer в RTV
	mGBuffer.TransitionToShaderResource(mCommandList.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), TRUE, nullptr);

	// Очистка backbuffer (по желанию)
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Black, 0, nullptr);

	mCommandList->SetPipelineState(mPSOs["lighting"].Get());
	mCommandList->SetGraphicsRootSignature(mLightingRootSignature.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(1, descriptorHeaps);

	auto srvTableHandle = mGBuffer.GetSRVTable(mSrvDescriptorHeap.Get(), mCbvSrvDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(0, srvTableHandle);

	// Передаём passCB
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	// Рисуем полноэкранный треугольник
	mCommandList->IASetVertexBuffers(0, 0, nullptr);
	mCommandList->IASetIndexBuffer(nullptr);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->DrawInstanced(3, 1, 0, 0);

	//DrawDebugGBuffer(mCommandList.Get());
}

void TexColumnsApp::FinalTransitionAndPresent()
{
	// Переход back buffer в состояние PRESENT
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	// Закрытие командного списка
	ThrowIfFailed(mCommandList->Close());

	// Отправка списка в GPU
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// SwapChain Present
	ThrowIfFailed(mSwapChain->Present(1, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Fence
	mCurrFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}


void TexColumnsApp::Draw(const GameTimer& gt)
{
	BeginFrame();
	GeometryPass();
	//GeometryTerrainPass();
	LightingPass();
	FinalTransitionAndPresent();/**/

	/*auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());

	mCommandList->SetPipelineState(mPSOs["opaque"].Get());
	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);


	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(1, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point.
	// Because we are on the GPU timeline, the new fence point won't be
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);*/

}

void TexColumnsApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void TexColumnsApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void TexColumnsApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.

		cam.YawPitch(dx, -dy);

	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}


void TexColumnsApp::OnKeyPressed(const GameTimer& gt, WPARAM key)
{
	if (GET_WHEEL_DELTA_WPARAM(key) > 0)
	{
		cam.IncreaseSpeed(0.05);
	}
	else if (GET_WHEEL_DELTA_WPARAM(key) < 0)
	{
		cam.IncreaseSpeed(-0.05);
	}
	switch (key)
	{
	case 'A':
		MoveLeftRight(-cam.GetSpeed());
		return;
	case 'W':
		MoveBackFwd(cam.GetSpeed());
		return;
	case 'S':
		MoveBackFwd(-cam.GetSpeed());
		return;
	case 'D':
		MoveLeftRight(cam.GetSpeed());
		return;
	case 'Q':
		MoveUpDown(-cam.GetSpeed());
		return;
	case 'E':
		MoveUpDown(cam.GetSpeed());
		return;
	case VK_SHIFT:
		cam.SpeedUp();
		return;
	}
}

void TexColumnsApp::OnKeyReleased(const GameTimer& gt, WPARAM key)
{

	switch (key)
	{
	case VK_SHIFT:
		cam.SpeedDown();
		return;
	}
}

std::wstring TexColumnsApp::GetCamSpeed()
{
	return std::to_wstring(cam.GetSpeed());
}

void TexColumnsApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMVECTOR campos = cam.GetPosition();
	pos = XMVectorSet(campos.m128_f32[0], campos.m128_f32[1], campos.m128_f32[2], 0.0f);
	target = cam.GetLook();
	up = cam.GetUp();

	XMMATRIX view = XMMatrixLookToLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void TexColumnsApp::AnimateMaterials(const GameTimer& gt)
{

}

void TexColumnsApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{

		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.InvWorld, MathHelper::InverseTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void TexColumnsApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{

		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void TexColumnsApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = {0.4f, 0.4f, 0.5f, 1.0f};
	mMainPassCB.Lights[0].Position = { 10.0f, 15.0f, 20.0f };
	mMainPassCB.Lights[0].Strength = { 0.7, 0.7, 0.7 };
	mMainPassCB.Lights[0].Direction = { 0, -0.5f, -1.f };
	mMainPassCB.Lights[0].FalloffEnd = 100.f;

	mMainPassCB.Lights[1].Position = { 0.0f, 10.0f, 0.0f };
	mMainPassCB.Lights[1].Strength = { 1.2, 0, 0.5 };
	mMainPassCB.Lights[1].FalloffEnd = 40.f;

	mMainPassCB.Lights[2].Position = { 0.0f, 5.0f, -9.0f };
	mMainPassCB.Lights[2].Strength = { 0, 0.9, 0 };
	mMainPassCB.Lights[2].FalloffEnd = 40.f;


	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void TexColumnsApp::LoadAllTextures()
{
	// MEGA COSTYL
	for (const auto& entry : std::filesystem::directory_iterator("../../Textures/textures"))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".dds")
		{
			std::string filepath = entry.path().string();
			filepath = filepath.substr(24, filepath.size());
			filepath = filepath.substr(0, filepath.size() - 4);
			filepath = "textures/" + filepath;
			LoadTexture(filepath);
		}
	}
}

void TexColumnsApp::LoadTexture(const std::string& name)
{
	auto tex = std::make_unique<Texture>();
	tex->Name = name;
	tex->Filename = L"../../Textures/" + std::wstring(name.begin(), name.end()) + L".dds";

	if (FAILED(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), tex->Filename.c_str(),
		tex->Resource, tex->UploadHeap))) std::cout << name << "\n";
	mTextures[name] = std::move(tex);
}

void TexColumnsApp::BuildRootSignature()
{
	// ==== СТАРАЯ СИГНАТУРА (mRootSignature) ====
	CD3DX12_DESCRIPTOR_RANGE diffuseRange;
	diffuseRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

	CD3DX12_DESCRIPTOR_RANGE normalRange;
	normalRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1

	CD3DX12_ROOT_PARAMETER oldRootParams[5];
	oldRootParams[0].InitAsDescriptorTable(1, &diffuseRange, D3D12_SHADER_VISIBILITY_ALL);
	oldRootParams[1].InitAsDescriptorTable(1, &normalRange, D3D12_SHADER_VISIBILITY_ALL);
	oldRootParams[2].InitAsConstantBufferView(0); // b0
	oldRootParams[3].InitAsConstantBufferView(1); // b1
	oldRootParams[4].InitAsConstantBufferView(2); // b2

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC oldRootSigDesc(
		_countof(oldRootParams), oldRootParams,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(&oldRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob) ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));

	// ==== GEOMETRY ROOT SIGNATURE ====

	CD3DX12_DESCRIPTOR_RANGE geoTexTable[2];
	geoTexTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
	geoTexTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1

	CD3DX12_ROOT_PARAMETER geoParams[5];
	geoParams[0].InitAsDescriptorTable(1, &geoTexTable[0], D3D12_SHADER_VISIBILITY_ALL);
	geoParams[1].InitAsDescriptorTable(1, &geoTexTable[1], D3D12_SHADER_VISIBILITY_ALL);
	geoParams[2].InitAsConstantBufferView(0); // b0
	geoParams[3].InitAsConstantBufferView(1); // b1
	geoParams[4].InitAsConstantBufferView(2); // b2

	CD3DX12_ROOT_SIGNATURE_DESC geoRootSigDesc(
		_countof(geoParams), geoParams,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedGeoRootSig = nullptr;
	errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(&geoRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedGeoRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
	if (errorBlob) ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedGeoRootSig->GetBufferPointer(),
		serializedGeoRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mGeometryRootSignature)));

	// ==== TERRAIN GEOMETRY ROOT SIGNATURE ====

	CD3DX12_DESCRIPTOR_RANGE tgeoTexTable[3];
	tgeoTexTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
	tgeoTexTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1
	tgeoTexTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // t2

	CD3DX12_ROOT_PARAMETER tgeoParams[6];
	tgeoParams[0].InitAsDescriptorTable(1, &tgeoTexTable[0], D3D12_SHADER_VISIBILITY_ALL);
	tgeoParams[1].InitAsDescriptorTable(1, &tgeoTexTable[1], D3D12_SHADER_VISIBILITY_ALL);
	tgeoParams[2].InitAsDescriptorTable(1, &tgeoTexTable[2], D3D12_SHADER_VISIBILITY_ALL);
	tgeoParams[3].InitAsConstantBufferView(0); // b0
	tgeoParams[4].InitAsConstantBufferView(1); // b1
	tgeoParams[5].InitAsConstantBufferView(2); // b2

	CD3DX12_ROOT_SIGNATURE_DESC tgeoRootSigDesc(
		_countof(tgeoParams), tgeoParams,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedtGeoRootSig = nullptr;
	errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(&tgeoRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedtGeoRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
	if (errorBlob) ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedtGeoRootSig->GetBufferPointer(),
		serializedtGeoRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mTerrainGeometryRootSignature)));

	// ==== LIGHTING ROOT SIGNATURE ====

	CD3DX12_DESCRIPTOR_RANGE gbufferRange;
	gbufferRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // t0-3

	CD3DX12_ROOT_PARAMETER lightingParams[2];
	lightingParams[0].InitAsDescriptorTable(1, &gbufferRange, D3D12_SHADER_VISIBILITY_ALL);
	lightingParams[1].InitAsConstantBufferView(0); // b0 (свет)

	CD3DX12_ROOT_SIGNATURE_DESC lightingRootSigDesc(
		_countof(lightingParams), lightingParams,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> serializedLightingRootSig = nullptr;
	errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(&lightingRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedLightingRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
	if (errorBlob) ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedLightingRootSig->GetBufferPointer(),
		serializedLightingRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mLightingRootSignature)));

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

	CD3DX12_ROOT_PARAMETER slotRootParameter[1];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		1, slotRootParameter,
		1, &staticSampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	serializedRootSig = nullptr;
	errorBlob = nullptr;

	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig,
		&errorBlob
	));

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mDebugRootSignature)
	));
}
void TexColumnsApp::CreateMaterial(std::string _name, int _CBIndex, int _SRVDiffIndex, int _SRVNMapIndex, XMFLOAT4 _DiffuseAlbedo, XMFLOAT3 _FresnelR0, float _Roughness)
{

	auto material = std::make_unique<Material>();
	material->Name = _name;
	material->MatCBIndex = _CBIndex;
	material->DiffuseSrvHeapIndex = _SRVDiffIndex;
	material->NormalSrvHeapIndex = _SRVNMapIndex;

	material->DiffuseAlbedo = _DiffuseAlbedo;
	material->FresnelR0 = _FresnelR0;
	material->Roughness = _Roughness;
	mMaterials[_name] = std::move(material);
}
void TexColumnsApp::BuildDescriptorHeaps()
{//
	// 1. Создаём SRV хип с учётом GBuffer
	//
	UINT numTextureSRVs = static_cast<UINT>(mTextures.size());
	UINT numGBufferSRVs = 4; // albedo, normal, world pos, roughness

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = numTextureSRVs + numGBufferSRVs;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// 2. Создаём RTV хип под GBuffer
	//
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 4;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvDescriptorHeap)));

	//
	// 3. Заполняем SRV дескрипторы для обычных текстур
	//
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	int offset = 0;
	for (const auto& tex : mTextures) {
		auto text = tex.second->Resource;
		srvDesc.Format = text->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = text->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(text.Get(), &srvDesc, hDescriptor);
		TexOffsets[tex.first] = offset;
		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
		offset++;
	}

	mGBuffer.SrvHeapStartIndex = offset;

	//
	// 4. Создаём дескрипторы для GBuffer
	//
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[4];
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandles[4];

	UINT rtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UINT srvDescriptorSize = mCbvSrvDescriptorSize;

	// Получаем первый дескриптор из RTV и SRV хипов
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE gbufferSrvHandle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), offset, srvDescriptorSize);

	// Собираем массивы дескрипторов
	for (int i = 0; i < 4; ++i) {
		rtvHandles[i] = rtvHandle;
		rtvHandle.Offset(1, rtvDescriptorSize);

		srvHandles[i] = gbufferSrvHandle;
		gbufferSrvHandle.Offset(1, srvDescriptorSize);
	}

	//
	// 5. Инициализация GBuffer
	//

	mGBuffer.Initialize(md3dDevice.Get(), mClientWidth, mClientHeight, rtvHandles, srvHandles);

}

void TexColumnsApp::BuildShadersAndInputLayout()
{

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["gbufferVS"] = d3dUtil::CompileShader(L"Shaders\\GeometryPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["gbufferVSTerrain"] = d3dUtil::CompileShader(L"Shaders\\GeometryPass.hlsl", nullptr, "VSTerrain", "vs_5_1");
	mShaders["gbufferPS"] = d3dUtil::CompileShader(L"Shaders\\GeometryPass.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["fullscreenVS"] = d3dUtil::CompileShader(L"Shaders\\LightingPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["lightingPS"] = d3dUtil::CompileShader(L"Shaders\\LightingPass.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\Debug.hlsl", nullptr, "VSMain", "vs_5_1");
	mShaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\Debug.hlsl", nullptr, "PSMain", "ps_5_1");


	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mDebugInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}
void TexColumnsApp::BuildCustomMeshGeometry(std::string name, UINT& meshVertexOffset, UINT& meshIndexOffset, UINT& prevVertSize, UINT& prevIndSize, std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices, MeshGeometry* Geo)
{
	std::vector<GeometryGenerator::MeshData> meshDatas; // Это твоя структура для хранения вершин и индексов

	// Создаем инстанс импортера.
	Assimp::Importer importer;

	// Читаем файл с постпроцессингом: триангуляция, флип UV (если нужно) и генерация нормалей.
	const aiScene* scene = importer.ReadFile("../../Common/" + name + ".obj",
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_FlipUVs |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace);
	if (!scene || !scene->mRootNode)
	{
		std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
	}
	unsigned int nMeshes = scene->mNumMeshes;
	ObjectsMeshCount[name] = nMeshes;

	

	for (int i = 0; i < scene->mNumMeshes; i++)
	{
		GeometryGenerator::MeshData meshData;
		aiMesh* mesh = scene->mMeshes[i];

		// Подготовка контейнеров для вершин и индексов.
		std::vector<GeometryGenerator::Vertex> vertices;
		std::vector<std::uint16_t> indices;

		XMFLOAT3 _vMin;
		_vMin.x = scene->mMeshes[i]->mVertices[0].x;
		_vMin.y = scene->mMeshes[i]->mVertices[0].y;
		_vMin.z = scene->mMeshes[i]->mVertices[0].z;
		XMVECTOR vMin = XMLoadFloat3(&_vMin);
		XMVECTOR vMax = vMin;

		// Проходим по всем вершинам и копируем данные.
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		{
			GeometryGenerator::Vertex v;

			v.Position.x = mesh->mVertices[i].x;
			v.Position.y = mesh->mVertices[i].y;
			v.Position.z = mesh->mVertices[i].z;

			XMVECTOR _v = XMLoadFloat3(&v.Position);
			vMin = XMVectorMin(vMin, _v);
			vMax = XMVectorMax(vMax, _v);

			if (mesh->HasNormals())
			{
				v.Normal.x = mesh->mNormals[i].x;
				v.Normal.y = mesh->mNormals[i].y;
				v.Normal.z = mesh->mNormals[i].z;
			}

			if (mesh->HasTextureCoords(0))
			{
				v.TexC.x = mesh->mTextureCoords[0][i].x;
				v.TexC.y = mesh->mTextureCoords[0][i].y;
			}
			else
			{
				v.TexC = XMFLOAT2(0.0f, 0.0f);
			}
			if (mesh->HasTangentsAndBitangents() && false)
			{
				v.TangentU.x = mesh->mTangents[i].x;
				v.TangentU.y = mesh->mTangents[i].y;
				v.TangentU.z = mesh->mTangents[i].z;

			}

			// Если необходимо, можно обработать тангенты и другие атрибуты.
			vertices.push_back(v);
		}
		// Проходим по всем граням для формирования индексов.
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			// Убедимся, что грань треугольная.
			if (face.mNumIndices != 3) continue;
			indices.push_back(static_cast<std::uint16_t>(face.mIndices[0]));
			indices.push_back(static_cast<std::uint16_t>(face.mIndices[1]));
			indices.push_back(static_cast<std::uint16_t>(face.mIndices[2]));
		}

		// Заполняем meshData. Здесь тебе нужно адаптировать под свою структуру:
		meshData.Vertices = vertices;
		meshData.Indices32.resize(indices.size());
		XMStoreFloat3(&meshData.Bounds.Center, XMVectorAdd(vMin, vMax) / 2);
		XMStoreFloat3(&meshData.Bounds.Center, XMVectorSubtract(vMin, vMax) / 2);
		for (size_t j = 0; j < indices.size(); ++j)
			meshData.Indices32[j] = indices[j];

		aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

		aiString texPath;

		meshData.matName = scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
		// Если требуется, можно выполнить дополнительные операции, например, нормализацию, вычисление тангенсов и т.д.
		meshDatas.push_back(meshData);
	}

	for (int k = 0; k < scene->mNumMaterials; k++)
	{
		aiString texPath;
		scene->mMaterials[k]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
		std::string a = std::string(texPath.C_Str());
		a = a.substr(0, a.length() - 4);
		std::cout << "DIFFUSE: " << a << "\n";
		scene->mMaterials[k]->GetTexture(aiTextureType_DISPLACEMENT, 0, &texPath);
		std::string b = std::string(texPath.C_Str());
		b = b.substr(0, b.length() - 4);
		std::cout << "NORMAL: " << b << "\n";

		CreateMaterial(scene->mMaterials[k]->GetName().C_Str(), k, TexOffsets[a], TexOffsets[b], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	}

	UINT totalMeshSize = 0;
	UINT k = vertices.size();
	std::vector<std::pair<GeometryGenerator::MeshData, SubmeshGeometry>>meshSubmeshes;
	
	for (auto mesh : meshDatas)
	{
		meshVertexOffset = meshVertexOffset + prevVertSize;
		prevVertSize = mesh.Vertices.size();
		totalMeshSize += mesh.Vertices.size();

		meshIndexOffset = meshIndexOffset + prevIndSize;
		prevIndSize = mesh.Indices32.size();
		SubmeshGeometry meshSubmesh;
		meshSubmesh.IndexCount = (UINT)mesh.Indices32.size();
		meshSubmesh.StartIndexLocation = meshIndexOffset;
		meshSubmesh.BaseVertexLocation = meshVertexOffset;
		meshSubmesh.Bounds = mesh.Bounds;
		GeometryGenerator::MeshData m = mesh;
		meshSubmeshes.push_back(std::make_pair(m, meshSubmesh));
	}
	/////////
	/////
	for (auto mesh : meshDatas)
	{
		for (size_t i = 0; i < mesh.Vertices.size(); ++i, ++k)
		{
			vertices.push_back(Vertex(mesh.Vertices[i].Position, mesh.Vertices[i].Normal, mesh.Vertices[i].TexC, mesh.Vertices[i].TangentU));
		}
	}
	////////

	///////
	for (auto mesh : meshDatas)
	{
		indices.insert(indices.end(), std::begin(mesh.GetIndices16()), std::end(mesh.GetIndices16()));
	}
	///////
	Geo->MultiDrawArgs[name] = meshSubmeshes;
}


void TexColumnsApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();


	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));



	UINT meshVertexOffset = cylinderVertexOffset;
	UINT meshIndexOffset = cylinderIndexOffset;
	UINT prevIndSize = (UINT)cylinder.Indices32.size();
	UINT prevVertSize = (UINT)cylinder.Vertices.size();

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";
	BuildCustomMeshGeometry("sponza", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("negr", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("Pirate", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("left", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("right", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("plane2", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("diablo3_pose", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());




	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);




	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

void TexColumnsApp::BuildPSOs()
{

	//
	// PSO for opaque objects.
	//
	//ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	//
	// === Обычный opaque PSO (использует старую сигнатуру) ===
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = {};
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get(); // старая сигнатура
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// === Geometry Pass PSO (записывает в G-Buffer) ===
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC geoPsoDesc = opaquePsoDesc;
	geoPsoDesc.pRootSignature = mGeometryRootSignature.Get(); // новая сигнатура

	geoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["gbufferVS"]->GetBufferPointer()),
		mShaders["gbufferVS"]->GetBufferSize()
	};
	geoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["gbufferPS"]->GetBufferPointer()),
		mShaders["gbufferPS"]->GetBufferSize()
	};

	geoPsoDesc.NumRenderTargets = GBuffer::NumTextures;
	geoPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;     // Albedo
	geoPsoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normal
	geoPsoDesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT; // WorldPos
	geoPsoDesc.RTVFormats[3] = DXGI_FORMAT_R8_UNORM;           // Roughness

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&geoPsoDesc, IID_PPV_ARGS(&mPSOs["gbuffer"])));


	//
	// === Geometry Pass Terrain PSO (записывает в G-Buffer с heightmap) ===
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC TgeoPsoDesc = geoPsoDesc;
	TgeoPsoDesc.pRootSignature = mTerrainGeometryRootSignature.Get(); // terrain сигнатура

	TgeoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["gbufferVSTerrain"]->GetBufferPointer()),
		mShaders["gbufferVSTerrain"]->GetBufferSize()
	};

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&TgeoPsoDesc, IID_PPV_ARGS(&mPSOs["gbufferTerrain"])));

	//
	// === Lighting Pass PSO (освещение по G-Buffer) ===
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC lightPsoDesc = opaquePsoDesc;
	lightPsoDesc.pRootSignature = mLightingRootSignature.Get();

	lightPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["fullscreenVS"]->GetBufferPointer()),
		mShaders["fullscreenVS"]->GetBufferSize()
	};
	lightPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["lightingPS"]->GetBufferPointer()),
		mShaders["lightingPS"]->GetBufferSize()
	};

	lightPsoDesc.InputLayout = { nullptr, 0 }; // fullscreen quad не требует входных данных
	lightPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	lightPsoDesc.NumRenderTargets = 1;
	lightPsoDesc.RTVFormats[0] = mBackBufferFormat;
	lightPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN; // нет глубины

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&lightPsoDesc, IID_PPV_ARGS(&mPSOs["lighting"])));

	//
	// === Transparent PSO (оставляем старую логику) ===
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPSO = opaquePsoDesc;

	transparentPSO.BlendState.RenderTarget[0].BlendEnable = TRUE;
	transparentPSO.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparentPSO.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparentPSO.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	transparentPSO.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPSO, IID_PPV_ARGS(&mPSOs["transparent"])));
	//
	// == Debug PSO ==
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPSO = {};
	ZeroMemory(&debugPSO, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	debugPSO.InputLayout = { mDebugInputLayout.data(), (UINT)mDebugInputLayout.size() };
	debugPSO.pRootSignature = mDebugRootSignature.Get();
	debugPSO.VS = {
		reinterpret_cast<BYTE*>(mShaders["debugVS"]->GetBufferPointer()),
		mShaders["debugVS"]->GetBufferSize()
	};
	debugPSO.PS = {
		reinterpret_cast<BYTE*>(mShaders["debugPS"]->GetBufferPointer()),
		mShaders["debugPS"]->GetBufferSize()
	};
	debugPSO.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	debugPSO.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	debugPSO.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	debugPSO.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	debugPSO.SampleMask = UINT_MAX;
	debugPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	debugPSO.NumRenderTargets = 1;
	debugPSO.RTVFormats[0] = mBackBufferFormat; // например, DXGI_FORMAT_R8G8B8A8_UNORM
	debugPSO.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	debugPSO.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	debugPSO.DSVFormat = mDepthStencilFormat;
	debugPSO.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&debugPSO, IID_PPV_ARGS(&mPSOs["debug"])));
}

void TexColumnsApp::BuildFrameResources()
{
	FlushCommandQueue();
	mFrameResources.clear();
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
	mCurrFrameResourceIndex = 0;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();
	for (auto& ri : mAllRitems)
	{
		ri->NumFramesDirty = gNumFrameResources;
	}
	for (auto& kv : mMaterials)
	{
		kv.second->NumFramesDirty = gNumFrameResources;
	}
}

void TexColumnsApp::BuildMaterials()
{
	CreateMaterial("NiggaMat", 0, TexOffsets["textures/texture"], TexOffsets["textures/texture_nm"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	//CreateMaterial("NiggaMat", 0, TexOffsets["textures/terrain_hm"], TexOffsets["textures/texture_nm"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	CreateMaterial("eye", 0, TexOffsets["textures/eye"], TexOffsets["textures/eye_nm"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	CreateMaterial("map", 0, TexOffsets["textures/terrain_diffuse"], TexOffsets["textures/terrain_nm"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	CreateMaterial("map2", 0, TexOffsets["textures/HeightMap"], TexOffsets["textures/HeightMap_nm"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	CreateMaterial("bricks", 0, TexOffsets["textures/bricks"], TexOffsets["textures/bricks_nm"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	CreateMaterial("diabloMat", 0, TexOffsets["textures/diablo3_pose_diffuse"], TexOffsets["textures/diablo3_pose_nm"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
}
void TexColumnsApp::RenderCustomMesh(std::string unique_name, std::string meshname, std::string materialName, XMMATRIX Scale, XMMATRIX Rotation, XMMATRIX Translation, bool terrain = false)
{
	for (int i = 0; i < ObjectsMeshCount[meshname]; i++)
	{
		auto rItem = std::make_unique<RenderItem>();
		std::string textureFile;
		rItem->Name = unique_name;
		XMStoreFloat4x4(&rItem->TexTransform, XMMatrixScaling(1, 1., 1.));
		XMStoreFloat4x4(&rItem->World, Scale * Rotation * Translation);
		rItem->ObjCBIndex = mAllRitems.size();
		rItem->Geo = mGeometries["shapeGeo"].get();
		rItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		std::string matname = rItem->Geo->MultiDrawArgs[meshname][i].first.matName;
		std::cout << " mat : " << matname << "\n";
		std::cout << unique_name << " " << matname << "\n";
		if (materialName != "") matname = materialName;
		rItem->Mat = mMaterials[matname].get();
		rItem->IndexCount = rItem->Geo->MultiDrawArgs[meshname][i].second.IndexCount;
		rItem->StartIndexLocation = rItem->Geo->MultiDrawArgs[meshname][i].second.StartIndexLocation;
		rItem->BaseVertexLocation = rItem->Geo->MultiDrawArgs[meshname][i].second.BaseVertexLocation;
		rItem->aabb = rItem->Geo->MultiDrawArgs[meshname][i].second.Bounds;
		//XMStoreFloat3(&rItem->aabb.Center, XMVector3Transform(XMLoadFloat3(&rItem->aabb.Center), Scale * Rotation * Translation));
		//XMStoreFloat3(&rItem->aabb.Extents, XMVector3Transform(XMLoadFloat3(&rItem->aabb.Extents), Scale * Rotation * Translation));
		mAllRitems.push_back(std::move(rItem));

		if (!terrain) {
			mOpaqueRitems.push_back(mAllRitems[mAllRitems.size() - 1].get());
		}
		else {
			mTerrainTiles.push_back(mAllRitems[mAllRitems.size() - 1].get());
		}
	}
	BuildFrameResources();
}



void TexColumnsApp::BuildRenderItems()
{
	//RenderCustomMesh("building", "sponza", "", XMMatrixScaling(0.07, 0.07, 0.07), XMMatrixRotationRollPitchYaw(0, 3.14 / 2, 0), XMMatrixTranslation(0, 0, 0));
	RenderCustomMesh("nigga", "negr", "NiggaMat", XMMatrixScaling(3, 3, 3), XMMatrixRotationRollPitchYaw(0, 3.14, 0), XMMatrixTranslation(10, 3, 0));
	RenderCustomMesh("abbox", "negr", "bricks", XMMatrixScaling(3, 3, 3), XMMatrixRotationRollPitchYaw(0, 3.14, 0), XMMatrixTranslation(0, 15, 0));
	RenderCustomMesh("diablo3", "diablo3_pose", "diabloMat", XMMatrixScaling(3, 3, 3), XMMatrixRotationRollPitchYaw(0, 3.14, 0), XMMatrixTranslation(15, 5, -9));
	RenderCustomMesh("eyeL", "left", "eye", XMMatrixScaling(3, 3, 3), XMMatrixRotationRollPitchYaw(0, 3.14, 0), XMMatrixIdentity());
	RenderCustomMesh("eyeR", "right", "eye", XMMatrixScaling(3, 3, 3), XMMatrixRotationRollPitchYaw(0, 3.14, 0), XMMatrixIdentity());
	//RenderCustomMesh("pirate", "Pirate", "Body_mat", XMMatrixScaling(3, 3, 3), XMMatrixRotationRollPitchYaw(0, 3.14, 0), XMMatrixIdentity());
	RenderCustomMesh("plan", "plane2", "map", XMMatrixScaling(3, 3, 3), XMMatrixRotationRollPitchYaw(3.14, 0, 3.14), XMMatrixTranslation(0,-10,0), true);
	//RenderCustomMesh("plan", "plane2", "map2", XMMatrixScaling(3, 3, 3), XMMatrixRotationRollPitchYaw(3.14, 0, 3.14), XMMatrixTranslation(0,10,0));
	// All the render items are opaque.
	for (auto& e : mAllRitems)
	{
		if (e->Name == "plan")
		{
			XMStoreFloat4x4(&e->TexTransform, XMMatrixScaling(1, 1, 1));
		}
		mOpaqueRitems.push_back(e.get());
	}
}

void TexColumnsApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, bool height = false)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	cam.UpdateViewMatrix();

	XMMATRIX view = cam.GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];
		XMMATRIX world = XMLoadFloat4x4(&ri->World);
		XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);

		XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);

		/*BoundingFrustum localSpaceFrustum;
		mCamFrustum.Transform(localSpaceFrustum, viewToLocal);

		if (localSpaceFrustum.Contains(ri->aabb) == DISJOINT)
		{
			//continue;
		}*/
		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE diffuseHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		diffuseHandle.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(0, diffuseHandle);

		CD3DX12_GPU_DESCRIPTOR_HANDLE normalHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		normalHandle.Offset(ri->Mat->NormalSrvHeapIndex, mCbvSrvDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(1, normalHandle);


		CD3DX12_GPU_DESCRIPTOR_HANDLE heightHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		heightHandle.Offset(mHeightMapHeapIndex, mCbvSrvDescriptorSize);
		if (height) {
			
			cmdList->SetGraphicsRootDescriptorTable(2, heightHandle);
		}

		//// Получаем дескриптор для нормальной карты по её оффсету.
		//CD3DX12_GPU_DESCRIPTOR_HANDLE normalHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		//normalHandle.Offset(ri->Mat->NormalSrvHeapIndex, mCbvSrvDescriptorSize);
		//cmdList->SetGraphicsRootDescriptorTable(1, normalHandle);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(2, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(4, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TexColumnsApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

