#pragma once
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <D3DX11.h>

class DXApplication {
public:
	// Ctor
	DXApplication() 
		: m_pd3dDevice(NULL)
		, m_pImmediateContext(NULL)
		, m_pSwapChain(NULL)
		, m_pRenderTargetView(NULL)
		, m_srcTexture(NULL)
		, m_srcTextureData(NULL)
		, m_srcDataGPUBuffer(NULL)
		, m_srcDataGPUBufferView(NULL)
		, m_destTexture(NULL)
		, m_destDataGPUBuffer(NULL)
		, m_destDataGPUBufferView(NULL)
		, m_computeShader(NULL)
	{}

	// Methods
	bool	initialize(HWND hwnd, int w, int h);
	bool	runComputeShader( LPCWSTR shaderFilename, int groupSizeX, int groupSizeY);
	void	update();
	void	render();
	void	release();

private:
	// Methods
	bool	loadFullScreenQuad();
	void	releaseFullScreenQuad();
	bool	loadTexture( LPCWSTR filename, ID3D11Texture2D** texture);
	bool	createInputBuffer();
	bool	createOutputBuffer();
	bool	loadComputeShader( LPCWSTR filename, ID3D11ComputeShader** computeShader);
	byte*	getCopyOfGPUDestBuffer();

	// Fields
	int							m_windowWidth;
	int							m_windowHeight;

	ID3D11Device*				m_pd3dDevice;
	ID3D11DeviceContext*		m_pImmediateContext;
	IDXGISwapChain*				m_pSwapChain;
	ID3D11RenderTargetView*		m_pRenderTargetView;

	ID3D11VertexShader*			m_pVertexShader;
	ID3D11PixelShader*			m_pPixelShader;
	ID3D11InputLayout*			m_pVertexLayout;
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11SamplerState*			m_pSamplerLinear;

	UINT						m_textureDataSize;

	ID3D11Texture2D*			m_srcTexture;
	ID3D11ShaderResourceView*	m_srcTextureView;
	byte*						m_srcTextureData;
	ID3D11Buffer*				m_srcDataGPUBuffer;
	ID3D11ShaderResourceView*	m_srcDataGPUBufferView;

	ID3D11Texture2D*			m_destTexture;
	ID3D11ShaderResourceView*	m_destTextureView;
	ID3D11Buffer*				m_destDataGPUBuffer;
	ID3D11UnorderedAccessView*	m_destDataGPUBufferView;

	ID3D11ComputeShader*		m_computeShader;
};