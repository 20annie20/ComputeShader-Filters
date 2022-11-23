#include "DXApplication.h"

// Safe Release Function
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

/**
*	Initialize our DX application
*/
bool DXApplication::initialize(HWND hWnd, int width, int height) 
{
	HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE( driverTypes );

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_DRIVER_TYPE         driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL       featureLevel = D3D_FEATURE_LEVEL_11_0;

	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
	{
		driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain( NULL, driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice, &featureLevel, &m_pImmediateContext );
		if( SUCCEEDED( hr ) )
			break;
	}
	if( FAILED( hr ) )
		return false;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = m_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
	if( FAILED( hr ) )
		return false;

	hr = m_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &m_pRenderTargetView );
	pBackBuffer->Release();
	if( FAILED( hr ) )
		return false;

	m_pImmediateContext->OMSetRenderTargets( 1, &m_pRenderTargetView, NULL );

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_pImmediateContext->RSSetViewports( 1, &vp );

	// We then load a texture as a source of data for our compute shader
	if(!loadFullScreenQuad())
		return false;
	if(!loadTexture( L"data/countryside.bmp", &m_srcTexture ))
		return false;
	if(!createInputBuffer())
		return false;
	if(!createOutputBuffer())
		return false;
	if(!runComputeShader( L"data/Desaturate.hlsl", 32, 64))
		return false;

	return true;
}

bool DXApplication::runComputeShader( LPCWSTR shaderFilename, int groupSizeX, int groupSizeY ) 
{
	ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
	ID3D11ShaderResourceView* ppSRVNULL[2] = { NULL, NULL };

	if(!loadComputeShader( shaderFilename, &m_computeShader ))
		return false;

	m_pImmediateContext->CSSetShader( m_computeShader, NULL, 0 );
	m_pImmediateContext->CSSetShaderResources( 0, 1, &m_srcDataGPUBufferView );
	m_pImmediateContext->CSSetUnorderedAccessViews( 0, 1, &m_destDataGPUBufferView, 0 );

	m_pImmediateContext->Dispatch( groupSizeX, groupSizeY, 1 );

	m_pImmediateContext->CSSetShader( NULL, NULL, 0 );
	m_pImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, NULL );
	m_pImmediateContext->CSSetShaderResources( 0, 2, ppSRVNULL );

	// Copy the result into the destination texture
	if(m_destTexture)
		m_destTexture->Release();

	D3D11_TEXTURE2D_DESC desc;
	m_srcTexture->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.MipLevels = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if(m_pd3dDevice->CreateTexture2D(&desc, NULL, &m_destTexture) != S_OK)
		return false;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if(m_pImmediateContext->Map(m_destTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) != S_OK)
		return false;

	byte* gpuDestBufferCopy = getCopyOfGPUDestBuffer();
	memcpy(mappedResource.pData, gpuDestBufferCopy, m_textureDataSize);

	m_pImmediateContext->Unmap(m_destTexture, 0);

	// Create a view of the output texture
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc; 
	ZeroMemory(&viewDesc, sizeof(viewDesc));
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
	viewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;
	viewDesc.Texture2D.MostDetailedMip = 0;
	if( FAILED( m_pd3dDevice->CreateShaderResourceView( m_destTexture, &viewDesc, &m_destTextureView) ) )
	{	
		OutputDebugStringA( "Failed to create texture view" );
		return false;
	}

	return true;
}




/**
*	Update
*/
void DXApplication::update() 
{
}

/**
*	Render the scene
*/
void DXApplication::render() 
{
	float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	m_pImmediateContext->ClearRenderTargetView( m_pRenderTargetView, ClearColor );

	m_pImmediateContext->VSSetShader( m_pVertexShader, NULL, 0 );
	m_pImmediateContext->PSSetShader( m_pPixelShader, NULL, 0 );
	m_pImmediateContext->PSSetShaderResources( 0, 1, &m_srcTextureView );
	m_pImmediateContext->PSSetShaderResources( 1, 1, &m_destTextureView );
	m_pImmediateContext->PSSetSamplers( 0, 1, &m_pSamplerLinear );
	m_pImmediateContext->Draw( 4, 0 );

	m_pSwapChain->Present( 0, 0 );
}

/**
*	Release all the DX resources we have allocated
*/
void DXApplication::release() 
{
	if(m_srcTextureData)
		delete [] m_srcTextureData;
	m_srcTextureData = NULL;

	SafeRelease(&m_computeShader);

	SafeRelease(&m_destTexture);
	SafeRelease(&m_destTextureView);
	SafeRelease(&m_destDataGPUBuffer);
	SafeRelease(&m_destDataGPUBufferView);

	SafeRelease(&m_srcTexture);
	SafeRelease(&m_srcTextureView);
	SafeRelease(&m_srcDataGPUBuffer);
	SafeRelease(&m_srcDataGPUBufferView);

	SafeRelease(&m_pVertexShader);
	SafeRelease(&m_pPixelShader);
	SafeRelease(&m_pVertexLayout);
	SafeRelease(&m_pVertexBuffer);
	SafeRelease(&m_pSamplerLinear);

	if( m_pImmediateContext ) m_pImmediateContext->ClearState();
	SafeRelease(&m_pRenderTargetView);
	SafeRelease(&m_pSwapChain);
	SafeRelease(&m_pImmediateContext);
	SafeRelease(&m_pd3dDevice);
}

/**
*	Load full screen quad for rendering both src and dest texture.
*/
bool DXApplication::loadFullScreenQuad()
{
	struct SimpleVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Tex;
	};

	HRESULT hr;
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
	ID3DBlob* pErrorBlob;

	// Compile the vertex shader
	ID3DBlob* pVSBlob = NULL;
	if( FAILED( D3DX11CompileFromFile(L"./data/fullQuad.fx", NULL, NULL, "VS", "vs_4_0", dwShaderFlags, 0, NULL, &pVSBlob, &pErrorBlob, NULL) ) )
	{
		if( pErrorBlob != NULL )
		{
			OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
			pErrorBlob->Release();
		}
		return false;
	}
	if( pErrorBlob ) 
		pErrorBlob->Release();

	// Create the vertex shader
	hr = m_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_pVertexShader );
	if( FAILED( hr ) )
	{	
		OutputDebugStringA( "Failed to create vertex shader" );
		return false;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	m_pVertexLayout = NULL;
	hr = m_pd3dDevice->CreateInputLayout( layout, 2, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pVertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
	{	
		OutputDebugStringA( "Failed to create input layout" );
		return false;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = NULL;
	if( FAILED( D3DX11CompileFromFile(L"./data/fullQuad.fx", NULL, NULL, "PS", "ps_4_0", dwShaderFlags, 0, NULL, &pPSBlob, &pErrorBlob, NULL) ) )
	{
		if( pErrorBlob != NULL )
		{
			OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
			pErrorBlob->Release();
		}
		return false;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pPixelShader );
	pPSBlob->Release();
	if( FAILED( hr ) )
	{	
		OutputDebugStringA( "Failed to create pixel shader" );
		return false;
	}

	// Create vertex buffer
	SimpleVertex vertices[] =
	{
		{  XMFLOAT3(-1.0f,-1.0f, 0.5f ), XMFLOAT2( 0.0f, 1.0f ) },
		{  XMFLOAT3(-1.0f, 1.0f, 0.5f ), XMFLOAT2( 0.0f, 0.0f ) },
		{  XMFLOAT3( 1.0f,-1.0f, 0.5f ), XMFLOAT2( 1.0f, 1.0f ) },
		{  XMFLOAT3( 1.0f, 1.0f, 0.5f ), XMFLOAT2( 1.0f, 0.0f ) }
	};
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( SimpleVertex ) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = vertices;
	hr = m_pd3dDevice->CreateBuffer( &bd, &InitData, &m_pVertexBuffer );
	if( FAILED( hr ) )
	{	
		OutputDebugStringA( "Failed to create vertex buffer" );
		return false;
	}

	// Set vertex buffer
	UINT stride = sizeof( SimpleVertex );
	UINT offset = 0;
	m_pImmediateContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

	// Set primitive topology
	m_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	// Set the input layout
	m_pImmediateContext->IASetInputLayout( m_pVertexLayout );

	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory( &sampDesc, sizeof(sampDesc) );
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_pd3dDevice->CreateSamplerState( &sampDesc, &m_pSamplerLinear );
	if( FAILED( hr ) )
		return false;

	return true;
}

bool DXApplication::loadTexture(LPCWSTR filename, ID3D11Texture2D** texture)
{
	ID3D11Texture2D* tempTexture;
	if(SUCCEEDED(D3DX11CreateTextureFromFile(m_pd3dDevice, filename, NULL, NULL, (ID3D11Resource **)texture, NULL)))
	{
		D3DX11CreateShaderResourceViewFromFile( m_pd3dDevice, filename, NULL, NULL, &m_srcTextureView, NULL );

		D3D11_TEXTURE2D_DESC desc;
		(*texture)->GetDesc(&desc);

		// To keep it simple, we limit the textures we load to RGBA 8bits per channel
		if(desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			OutputDebugStringA( "We want to read a simple RGBA texture 8 bits per channel but the required texture has a different format." );
			return false;
		}

		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		if(m_pd3dDevice->CreateTexture2D(&desc, NULL, &tempTexture) != S_OK)
			return false;

		m_pImmediateContext->CopyResource(tempTexture, *texture);

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(m_pImmediateContext->Map(tempTexture, 0, D3D11_MAP_READ, 0, &mappedResource) != S_OK)
			return false;

		m_textureDataSize =  mappedResource.RowPitch * desc.Height;
		if(m_srcTextureData)
			delete [] m_srcTextureData;
		m_srcTextureData = new byte[m_textureDataSize];
		memcpy(m_srcTextureData, mappedResource.pData, m_textureDataSize);

		m_pImmediateContext->Unmap(tempTexture, 0);

		return true;
	}
	else
		return false;
}

bool DXApplication::createInputBuffer()
{
	if(m_srcDataGPUBuffer)
		m_srcDataGPUBuffer->Release();
	m_srcDataGPUBuffer = NULL;

	if(m_srcTextureData)
	{
		// First we create a buffer in GPU memory
		D3D11_BUFFER_DESC descGPUBuffer;
		ZeroMemory( &descGPUBuffer, sizeof(descGPUBuffer) );
		descGPUBuffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		descGPUBuffer.ByteWidth = m_textureDataSize;
		descGPUBuffer.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		descGPUBuffer.StructureByteStride = 4;	// We assume the data is in the RGBA format, 8 bits per channel

		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = m_srcTextureData;
		if(FAILED(m_pd3dDevice->CreateBuffer( &descGPUBuffer, &InitData, &m_srcDataGPUBuffer )))
			return false;

		// Now we create a view on the resource. DX11 requires you to send the data to shaders using a "shader view"
		D3D11_BUFFER_DESC descBuf;
		ZeroMemory( &descBuf, sizeof(descBuf) );
		m_srcDataGPUBuffer->GetDesc( &descBuf );

		D3D11_SHADER_RESOURCE_VIEW_DESC descView;
		ZeroMemory( &descView, sizeof(descView) );
		descView.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		descView.BufferEx.FirstElement = 0;

		descView.Format = DXGI_FORMAT_UNKNOWN;
		descView.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
		
		if(FAILED(m_pd3dDevice->CreateShaderResourceView( m_srcDataGPUBuffer, &descView, &m_srcDataGPUBufferView )))
			return false;

		return true;
	}
	else
		return false;
}

bool DXApplication::createOutputBuffer()
{
	// The compute shader will need to output to some buffer so here we create a GPU buffer for that.
	D3D11_BUFFER_DESC descGPUBuffer;
	ZeroMemory( &descGPUBuffer, sizeof(descGPUBuffer) );
	descGPUBuffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	descGPUBuffer.ByteWidth = m_textureDataSize;
	descGPUBuffer.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	descGPUBuffer.StructureByteStride = 4;	// We assume the output data is in the RGBA format, 8 bits per channel

	if(FAILED(m_pd3dDevice->CreateBuffer( &descGPUBuffer, NULL, &m_destDataGPUBuffer )))
		return false;

	// The view we need for the output is an unordered access view. This is to allow the compute shader to write anywhere in the buffer.
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory( &descBuf, sizeof(descBuf) );
	m_destDataGPUBuffer->GetDesc( &descBuf );

	D3D11_UNORDERED_ACCESS_VIEW_DESC descView;
	ZeroMemory( &descView, sizeof(descView) );
	descView.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	descView.Buffer.FirstElement = 0;

	descView.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
	descView.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride; 
		
	if(FAILED(m_pd3dDevice->CreateUnorderedAccessView( m_destDataGPUBuffer, &descView, &m_destDataGPUBufferView )))
		return false;

	return true;
}

bool DXApplication::loadComputeShader(LPCWSTR filename, ID3D11ComputeShader** computeShader)
{
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( _DEBUG )
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	LPCSTR pProfile = ( m_pd3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ) ? "cs_5_0" : "cs_4_0";

	ID3DBlob* pErrorBlob = NULL;
	ID3DBlob* pBlob = NULL;
	HRESULT hr = D3DX11CompileFromFile( filename, NULL, NULL, "CSMain", pProfile, dwShaderFlags, NULL, NULL, &pBlob, &pErrorBlob, NULL );
	if ( FAILED(hr) )
	{
		if ( pErrorBlob )
			OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
		if(pErrorBlob)
			pErrorBlob->Release();
		if(pBlob)
			pBlob->Release();

		return false;
	}
	else
	{
		hr = m_pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, computeShader );
		if(pErrorBlob)
			pErrorBlob->Release();
		if(pBlob)
			pBlob->Release();

		return hr == S_OK;
	}
}

/**
*	Get a copy of the GPU dest buffer.
*/
byte* DXApplication::getCopyOfGPUDestBuffer()
{
	ID3D11Buffer* debugbuf = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof(desc) );
	m_destDataGPUBuffer->GetDesc( &desc );

	UINT byteSize = desc.ByteWidth;

	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;

	if ( SUCCEEDED(m_pd3dDevice->CreateBuffer(&desc, NULL, &debugbuf)) )
	{
		m_pImmediateContext->CopyResource( debugbuf, m_destDataGPUBuffer );

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(m_pImmediateContext->Map(debugbuf, 0, D3D11_MAP_READ, 0, &mappedResource) != S_OK)
			return false;

		byte* outBuff = new byte[byteSize];
		memcpy(outBuff, mappedResource.pData, byteSize);

		m_pImmediateContext->Unmap(debugbuf, 0);

		debugbuf->Release();

		return outBuff;
	}
	return NULL;
}