#include "stdafx.h"

#include "DX11SceneRepHashSDF.h"

#include <iostream>
#include <cassert>
#include <list>

ID3D11ComputeShader*	DX11SceneRepHashSDF::s_SDFVoxelHashResetHeap = NULL;
ID3D11ComputeShader*	DX11SceneRepHashSDF::s_SDFVoxelHashResetHash = NULL;
ID3D11ComputeShader*	DX11SceneRepHashSDF::s_SDFVoxelStarve = NULL;
ID3D11ComputeShader*	DX11SceneRepHashSDF::s_SDFVoxelHashAlloc = NULL;
ID3D11ComputeShader*	DX11SceneRepHashSDF::s_SDFVoxelHashIntegrate = NULL;

ID3D11ComputeShader*	DX11SceneRepHashSDF::s_HashDecisionArrayFiller = NULL;
ID3D11ComputeShader*	DX11SceneRepHashSDF::s_HashCompactify = NULL;

DX11ScanCS				DX11SceneRepHashSDF::s_PrefixSumScan;

ID3D11ComputeShader*	DX11SceneRepHashSDF::s_GarbageCollectIdentify = NULL;
ID3D11ComputeShader*	DX11SceneRepHashSDF::s_GarbageCollectIdentifyOld = NULL;
ID3D11ComputeShader*	DX11SceneRepHashSDF::s_GarbageCollectFree = NULL;

ID3D11ComputeShader*	DX11SceneRepHashSDF::s_SDFVoxelHashRemoveAndIntegrateOutFrustum = NULL;
ID3D11ComputeShader*	DX11SceneRepHashSDF::s_SDFVoxelHashRemoveAndIntegrateInFrustum = NULL;

Timer					DX11SceneRepHashSDF::s_Timer;

DX11SceneRepHashSDF::DX11SceneRepHashSDF()
{
	m_SDFBlockSize = SDF_BLOCK_SIZE;
	m_NumIntegratedImages = 0;
	m_LastRigidTransform.setIdentity();

	m_Hash = NULL;
	m_HashUAV = NULL;
	m_HashSRV = NULL;

	//wei add
	m_PCXYZID = NULL;
	m_PCXYZIDUAV = NULL;
	m_PCXYZIDSRV = NULL;	

	m_PointBID = NULL;
	m_PointBIDUAV = NULL;
	m_PointBIDSRV = NULL;

	m_BIDArray = NULL;
	m_BIDArrayUAV = NULL;
	m_BIDArraySRV = NULL;
	//wei end

	m_HashBucketMutex = NULL;
	m_HashBucketMutexUAV = NULL;
	m_HashBucketMutexSRV = NULL;

	m_HashIntegrateDecision  = NULL;
	m_HashIntegrateDecisionUAV = NULL;
	m_HashIntegrateDecisionSRV = NULL;

	m_HashIntegrateDecisionPrefix = NULL;
	m_HashIntegrateDecisionPrefixUAV = NULL;
	m_HashIntegrateDecisionPrefixSRV = NULL;

	m_HashCompactified = NULL;
	m_HashCompactifiedUAV = NULL;
	m_HashCompactifiedSRV = NULL;

	m_Heap = NULL;
	m_HeapSRV = NULL;
	m_HeapUAV = NULL;
	m_HeapStaticUAV = NULL;
	m_HeapFreeCount = NULL;

	m_SDFBlocksSDF = NULL;
	m_SDFBlocksSDFSRV = NULL;
	m_SDFBlocksSDFUAV = NULL;

	m_SDFBlocksRGBW = NULL;
	m_SDFBlocksRGBWSRV = NULL;
	m_SDFBlocksRGBWUAV = NULL;

	m_SDFVoxelHashCB = NULL;

	m_bEnableGarbageCollect = true;
}

DX11SceneRepHashSDF::~DX11SceneRepHashSDF()
{
	Destroy();
}

HRESULT DX11SceneRepHashSDF::OnD3D11CreateDevice( ID3D11Device* pd3dDevice )
{
	HRESULT hr = S_OK;

	char SDFBLOCKSIZE[5];
	sprintf_s(SDFBLOCKSIZE, "%d", SDF_BLOCK_SIZE);

	char HANDLECOLLISIONS[5];
	sprintf_s(HANDLECOLLISIONS, "%d", GlobalAppState::getInstance().s_HANDLE_COLLISIONS);

	char blockSize[5];
	sprintf_s(blockSize, "%d", THREAD_GROUP_SIZE_SCENE_REP);
	D3D_SHADER_MACRO macro[] = {{"groupthreads", blockSize}, { "SDF_BLOCK_SIZE", SDFBLOCKSIZE }, {"HANDLE_COLLISIONS", HANDLECOLLISIONS }, {0}};
	D3D_SHADER_MACRO macro2[] = {{"groupthreads", blockSize}, { "SDF_BLOCK_SIZE", SDFBLOCKSIZE }, {0}};
	
	D3D_SHADER_MACRO* validDefines = macro;
	if(GlobalAppState::getInstance().s_HANDLE_COLLISIONS == 0)
	{
		validDefines = macro2;
	}
	
	ID3DBlob* pBlob = NULL;
	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "integrateCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_SDFVoxelHashIntegrate));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "resetHeapCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_SDFVoxelHashResetHeap));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "resetHashCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_SDFVoxelHashResetHash));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "starveVoxelsCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_SDFVoxelStarve));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "allocCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_SDFVoxelHashAlloc));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "decisionArrayFillerCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_HashDecisionArrayFiller));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "compactifyHashCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_HashCompactify));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "garbadgeCollectIdentifyCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_GarbageCollectIdentify));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "garbadgeCollectIdentifyOldCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_GarbageCollectIdentifyOld));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "garbadgeCollectFreeCS", "cs_5_0", &pBlob, validDefines));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_GarbageCollectFree));


	D3D_SHADER_MACRO macro_out = {"MOVE_OUT_FRUSTUM", "1"};
	D3D_SHADER_MACRO macro_in = {"MOVE_IN_FRUSTUM", "1"};

	D3D_SHADER_MACRO macro_and_out[10];
	D3D_SHADER_MACRO macro_and_in[10];
	AddDefinitionToMacro(validDefines, macro_and_out, macro_out);
	AddDefinitionToMacro(validDefines, macro_and_in, macro_in);

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "removeAndIntegrateCS", "cs_5_0", &pBlob, macro_and_out));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_SDFVoxelHashRemoveAndIntegrateOutFrustum));

	V_RETURN(CompileShaderFromFile(L"Shaders\\SceneRepSDF.hlsl", "removeAndIntegrateCS", "cs_5_0", &pBlob, macro_and_in));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &s_SDFVoxelHashRemoveAndIntegrateInFrustum));


	SAFE_RELEASE(pBlob);

	V_RETURN(s_PrefixSumScan.OnD3D11CreateDevice(pd3dDevice))

	return hr;
}

void DX11SceneRepHashSDF::OnD3D11DestroyDevice()
{
	SAFE_RELEASE(s_SDFVoxelHashIntegrate);
	SAFE_RELEASE(s_SDFVoxelHashAlloc);
	SAFE_RELEASE(s_SDFVoxelHashResetHeap);
	SAFE_RELEASE(s_SDFVoxelHashResetHash);
	SAFE_RELEASE(s_SDFVoxelStarve);
	SAFE_RELEASE(s_HashDecisionArrayFiller);
	SAFE_RELEASE(s_HashCompactify);
	SAFE_RELEASE(s_GarbageCollectIdentify);
	SAFE_RELEASE(s_GarbageCollectIdentifyOld);
	SAFE_RELEASE(s_GarbageCollectFree);
	SAFE_RELEASE(s_SDFVoxelHashRemoveAndIntegrateInFrustum);
	SAFE_RELEASE(s_SDFVoxelHashRemoveAndIntegrateOutFrustum);

	s_PrefixSumScan.OnD3D11DestroyDevice();
}

void DX11SceneRepHashSDF::Destroy()
{
	m_NumIntegratedImages = 0;

	SAFE_RELEASE(m_Hash);
	SAFE_RELEASE(m_HashSRV);
	SAFE_RELEASE(m_HashUAV);
	
	//wei add
	SAFE_RELEASE(m_PCXYZID);
	SAFE_RELEASE(m_PCXYZIDSRV);
	SAFE_RELEASE(m_PCXYZIDUAV);

	SAFE_RELEASE(m_PointBID);
	SAFE_RELEASE(m_PointBIDUAV);
	SAFE_RELEASE(m_PointBIDSRV);

	SAFE_RELEASE(m_BIDArray);
	SAFE_RELEASE(m_BIDArrayUAV);
	SAFE_RELEASE(m_BIDArraySRV);
	//wei end

	SAFE_RELEASE(m_HashBucketMutex);
	SAFE_RELEASE(m_HashBucketMutexSRV);
	SAFE_RELEASE(m_HashBucketMutexUAV);

	SAFE_RELEASE(m_Heap);
	SAFE_RELEASE(m_HeapSRV);
	SAFE_RELEASE(m_HeapUAV);
	SAFE_RELEASE(m_HeapStaticUAV);
	SAFE_RELEASE(m_HeapFreeCount);

	SAFE_RELEASE(m_SDFBlocksSDF);
	SAFE_RELEASE(m_SDFBlocksSDFSRV);
	SAFE_RELEASE(m_SDFBlocksSDFUAV);

	SAFE_RELEASE(m_SDFBlocksRGBW);
	SAFE_RELEASE(m_SDFBlocksRGBWSRV);
	SAFE_RELEASE(m_SDFBlocksRGBWUAV);


	SAFE_RELEASE(m_HashIntegrateDecision);
	SAFE_RELEASE(m_HashIntegrateDecisionUAV);
	SAFE_RELEASE(m_HashIntegrateDecisionSRV);

	SAFE_RELEASE(m_HashIntegrateDecisionPrefix);
	SAFE_RELEASE(m_HashIntegrateDecisionPrefixUAV);
	SAFE_RELEASE(m_HashIntegrateDecisionPrefixSRV);

	SAFE_RELEASE(m_HashCompactified);
	SAFE_RELEASE(m_HashCompactifiedUAV);
	SAFE_RELEASE(m_HashCompactifiedSRV);

	SAFE_RELEASE(m_SDFVoxelHashCB);
}

HRESULT DX11SceneRepHashSDF::Init( ID3D11Device* pd3dDevice, bool justHash /*= false*/, unsigned int hashNumBuckets /*= 300000*/, unsigned int hashBucketSize /*= 10*/, unsigned int numSDFBlocks /*= 100000*/, float voxelSize /*= 0.005f*/ )
{
	HRESULT hr = S_OK;

	m_JustHashAndNoSDFBlocks = justHash;
	m_HashNumBuckets = hashNumBuckets;
	m_HashBucketSize = hashBucketSize;
	m_VirtualVoxelSize = voxelSize;
	m_SDFNumBlocks = numSDFBlocks;

	V_RETURN(CreateBuffers(pd3dDevice));

	return hr;
}

void DX11SceneRepHashSDF::Integrate( ID3D11DeviceContext* context, ID3D11ShaderResourceView* inputDepth, ID3D11ShaderResourceView* inputColor, ID3D11ShaderResourceView* bitMask, const mat4f* rigidTransform)
{
	m_LastRigidTransform = *rigidTransform;		
	if (m_JustHashAndNoSDFBlocks)	return;	//in this case we cannot integrate and we just updated rigid transform

	//////////////////
	// Alloc Phase
	//////////////////
	MapConstantBuffer(context);
	Alloc(context, inputDepth, inputColor, bitMask);


	//////////////////
	// Compactify
	//////////////////

	// Start query for timing
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		s_Timer.start();
	}
	CompactifyHashEntries(context);
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		TimingLog::totalTimeCompactifyHash+=s_Timer.getElapsedTimeMS();
		TimingLog::countCompactifyHash++;
	}

	////////////////////////
	// Integrate
	////////////////////////
	MapConstantBuffer(context);	//we need to remap the buffer since numOccupiedEntries was re-computed by 'Compacitfy'
	IntegrateDepthMap(context, inputDepth, inputColor);


	if (false)
	{
		std::cout << "occupied hash entries: " << m_NumOccupiedHashEntries << std::endl;
		std::cout << "free blocks (before free)\t: " << GetHeapFreeCount(context) << " ( " << m_SDFNumBlocks << " ) " << std::endl;
		std::cout << "free + occupied blocks: " << m_NumOccupiedHashEntries + GetHeapFreeCount(context) << std::endl;
	}

	//////////////////////
	// Garbage Collect
	//////////////////////
	if (m_bEnableGarbageCollect) {
		if (GetNumIntegratedImages() > 1 && GetNumIntegratedImages() % 15 == 0) {	//reduce one weight every 15 frames
			StarveVoxelWeights(context);	
		}
		GarbageCollect(context);
	}


	if (false)
	{
		//std::cout << "occupied hash entries: " << m_NumOccupiedHashEntries << std::endl;
		std::cout << "free blocks (after free)\t: " << GetHeapFreeCount(context) << " ( " << m_SDFNumBlocks << " ) " << std::endl;
		//std::cout << "free + occupied blocks: " << m_NumOccupiedHashEntries + GetHeapFreeCount(context) << std::endl;
	}

	//CreateAndCopyToDebugBuf(DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext(), m_HashCompactified);
	//DebugHash();
	//CreateAndCopyToDebugBuf(DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext(), m_SDFBlocks);

	m_NumIntegratedImages++;
}

void DX11SceneRepHashSDF::RemoveAndIntegrateToOther( ID3D11DeviceContext* context, DX11SceneRepHashSDF* other, const mat4f* lastRigid, bool moveOutsideFrustum )
{
	HRESULT hr = S_OK;
	if (lastRigid) {
		mat4f oldLastRigid = m_LastRigidTransform;
		m_LastRigidTransform = *lastRigid;
		MapConstantBuffer(context);
		m_LastRigidTransform = oldLastRigid;

		oldLastRigid = other->m_LastRigidTransform;
		other->m_LastRigidTransform = *lastRigid;		
		other->MapConstantBuffer(context);
		other->m_LastRigidTransform = oldLastRigid;
	}

	context->CSSetUnorderedAccessViews( 0, 1, &m_HashUAV, 0);
	context->CSSetUnorderedAccessViews( 1, 1, &other->m_HashUAV, 0);

	context->CSSetConstantBuffers(0, 1, &m_SDFVoxelHashCB);
	context->CSSetConstantBuffers(1, 1, &other->m_SDFVoxelHashCB);
	ID3D11Buffer* CBGlobalAppState = GlobalAppState::getInstance().MapAndGetConstantBuffer(context);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);

	if (moveOutsideFrustum)		context->CSSetShader(s_SDFVoxelHashRemoveAndIntegrateOutFrustum, 0, 0);
	else						context->CSSetShader(s_SDFVoxelHashRemoveAndIntegrateInFrustum, 0, 0);

	unsigned int groupeThreads = THREAD_GROUP_SIZE_SCENE_REP*THREAD_GROUP_SIZE_SCENE_REP*8;
	unsigned int dimX = (m_HashNumBuckets * m_HashBucketSize + groupeThreads - 1) / groupeThreads;
	unsigned int dimY = 1;
	assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	
	assert(dimY <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	

	context->Dispatch(dimX, dimY, 1);

	ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL };
	ID3D11UnorderedAccessView* nullUAV[] = { NULL, NULL };
	ID3D11Buffer* nullCB[] = { NULL };

	context->CSSetUnorderedAccessViews(0, 2, nullUAV, 0);
	context->CSSetConstantBuffers(0, 1, nullCB);
	context->CSSetConstantBuffers(8, 1, nullCB);
	context->CSSetShader(0, 0, 0);
}

void DX11SceneRepHashSDF::Reset( ID3D11DeviceContext* context )
{
	MapConstantBuffer(context);

	context->CSSetConstantBuffers(0, 1, &m_SDFVoxelHashCB);

	// Start Compute Shader
	unsigned int groupThreads = THREAD_GROUP_SIZE_SCENE_REP*THREAD_GROUP_SIZE_SCENE_REP*8;
	unsigned int dimX = (m_HashNumBuckets * m_HashBucketSize + groupThreads - 1) / groupThreads;

	ID3D11Buffer* CBGlobalAppState = GlobalAppState::getInstance().MapAndGetConstantBuffer(context);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);
	context->CSSetUnorderedAccessViews(0, 1, &m_HashUAV, NULL);
	context->CSSetShader(s_SDFVoxelHashResetHash, NULL, 0);

	assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);
	
	context->Dispatch(dimX, 1, 1);

	if (!m_JustHashAndNoSDFBlocks) {
		dimX = (m_SDFNumBlocks + groupThreads - 1) / groupThreads;
		unsigned int initUAVCount = (unsigned int)-1;
		context->CSSetUnorderedAccessViews(1, 1, &m_SDFBlocksSDFUAV, NULL);
		context->CSSetUnorderedAccessViews(7, 1, &m_SDFBlocksRGBWUAV, NULL);
		context->CSSetUnorderedAccessViews(4, 1, &m_HeapStaticUAV, NULL);
		context->CSSetShader(s_SDFVoxelHashResetHeap, NULL, 0);

		assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);
	
		context->Dispatch(dimX, 1, 1);
	}

	//unsigned int initalCount = m_SDFNumBlocks;
	//context->CSSetUnorderedAccessViews( 0, 1, &m_HeapUAV, &initalCount);

	ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11UnorderedAccessView* nullUAV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11Buffer* nullCB[] = { NULL };

	context->CSSetShaderResources(0, 6, nullSRV);
	context->CSSetUnorderedAccessViews(0, 6, nullUAV, 0);
	context->CSSetUnorderedAccessViews(7, 1, nullUAV, 0);
	context->CSSetConstantBuffers(0, 1, nullCB);
	context->CSSetConstantBuffers(8, 1, nullCB);
	context->CSSetShader(0, 0, 0);

	unsigned int initalCount = m_SDFNumBlocks;
	context->CSSetUnorderedAccessViews( 0, 1, &m_HeapUAV, &initalCount);
	context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);

	m_LastRigidTransform.setIdentity();
	m_NumIntegratedImages = 0;
	m_NumOccupiedHashEntries = 0;
}

unsigned int DX11SceneRepHashSDF::GetHeapFreeCount( ID3D11DeviceContext* context )
{
	HRESULT hr = S_OK;
	context->CopyStructureCount(m_HeapFreeCount, 0, m_HeapUAV);
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	V(context->Map(m_HeapFreeCount, 0, D3D11_MAP_READ, 0, &mappedResource));
	unsigned int val =  ((unsigned int*)mappedResource.pData)[0];
	context->Unmap(m_HeapFreeCount, 0);
	return val;
}

HRESULT DX11SceneRepHashSDF::DumpPointCloud( const std::string &filename, ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, unsigned int minWeight /*= 1*/, bool justOccupied /*= false*/ )
{
	HRESULT hr = S_OK;

	ID3D11Buffer* pBuffer = m_Hash;//数据在m_Hash中

	ID3D11Buffer* debugbuf = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof(desc) );
	pBuffer->GetDesc( &desc );
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	V_RETURN(pDevice->CreateBuffer(&desc, NULL, &debugbuf));//debugbuf 是最好返回的指向创建的buffer的指针

	pd3dImmediateContext->CopyResource( debugbuf, pBuffer );//把m_hash中的内容拷贝到debugbuffer中来
	unsigned int numElements = desc.ByteWidth/sizeof(INT); //desc的bytewidth难道就是所存的总共的字节数？？


	INT *cpuMemory = new INT[numElements]; //在CPU端分配出对应的内存

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	//这个是在CPU端获取对debugbuf的读取权限， 后面就可以用mappedResource来访问指定内存
	V(pd3dImmediateContext->Map(debugbuf, D3D11CalcSubresource(0,0,0), D3D11_MAP_READ, 0, &mappedResource));	
	//把内存拷贝到cupMemory中
	memcpy((void*)cpuMemory, (void*)mappedResource.pData, desc.ByteWidth);
	//得到数据之后， 返还权限给GPU
	pd3dImmediateContext->Unmap( debugbuf, 0 );
	 
	PointCloudf points;
	for (unsigned int i = 0; i < numElements / 3; i++) {
		if (cpuMemory[3*i+2] == -2)	continue;	//ignore non-allocated voxels

		int3 a;
		a.x = cpuMemory[3*i+0] & 0x0000ffff;
		if (a.x & (0x1 << 15)) a.x |= 0xffff0000;
		a.y = cpuMemory[3*i+0] >> 16;
		if (a.y & (0x1 << 15)) a.y |= 0xffff0000;
		a.z = cpuMemory[3*i+1] & 0x0000ffff;
		if (a.z & (0x1 << 15)) a.z |= 0xffff0000;

		vec3f p;
		p.x = (float)a.x;
		p.y = (float)a.y;
		p.z = (float)a.z;
		points.m_points.push_back(p);
	}


	std::cout << "Dumping voxel grid " << filename <<  " ( " << points.m_points.size() << " ) ...";
	PointCloudIOf::saveToFile(filename, points);
	std::cout << " done!" << std::endl;
	SAFE_RELEASE(debugbuf);
	SAFE_DELETE_ARRAY(cpuMemory);

	return hr;
}

HRESULT DX11SceneRepHashSDF::LoadPointCloud( ID3D11Device* pDevice )
{
	HRESULT hr = S_OK;

	/*std::cout<<"directy return S_OK in LoadPointCloud" <<std::endl;
	return hr;*/

	//步骤1:读入带有path_id的数据
	MeshDataf mesh_with_id;
	mesh_with_id.m_Vertices.push_back(vec3f(0.724041, 0.50689, 1.41012));
	mesh_with_id.m_Colors.push_back(vec3f(1.0f, 0.0f, 0.0f));


	mesh_with_id.m_Vertices.push_back(vec3f(0.0906226, 0.0550402, 0.418263));
	mesh_with_id.m_Colors.push_back(vec3f(2.0f, 0.0f, 0.0f));

	mesh_with_id.m_Vertices.push_back(vec3f(0.13409, 0.16703, 0.489359));
	mesh_with_id.m_Colors.push_back(vec3f(3.0f, 0.0f, 0.0f));

	mesh_with_id.m_Vertices.push_back(vec3f(0.146088, 0.175028, 0.479096));
	mesh_with_id.m_Colors.push_back(vec3f(4.0f, 0.0f, 0.0f));

	/*MeshIOf::loadFromPLY("./Scans/scan.ply", mesh_with_id);
	std::cout<<"position vert size: " <<mesh_with_id.m_Vertices.size() <<std::endl;
	std::cout<<"color vert size: " <<mesh_with_id.m_Colors.size() <<std::endl;*/

	//步骤2：把数据写入到buffer中去
	D3D11_BUFFER_DESC descBUF;
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;

	//2.1 create buf, 需要根据点云的数量创建对应大小的buff
	int nPoints = (int)mesh_with_id.m_Vertices.size();
	const int nelements_for_each_point = 5; //x y z patch_id object_id

	ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
	descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	descBUF.Usage	= D3D11_USAGE_DEFAULT;
	descBUF.CPUAccessFlags = 0;
	descBUF.MiscFlags	= 0;
	descBUF.ByteWidth	= sizeof(float) * ( 1 + nelements_for_each_point * nPoints);//第1个元素放点云的数量，后面对应每个点放点的 x y z patch_id obj_id
	descBUF.StructureByteStride = sizeof(int);

	//这块数据需要用读进来的pointcloud去显示
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
	float* cpuNull = new float[1 + nelements_for_each_point * nPoints];//cpuNULL 是初始化那块内存的数据
	cpuNull[0] = static_cast<float>(nPoints); //第一个位置上放点的数量， 后面每5个位置对应一个点的数据
	for (int i = 0; i < nPoints; i++) {
		vec3f &pt_pos = mesh_with_id.m_Vertices[i];
		float color_red = mesh_with_id.m_Colors[i].x;
		cpuNull[1 + nelements_for_each_point * i + 0] = pt_pos.x;
		cpuNull[1 + nelements_for_each_point * i + 1] = pt_pos.y;
		cpuNull[1 + nelements_for_each_point * i + 2] = pt_pos.z;
		//std::cout<<"color_red: "<<color_red <<std::endl;
		cpuNull[1 + nelements_for_each_point * i + 3] = color_red; //红色分量被用作 patch_id
		cpuNull[1 + nelements_for_each_point * i + 4] = -2.0f;   //这个分量目前没有被用到
	}
	InitData.pSysMem = cpuNull;
	V_RETURN(pDevice->CreateBuffer(&descBUF, &InitData, &m_PCXYZID));//用initData这块内存去初始化
	SAFE_DELETE_ARRAY(cpuNull);                                      //!!!!create完了之后就可以释放了!!!

	//2.2 create srv
	ZeroMemory( &descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
	descSRV.Format = DXGI_FORMAT_R32_FLOAT;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	descSRV.Buffer.FirstElement = 0;
	descSRV.Buffer.NumElements = nelements_for_each_point * nPoints;
	V_RETURN(pDevice->CreateShaderResourceView(m_PCXYZID, &descSRV, &m_PCXYZIDSRV));

	//2.3 create uav
	ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
	descUAV.Format = DXGI_FORMAT_R32_FLOAT;
	descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	descUAV.Buffer.FirstElement = 0;
	descUAV.Buffer.NumElements =  nelements_for_each_point * nPoints;
	V_RETURN(pDevice->CreateUnorderedAccessView(m_PCXYZID, &descUAV, &m_PCXYZIDUAV));

	//3.1 为SetVoxelID函数准备的RWbuffer， 创建后面用的PointBID, 这两个buffer的大小根据上面的点云进行分配
	ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
	descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	descBUF.Usage	= D3D11_USAGE_DEFAULT;
	descBUF.CPUAccessFlags = 0;
	descBUF.MiscFlags	= 0;
	descBUF.ByteWidth	= sizeof(int) * (nPoints);
	descBUF.StructureByteStride = sizeof(int);

	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
	int* cpuNull2 = new int[nPoints];
	for (int i = 0; i < nPoints; i++) {
		cpuNull2[i] = -1;
	}
	InitData.pSysMem = cpuNull2;
	V_RETURN(pDevice->CreateBuffer(&descBUF, &InitData, &m_PointBID));
	SAFE_DELETE_ARRAY(cpuNull);                                    

	//3.2
	ZeroMemory( &descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
	descSRV.Format = DXGI_FORMAT_R32_SINT;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	descSRV.Buffer.FirstElement = 0;
	descSRV.Buffer.NumElements = nPoints;
	V_RETURN(pDevice->CreateShaderResourceView(m_PointBID, &descSRV, &m_PointBIDSRV));

	//3.3
	ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
	descUAV.Format = DXGI_FORMAT_R32_SINT;
	descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	descUAV.Buffer.FirstElement = 0;
	descUAV.Buffer.NumElements =  nPoints;
	V_RETURN(pDevice->CreateUnorderedAccessView(m_PointBID, &descUAV, &m_PointBIDUAV));

	//4.1
	ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
	descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	descBUF.Usage	= D3D11_USAGE_DEFAULT;
	descBUF.CPUAccessFlags = 0;
	descBUF.MiscFlags	= 0;
	descBUF.ByteWidth	= sizeof(int) * (nPoints);
	descBUF.StructureByteStride = sizeof(int);

	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
	cpuNull2 = new int[nPoints];
	for (int i = 0; i < nPoints; i++) {
		cpuNull2[i] = -1;
	}
	InitData.pSysMem = cpuNull2;
	V_RETURN(pDevice->CreateBuffer(&descBUF, &InitData, &m_BIDArray));
	SAFE_DELETE_ARRAY(cpuNull);                                    

	//4.2
	ZeroMemory( &descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
	descSRV.Format = DXGI_FORMAT_R32_SINT;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	descSRV.Buffer.FirstElement = 0;
	descSRV.Buffer.NumElements = nPoints;
	V_RETURN(pDevice->CreateShaderResourceView(m_BIDArray, &descSRV, &m_BIDArraySRV));

	//4.3
	ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
	descUAV.Format = DXGI_FORMAT_R32_SINT;
	descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	descUAV.Buffer.FirstElement = 0;
	descUAV.Buffer.NumElements =  nPoints;
	V_RETURN(pDevice->CreateUnorderedAccessView(m_BIDArray, &descUAV, &m_BIDArrayUAV));

	return S_OK;
}

HRESULT DX11SceneRepHashSDF::SetVoxelID( ID3D11Device *pd3dDevice, ID3D11DeviceContext* context)
{
	HRESULT hr;

	D3D_SHADER_MACRO Shader_Macros[] = { "D3D11", "1" , NULL, NULL };
	ID3DBlob* pBlob = NULL;
	ID3D11ComputeShader* m_setVoxelIDComputeShader = NULL;
	V_RETURN(CompileShaderFromFile(L"Shaders\\RayCastingHashSDF.hlsl", "SetVoxelIDByPointCloud", "cs_5_0", &pBlob, Shader_Macros)); //, D3DCOMPILE_SKIP_VALIDATION | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_OPTIMIZATION_LEVEL0));
	V_RETURN(pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &m_setVoxelIDComputeShader));
	SAFE_RELEASE(pBlob);

	context->CSSetShaderResources(7, 1, &m_PCXYZIDSRV);
	context->CSSetUnorderedAccessViews(8, 1, &m_PointBIDUAV, NULL);
	context->CSSetUnorderedAccessViews(9, 1, &m_BIDArrayUAV, NULL);
	context->CSSetUnorderedAccessViews(10, 1, &m_SDFBlocksSDFUAV, NULL);
	context->CSSetUnorderedAccessViews(11, 1, &m_SDFBlocksRGBWUAV, NULL);
	ID3D11Buffer* CBGlobalAppState = GlobalAppState::getInstance().MapAndGetConstantBuffer(context);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);

	context->CSSetShader(m_setVoxelIDComputeShader, 0, 0);
	context->Dispatch(1, 1, 1);

	//clean up
	ID3D11UnorderedAccessView* nullUAV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11Buffer* nullB[1] = {NULL};
	context->CSSetUnorderedAccessViews(7, 1, nullUAV, 0);
	context->CSSetUnorderedAccessViews(8, 1, nullUAV, 0);
	context->CSSetUnorderedAccessViews(9, 1, nullUAV, 0);
	context->CSSetUnorderedAccessViews(10, 1, nullUAV, 0);
	context->CSSetUnorderedAccessViews(11, 1, nullUAV, 0);
	context->CSSetConstantBuffers(8, 1, nullB);

	context->CSSetShader(0, 0, 0);
	return S_OK;
}

void DX11SceneRepHashSDF::MapConstantBuffer( ID3D11DeviceContext* context )
{
	HRESULT hr = S_OK;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	V(context->Map(m_SDFVoxelHashCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	CB_VOXEL_HASH_SDF* cbuffer = (CB_VOXEL_HASH_SDF*)mappedResource.pData;
	memcpy(cbuffer->m_RigidTransform, &m_LastRigidTransform, sizeof(mat4f)); 
	//D3DXMatrixInverse(&cbuffer->m_RigidTransformInverse, NULL, &cbuffer->m_RigidTransform);
	D3DXMatrixInverse(&cbuffer->m_RigidTransformInverse, NULL, (D3DXMATRIX*)&m_LastRigidTransform);
	cbuffer->m_HashNumBuckets = m_HashNumBuckets;
	cbuffer->m_HashBucketSize = m_HashBucketSize;
	cbuffer->m_InputImageWidth = GlobalAppState::getInstance().s_windowWidth;
	cbuffer->m_InputImageHeight = GlobalAppState::getInstance().s_windowHeight;
	cbuffer->m_VirtualVoxelSize = m_VirtualVoxelSize;
	cbuffer->m_VirtualVoxelResolutionScalar = 1.0f/m_VirtualVoxelSize;
	cbuffer->m_NumSDFBlocks = m_SDFNumBlocks;
	cbuffer->m_NumOccupiedSDFBlocks = m_NumOccupiedHashEntries;
	context->Unmap(m_SDFVoxelHashCB, 0);
}

void DX11SceneRepHashSDF::Alloc(ID3D11DeviceContext* context, ID3D11ShaderResourceView* inputDepth, ID3D11ShaderResourceView* inputColor, ID3D11ShaderResourceView* bitMask)
{
	context->CSSetShaderResources(0, 1, &inputDepth);
	context->CSSetShaderResources(1, 1, &inputColor);
	context->CSSetShaderResources(8, 1, &bitMask);
	context->CSSetUnorderedAccessViews(0, 1, &m_HashUAV, NULL);
	UINT cleanUAV[] = {0,0,0,0};
	context->ClearUnorderedAccessViewUint(m_HashBucketMutexUAV, cleanUAV);
	context->CSSetUnorderedAccessViews(5, 1, &m_HashBucketMutexUAV, NULL);
	unsigned int initUAVCount = (unsigned int)-1;
	context->CSSetUnorderedAccessViews(2, 1, &m_HeapUAV, &initUAVCount);	//consume buffer (slot 2)
	context->CSSetConstantBuffers(0, 1, &m_SDFVoxelHashCB);
	ID3D11Buffer* CBGlobalAppState = GlobalAppState::getInstance().MapAndGetConstantBuffer(context);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);
	context->CSSetShader(s_SDFVoxelHashAlloc, NULL, 0);

	const unsigned int imageWidth = GlobalAppState::getInstance().s_windowWidth;
	const unsigned int imageHeight = GlobalAppState::getInstance().s_windowHeight;


	unsigned int dimX = (unsigned int)ceil(((float)imageWidth)/THREAD_GROUP_SIZE_SCENE_REP);
	unsigned int dimY = (unsigned int)ceil(((float)imageHeight)/THREAD_GROUP_SIZE_SCENE_REP);
	assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	
	assert(dimY <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	

	// Start query for timing
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		s_Timer.start();
	}


	context->Dispatch(dimX, dimY, 1);

	// Wait for query
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		TimingLog::totalTimeAlloc+=s_Timer.getElapsedTimeMS();
		TimingLog::countAlloc++;
	}

	ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11UnorderedAccessView* nullUAV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11Buffer* nullCB[] = { NULL };

	context->CSSetShaderResources(0, 6, nullSRV);
	context->CSSetShaderResources(8, 1, nullSRV);
	context->CSSetUnorderedAccessViews(0, 6, nullUAV, 0);
	context->CSSetConstantBuffers(0, 1, nullCB);
	context->CSSetConstantBuffers(8, 1, nullCB);
	context->CSSetShader(0, 0, 0);
}

void DX11SceneRepHashSDF::CompactifyHashEntries( ID3D11DeviceContext* context )
{
	context->CSSetShaderResources(3, 1, &m_HashSRV);
	context->CSSetUnorderedAccessViews(6, 1, &m_HashIntegrateDecisionUAV, NULL);
	context->CSSetConstantBuffers(0, 1, &m_SDFVoxelHashCB);
	ID3D11Buffer* CBGlobalAppState = GlobalAppState::getInstance().MapAndGetConstantBuffer(context);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);

	unsigned int groupThreads = THREAD_GROUP_SIZE_SCENE_REP*THREAD_GROUP_SIZE_SCENE_REP*8;
	unsigned int dimX = (m_HashNumBuckets * m_HashBucketSize + groupThreads - 1) / groupThreads;
	unsigned int dimY = 1;

	context->CSSetShader(s_HashDecisionArrayFiller, NULL, 0);
	assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	
	assert(dimY <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	

	context->Dispatch(dimX, 1, 1);

	ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11UnorderedAccessView* nullUAV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11Buffer* nullCB[] = { NULL };

	context->CSSetShaderResources(3, 1, nullSRV);
	context->CSSetUnorderedAccessViews(6, 1, nullUAV, NULL);
	context->CSSetConstantBuffers(0, 1, nullCB);
	context->CSSetConstantBuffers(8, 1, nullCB);
	context->CSSetShader(0, 0, 0);


	m_NumOccupiedHashEntries = s_PrefixSumScan.ScanCS(
		context, m_HashNumBuckets * m_HashBucketSize, 
		m_HashIntegrateDecisionSRV, m_HashIntegrateDecisionUAV, 
		m_HashIntegrateDecisionPrefixSRV, m_HashIntegrateDecisionPrefixUAV,
		m_HashIntegrateDecisionPrefix
		);


	context->CSSetShaderResources(2, 1, &m_HashIntegrateDecisionPrefixSRV);
	context->CSSetShaderResources(3, 1, &m_HashSRV);
	context->CSSetShaderResources(6, 1, &m_HashIntegrateDecisionSRV);
	context->CSSetUnorderedAccessViews(7, 1, &m_HashCompactifiedUAV, NULL);
	context->CSSetConstantBuffers(0, 1, &m_SDFVoxelHashCB);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);

	context->CSSetShader(s_HashCompactify, NULL, 0);
	assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);
	context->Dispatch(dimX, 1, 1);

	context->CSSetShaderResources(2, 2, nullSRV);
	context->CSSetShaderResources(6, 1, nullSRV);
	context->CSSetUnorderedAccessViews(7, 1, nullUAV, NULL);
	context->CSSetConstantBuffers(0, 1, nullCB);
	context->CSSetConstantBuffers(8, 1, nullCB);
	context->CSSetShader(0, 0, 0);
}

void DX11SceneRepHashSDF::IntegrateDepthMap( ID3D11DeviceContext* context, ID3D11ShaderResourceView* inputDepth, ID3D11ShaderResourceView* inputColor )
{
	context->CSSetShaderResources(0, 1, &inputDepth);
	context->CSSetShaderResources(1, 1, &inputColor);
	context->CSSetShaderResources(4, 1, &m_HashCompactifiedSRV);
	context->CSSetUnorderedAccessViews(1, 1, &m_SDFBlocksSDFUAV, NULL);
	context->CSSetUnorderedAccessViews(7, 1, &m_SDFBlocksRGBWUAV, NULL);
	context->CSSetConstantBuffers(0, 1, &m_SDFVoxelHashCB);
	ID3D11Buffer* CBGlobalAppState = GlobalAppState::getInstance().MapAndGetConstantBuffer(context);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);

	context->CSSetShader(s_SDFVoxelHashIntegrate, NULL, 0);
	//groupThreads = BLOCK_SIZE_SCENE_REP*BLOCK_SIZE_SCENE_REP;
	//dimX = (m_NumOccupiedHashEntries + groupThreads - 1) / groupThreads;
	unsigned int dimX = NUM_GROUPS_X;
	unsigned int dimY = (m_NumOccupiedHashEntries + NUM_GROUPS_X - 1) / NUM_GROUPS_X;
	assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	
	assert(dimY <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	

	// Start query for timing
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		s_Timer.start();
	}

	context->Dispatch(dimX, dimY, 1);

	// Wait for query
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		TimingLog::totalTimeIntegrate+=s_Timer.getElapsedTimeMS();
		TimingLog::countIntegrate++;
	}

	ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11UnorderedAccessView* nullUAV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11Buffer* nullCB[] = { NULL };

	context->CSSetShaderResources(0, 2, nullSRV);
	context->CSSetShaderResources(4, 1, nullSRV);
	context->CSSetUnorderedAccessViews(1, 1, nullUAV, NULL);
	context->CSSetUnorderedAccessViews(7, 1, nullUAV, NULL);
	context->CSSetConstantBuffers(0, 1, nullCB);
	context->CSSetConstantBuffers(8, 1, nullCB);
	context->CSSetShader(0, 0, 0);
}

void DX11SceneRepHashSDF::GarbageCollect( ID3D11DeviceContext* context )
{
	unsigned int dimX;
	unsigned int dimY;
	unsigned int initUAVCount = (unsigned int)-1;
	unsigned int groupThreads;

	ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11UnorderedAccessView* nullUAV[] = { NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11Buffer* nullCB[] = { NULL };

	/* Has to be adapted
	ID3D11Buffer* CBGlobalAppState = GlobalAppState::getInstance().MapAndGetConstantBuffer(context);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);

	context->CSSetConstantBuffers(8, 1, nullB);

	//old TEST TEST identify all sdfblocks that need to be freed
	context->CSSetShaderResources(4, 1, &m_HashCompactifiedSRV);
	context->CSSetShaderResources(5, 1, &m_SDFBlocksSRV);
	context->CSSetUnorderedAccessViews(3, 1, &m_HeapUAV, &initUAVCount);
	context->CSSetUnorderedAccessViews(6, 1, &m_HashIntegrateDecisionUAV, NULL);
	context->CSSetConstantBuffers(0, 1, &m_SDFVoxelHashCB);

	context->CSSetShader(s_GarbageCollectIdentifyOld, NULL, 0);
	groupThreads = THREAD_GROUP_SIZE_SCENE_REP*THREAD_GROUP_SIZE_SCENE_REP;
	dimX = (m_NumOccupiedHashEntries + groupThreads - 1) / groupThreads;
	dimY = 1;
	//dimX = NUM_GROUPS_X;
	//dimY = (m_NumOccupiedHashEntries + NUM_GROUPS_X - 1) / NUM_GROUPS_X;
	assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	
	assert(dimY <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);

	// Start query for timing
	if(GlobalAppState::getInstance().s_timingsEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		s_Timer.start();
	}


	context->Dispatch(dimX, dimY, 1);
	
	// Wait for query
	if(GlobalAppState::getInstance().s_timingsEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		TimingLog::totalTimeGarbageCollect0+=s_Timer.getElapsedTimeMS();
		TimingLog::countGarbageCollect0++;
	}

	context->CSSetShaderResources(4, 2, nullSRV);
	context->CSSetUnorderedAccessViews(3, 1, nullUAV, NULL);
	context->CSSetUnorderedAccessViews(6, 1, nullUAV, NULL);
	context->CSSetConstantBuffers(0, 1, nullCB);
	context->CSSetShader(0, 0, 0);
	*/

	//INT* memory0 = (INT*)CreateAndCopyToDebugBuf(DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext(), m_HashIntegrateDecision, true);
	//unsigned int numValid0 = 0;
	//for (unsigned int i = 0; i < m_NumOccupiedHashEntries; i++) {
	//	if (memory0[i] == 1) numValid0++;
	//}


	
	//first identify all sdfblocks that need to be freed
	context->CSSetShaderResources(4, 1, &m_HashCompactifiedSRV);
	context->CSSetShaderResources(5, 1, &m_SDFBlocksSDFSRV);
	context->CSSetShaderResources(7, 1, &m_SDFBlocksRGBWSRV);
	context->CSSetUnorderedAccessViews(3, 1, &m_HeapUAV, &initUAVCount);
	context->CSSetUnorderedAccessViews(6, 1, &m_HashIntegrateDecisionUAV, NULL);
	context->CSSetConstantBuffers(0, 1, &m_SDFVoxelHashCB);
	ID3D11Buffer* CBGlobalAppState = GlobalAppState::getInstance().MapAndGetConstantBuffer(context);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);

	context->CSSetShader(s_GarbageCollectIdentify, NULL, 0);
	//groupThreads = BLOCK_SIZE_SCENE_REP*BLOCK_SIZE_SCENE_REP;
	//dimX = (m_NumOccupiedHashEntries + groupThreads - 1) / groupThreads;
	dimX = NUM_GROUPS_X;
	dimY = (m_NumOccupiedHashEntries + NUM_GROUPS_X - 1) / NUM_GROUPS_X;
	assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	
	assert(dimY <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	



	// Start query for timing
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		s_Timer.start();
	}

	context->Dispatch(dimX, dimY, 1);

	// Wait for query
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		TimingLog::totalTimeGarbageCollect0+=s_Timer.getElapsedTimeMS();
		TimingLog::countGarbageCollect0++;
	}

	context->CSSetShaderResources(4, 2, nullSRV);
	context->CSSetShaderResources(7, 1, nullSRV);
	context->CSSetUnorderedAccessViews(3, 1, nullUAV, NULL);
	context->CSSetUnorderedAccessViews(5, 1, nullUAV, NULL);
	context->CSSetUnorderedAccessViews(6, 1, nullUAV, NULL);
	context->CSSetConstantBuffers(0, 1, nullCB);
	context->CSSetConstantBuffers(8, 1, nullCB);
	context->CSSetShader(0, 0, 0);
	

	//INT* memory1 = (INT*)CreateAndCopyToDebugBuf(DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext(), m_HashIntegrateDecision, true);
	//unsigned int numValid1 = 0;
	//for (unsigned int i = 0; i < m_NumOccupiedHashEntries; i++) {
	//	if (memory1[i] == 1) numValid1++;
	//}
	//assert(numValid0 == numValid1);


	//unsigned int* data0 = (unsigned int*)CreateAndCopyToDebugBuf(DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext(), m_HashIntegrateDecision, true);
	//unsigned int numFreed = 0;
	//for (unsigned int i = 0; i < m_NumOccupiedHashEntries; i++) {
	//	if (data0[i] == 1) numFreed++;
	//}
	//unsigned int* data1 = (unsigned int*)CreateAndCopyToDebugBuf(DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext(), m_Heap, true);
	//unsigned int freeBefore = GetHeapFreeCount(context);


	//free all identified sdfblocks
	context->CSSetShaderResources(4, 1, &m_HashCompactifiedSRV);
	context->CSSetShaderResources(6, 1, &m_HashIntegrateDecisionSRV);
	context->CSSetUnorderedAccessViews(0, 1, &m_HashUAV, NULL);
	context->CSSetUnorderedAccessViews(1, 1, &m_SDFBlocksSDFUAV, NULL);
	context->CSSetUnorderedAccessViews(7, 1, &m_SDFBlocksRGBWUAV, NULL);
	context->CSSetUnorderedAccessViews(3, 1, &m_HeapUAV, &initUAVCount);
	context->CSSetConstantBuffers(0, 1, &m_SDFVoxelHashCB);
	context->CSSetConstantBuffers(8, 1, &CBGlobalAppState);

	context->CSSetShader(s_GarbageCollectFree, NULL, 0);
	groupThreads = THREAD_GROUP_SIZE_SCENE_REP*THREAD_GROUP_SIZE_SCENE_REP;
	dimX = (m_NumOccupiedHashEntries + groupThreads - 1) / groupThreads;
	dimY = 1;
	//dimX = NUM_GROUPS_X;
	//dimY = (m_NumOccupiedHashEntries + NUM_GROUPS_X - 1) / NUM_GROUPS_X;
	//assert(dimX <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);	
	//assert(dimY <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);

	// Start query for timing
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		s_Timer.start();
	}

	UINT cleanUAV[] = {0,0,0,0};
	context->ClearUnorderedAccessViewUint(m_HashBucketMutexUAV, cleanUAV);
	context->CSSetUnorderedAccessViews(5, 1, &m_HashBucketMutexUAV, NULL);

	context->Dispatch(dimX, dimY, 1);

	// Wait for query
	if(GlobalAppState::getInstance().s_timingsDetailledEnabled)
	{
		GlobalAppState::getInstance().WaitForGPU();
		TimingLog::totalTimeGarbageCollect1+=s_Timer.getElapsedTimeMS();
		TimingLog::countGarbageCollect1++;
	}

	context->CSSetShaderResources(4, 1, nullSRV);
	context->CSSetShaderResources(6, 1, nullSRV);
	context->CSSetUnorderedAccessViews(0, 1, nullUAV, NULL);
	context->CSSetUnorderedAccessViews(1, 1, nullUAV, NULL);
	context->CSSetUnorderedAccessViews(3, 1, nullUAV, NULL);
	context->CSSetUnorderedAccessViews(5, 1, nullUAV, NULL);
	context->CSSetUnorderedAccessViews(7, 1, nullUAV, NULL);
	context->CSSetConstantBuffers(0, 1, nullCB);
	context->CSSetConstantBuffers(8, 1, nullCB);
	context->CSSetShader(0, 0, 0);
}

HRESULT DX11SceneRepHashSDF::CreateBuffers( ID3D11Device* pd3dDevice )
{
	HRESULT hr = S_OK;

	m_LastRigidTransform.setIdentity();

	D3D11_BUFFER_DESC bDesc;
	ZeroMemory(&bDesc, sizeof(D3D11_BUFFER_DESC));
	bDesc.BindFlags	= D3D11_BIND_CONSTANT_BUFFER;
	bDesc.Usage	= D3D11_USAGE_DYNAMIC;
	bDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bDesc.MiscFlags	= 0;
	bDesc.ByteWidth	= sizeof(CB_VOXEL_HASH_SDF);
	V_RETURN(pd3dDevice->CreateBuffer(&bDesc, NULL, &m_SDFVoxelHashCB));


	D3D11_BUFFER_DESC descBUF;
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;

	//create hash buffers/uav/srv (each element is a int3 -> (short3, short, int))
	ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
	descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	descBUF.Usage	= D3D11_USAGE_DEFAULT;
	descBUF.CPUAccessFlags = 0;
	descBUF.MiscFlags	= 0;
	descBUF.ByteWidth	= sizeof(int) * 3 * m_HashBucketSize * m_HashNumBuckets;
	descBUF.StructureByteStride = sizeof(int);

	ZeroMemory( &descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
	descSRV.Format = DXGI_FORMAT_R32_SINT;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	descSRV.Buffer.FirstElement = 0;
	descSRV.Buffer.NumElements = 3 * m_HashBucketSize * m_HashNumBuckets;

	ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
	descUAV.Format = DXGI_FORMAT_R32_SINT;
	descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	descUAV.Buffer.FirstElement = 0;
	descUAV.Buffer.NumElements =  3 * m_HashBucketSize * m_HashNumBuckets;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
	int* cpuNull = new int[3 * m_HashBucketSize * m_HashNumBuckets];
	for (unsigned int i = 0; i < m_HashBucketSize * m_HashNumBuckets; i++) {
		cpuNull[3*i+0] = 0;
		cpuNull[3*i+1] = 0;
		cpuNull[3*i+2] = -2;	//marks free memory
	}
	InitData.pSysMem = cpuNull;
	V_RETURN(pd3dDevice->CreateBuffer(&descBUF, &InitData, &m_Hash));
	V_RETURN(pd3dDevice->CreateBuffer(&descBUF, &InitData, &m_HashCompactified));
	SAFE_DELETE_ARRAY(cpuNull);

	V_RETURN(pd3dDevice->CreateShaderResourceView(m_Hash, &descSRV, &m_HashSRV));
	V_RETURN(pd3dDevice->CreateShaderResourceView(m_HashCompactified, &descSRV, &m_HashCompactifiedSRV));
	V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_Hash, &descUAV, &m_HashUAV));
	V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_HashCompactified, &descUAV, &m_HashCompactifiedUAV));

	if (!m_JustHashAndNoSDFBlocks)	{
		//create hash mutex
		ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
		descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		descBUF.Usage	= D3D11_USAGE_DEFAULT;
		descBUF.CPUAccessFlags = 0;
		descBUF.MiscFlags	= 0;
		descBUF.ByteWidth	= sizeof(unsigned int) * m_HashNumBuckets;
		descBUF.StructureByteStride = sizeof(unsigned int);

		ZeroMemory( &descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
		descSRV.Format = DXGI_FORMAT_R32_UINT;
		descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		descSRV.Buffer.FirstElement = 0;
		descSRV.Buffer.NumElements = m_HashNumBuckets;

		ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
		descUAV.Format = DXGI_FORMAT_R32_UINT;
		descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		descUAV.Buffer.FirstElement = 0;
		descUAV.Buffer.NumElements = m_HashNumBuckets;

		ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
		cpuNull = new int[m_HashNumBuckets];
		for (unsigned int i = 0; i < m_HashNumBuckets; i++) {
			cpuNull[i] = 0;
		}
		InitData.pSysMem = cpuNull;
		V_RETURN(pd3dDevice->CreateBuffer(&descBUF, &InitData, &m_HashBucketMutex));
		SAFE_DELETE_ARRAY(cpuNull);

		V_RETURN(pd3dDevice->CreateShaderResourceView(m_HashBucketMutex, &descSRV, &m_HashBucketMutexSRV));
		V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_HashBucketMutex, &descUAV, &m_HashBucketMutexUAV));



		//create heap (unsigned int -> ptr to data blocks)
		ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
		descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		descBUF.Usage	= D3D11_USAGE_DEFAULT;
		descBUF.CPUAccessFlags = 0;
		descBUF.MiscFlags	= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		descBUF.ByteWidth	= sizeof(unsigned int) * m_SDFNumBlocks;
		descBUF.StructureByteStride = sizeof(unsigned int);

		ZeroMemory (&descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		descSRV.Format = DXGI_FORMAT_UNKNOWN;
		descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		descSRV.Buffer.FirstElement = 0;
		descSRV.Buffer.NumElements = m_SDFNumBlocks;

		ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
		descUAV.Format = DXGI_FORMAT_UNKNOWN;
		descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		descUAV.Buffer.FirstElement = 0;
		descUAV.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
		descUAV.Buffer.NumElements =  m_SDFNumBlocks;

		cpuNull = new int[m_SDFNumBlocks];
		for (unsigned int i = 0; i < m_SDFNumBlocks; i++) {
			cpuNull[i] = m_SDFNumBlocks - i - 1;
		}
		InitData.pSysMem = cpuNull;
		V_RETURN(pd3dDevice->CreateBuffer(&descBUF, &InitData, &m_Heap));
		SAFE_DELETE_ARRAY(cpuNull);
		V_RETURN(pd3dDevice->CreateShaderResourceView(m_Heap, &descSRV, &m_HeapSRV));
		V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_Heap, &descUAV, &m_HeapUAV));

		ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
		descUAV.Format = DXGI_FORMAT_UNKNOWN;
		descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		descUAV.Buffer.FirstElement = 0;
		descUAV.Buffer.Flags = 0;
		descUAV.Buffer.NumElements = m_SDFNumBlocks;
		V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_Heap, &descUAV, &m_HeapStaticUAV))


		ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
		descBUF.BindFlags	= 0;
		descBUF.Usage	= D3D11_USAGE_STAGING;
		descBUF.CPUAccessFlags =  D3D11_CPU_ACCESS_READ;
		descBUF.MiscFlags	= 0;
		descBUF.ByteWidth	= sizeof(int);
		V_RETURN(pd3dDevice->CreateBuffer(&descBUF, NULL, &m_HeapFreeCount));

		unsigned int initalCount = m_SDFNumBlocks;
		DXUTGetD3D11DeviceContext()->CSSetUnorderedAccessViews(0, 1, &m_HeapUAV, &initalCount);
		ID3D11UnorderedAccessView* nullUAV[] = {NULL};
		DXUTGetD3D11DeviceContext()->CSSetUnorderedAccessViews(0, 1, nullUAV, NULL);


		//create sdf blocks (8x8x8 -> 8 byte per voxel: SDF part)
		ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
		descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		descBUF.Usage	= D3D11_USAGE_DEFAULT;
		descBUF.CPUAccessFlags = 0;
		descBUF.MiscFlags	= 0;
		descBUF.ByteWidth	= (sizeof(float)) * m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks;
		descBUF.StructureByteStride = sizeof(int);

		ZeroMemory( &descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
		descSRV.Format = DXGI_FORMAT_R32_FLOAT;
		descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		descSRV.Buffer.FirstElement = 0;
		descSRV.Buffer.NumElements = m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks;

		ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
		descUAV.Format = DXGI_FORMAT_R32_FLOAT;
		descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		descUAV.Buffer.FirstElement = 0;
		descUAV.Buffer.NumElements =  m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks;


		ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
		cpuNull = new int[m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks];
		ZeroMemory(cpuNull, sizeof(int) * m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks);
		InitData.pSysMem = cpuNull;
		V_RETURN(pd3dDevice->CreateBuffer(&descBUF, &InitData, &m_SDFBlocksSDF));
		SAFE_DELETE_ARRAY(cpuNull);

		V_RETURN(pd3dDevice->CreateShaderResourceView(m_SDFBlocksSDF, &descSRV, &m_SDFBlocksSDFSRV));
		V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_SDFBlocksSDF, &descUAV, &m_SDFBlocksSDFUAV));


		//create sdf blocks (8x8x8 -> 8 byte per voxel: RGBW part)
		ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
		descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		descBUF.Usage	= D3D11_USAGE_DEFAULT;
		descBUF.CPUAccessFlags = 0;
		descBUF.MiscFlags	= 0;
		descBUF.ByteWidth	= sizeof(int) * m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks;
		descBUF.StructureByteStride = sizeof(int);

		ZeroMemory( &descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
		descSRV.Format = DXGI_FORMAT_R32_SINT;
		descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		descSRV.Buffer.FirstElement = 0;
		descSRV.Buffer.NumElements = m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks;

		ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
		descUAV.Format = DXGI_FORMAT_R32_SINT;;
		descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		descUAV.Buffer.FirstElement = 0;
		descUAV.Buffer.NumElements =  m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks;


		ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
		cpuNull = new int[m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks];
		ZeroMemory(cpuNull, sizeof(int) * m_SDFBlockSize * m_SDFBlockSize * m_SDFBlockSize * m_SDFNumBlocks);
		InitData.pSysMem = cpuNull;
		V_RETURN(pd3dDevice->CreateBuffer(&descBUF, &InitData, &m_SDFBlocksRGBW));
		SAFE_DELETE_ARRAY(cpuNull);

		V_RETURN(pd3dDevice->CreateShaderResourceView(m_SDFBlocksRGBW, &descSRV, &m_SDFBlocksRGBWSRV));
		V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_SDFBlocksRGBW, &descUAV, &m_SDFBlocksRGBWUAV));

		//wei add
		LoadPointCloud(pd3dDevice);

		//{
		//	MeshDataf mesh_with_id;
		//	mesh_with_id.m_Vertices.push_back(vec3f(0.00650914, -0.001944, 0.462739));
		//	mesh_with_id.m_Colors.push_back(vec3f(1.0f, 0.0f, 0.0f));


		//	mesh_with_id.m_Vertices.push_back(vec3f(-0.0293826, 0.0620402, 0.430715));
		//	mesh_with_id.m_Colors.push_back(vec3f(3.0f, 0.0f, 0.0f));
		//	/*MeshIOf::loadFromPLY("./Scans/scan.ply", mesh_with_id);
		//	std::cout<<"position vert size: " <<mesh_with_id.m_Vertices.size() <<std::endl;
		//	std::cout<<"color vert size: " <<mesh_with_id.m_Colors.size() <<std::endl;*/

		//	//步骤2：把数据写入到buffer中去
		//	D3D11_BUFFER_DESC descBUF;
		//	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
		//	D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;

		//	//create buf, 需要根据点云的数量创建对应大小的buff
		//	int nPoints = (int)mesh_with_id.m_Vertices.size();
		//	const int nelements_for_each_point = 5; //x y z patch_id object_id

		//	ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
		//	descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		//	descBUF.Usage	= D3D11_USAGE_DEFAULT;
		//	descBUF.CPUAccessFlags = 0;
		//	descBUF.MiscFlags	= 0;
		//	descBUF.ByteWidth	= sizeof(float) * ( 1 + nelements_for_each_point * nPoints);//第1个元素放点云的数量，后面对应每个点放点的 x y z patch_id obj_id
		//	descBUF.StructureByteStride = sizeof(int);

		//	//这块数据需要用读进来的pointcloud去显示
		//	D3D11_SUBRESOURCE_DATA InitData;
		//	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
		//	int* cpuNull = new int[1 + nelements_for_each_point * nPoints];//cpuNULL 是初始化那块内存的数据
		//	cpuNull[0] = nPoints; //static_cast<int>(nPoints); //第一个位置上放点的数量， 后面每5个位置对应一个点的数据
		//	for (int i = 0; i < nPoints; i++) {
		//		vec3f &pt_pos = mesh_with_id.m_Vertices[i];
		//		float color_red = mesh_with_id.m_Colors[i].x;
		//		cpuNull[1 + nelements_for_each_point * i + 0] = pt_pos.x;
		//		cpuNull[1 + nelements_for_each_point * i + 1] = pt_pos.y;
		//		cpuNull[1 + nelements_for_each_point * i + 2] = pt_pos.z;
		//		//std::cout<<"color_red: "<<color_red <<std::endl;
		//		cpuNull[1 + nelements_for_each_point * i + 3] = color_red; //红色分量被用作 patch_id
		//		cpuNull[1 + nelements_for_each_point * i + 4] = -2.0f;   //这个分量目前没有被用到
		//	}
		//	InitData.pSysMem = cpuNull;
		//	V_RETURN(pd3dDevice->CreateBuffer(&descBUF, &InitData, &m_PCXYZID));//用initData这块内存去初始化
		//	SAFE_DELETE_ARRAY(cpuNull);                                      //!!!!create完了之后就可以释放了!!!

		//	//create srv
		//	ZeroMemory( &descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
		//	descSRV.Format = DXGI_FORMAT_R32_FLOAT;
		//	descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		//	descSRV.Buffer.FirstElement = 0;
		//	descSRV.Buffer.NumElements = nelements_for_each_point * nPoints;
		//	V_RETURN(pd3dDevice->CreateShaderResourceView(m_PCXYZID, &descSRV, &m_PCXYZIDSRV));

		//	//create uav
		//	ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
		//	descUAV.Format = DXGI_FORMAT_R32_FLOAT;
		//	descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		//	descUAV.Buffer.FirstElement = 0;
		//	descUAV.Buffer.NumElements =  nelements_for_each_point * nPoints;
		//	V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_PCXYZID, &descUAV, &m_PCXYZIDUAV));
		//}
	}

	////////////////
	// Compactify //
	////////////////

	//create hash buffers/uav/srv (each element is a int3 -> (short3, short, int))
	ZeroMemory(&descBUF, sizeof(D3D11_BUFFER_DESC));
	descBUF.BindFlags	= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	descBUF.Usage	= D3D11_USAGE_DEFAULT;
	descBUF.CPUAccessFlags = 0;
	descBUF.MiscFlags	= 0;
	descBUF.ByteWidth	= sizeof(int) * m_HashBucketSize * m_HashNumBuckets;
	descBUF.StructureByteStride = sizeof(int);

	ZeroMemory( &descSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
	descSRV.Format = DXGI_FORMAT_R32_SINT;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	descSRV.Buffer.FirstElement = 0;
	descSRV.Buffer.NumElements = m_HashBucketSize * m_HashNumBuckets;

	ZeroMemory( &descUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
	descUAV.Format = DXGI_FORMAT_R32_SINT;
	descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	descUAV.Buffer.FirstElement = 0;
	descUAV.Buffer.NumElements =  m_HashBucketSize * m_HashNumBuckets;


	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
	cpuNull = new int[m_HashBucketSize * m_HashNumBuckets];
	for (unsigned int i = 0; i < m_HashBucketSize * m_HashNumBuckets; i++) {
		cpuNull[i] = 0;
	}
	InitData.pSysMem = cpuNull;
	V_RETURN(pd3dDevice->CreateBuffer(&descBUF, &InitData, &m_HashIntegrateDecision));
	V_RETURN(pd3dDevice->CreateBuffer(&descBUF, &InitData, &m_HashIntegrateDecisionPrefix));
	SAFE_DELETE_ARRAY(cpuNull);

	V_RETURN(pd3dDevice->CreateShaderResourceView(m_HashIntegrateDecision, &descSRV, &m_HashIntegrateDecisionSRV));
	V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_HashIntegrateDecision, &descUAV, &m_HashIntegrateDecisionUAV));

	V_RETURN(pd3dDevice->CreateShaderResourceView(m_HashIntegrateDecisionPrefix, &descSRV, &m_HashIntegrateDecisionPrefixSRV));
	V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_HashIntegrateDecisionPrefix, &descUAV, &m_HashIntegrateDecisionPrefixUAV));



	return hr;
}
