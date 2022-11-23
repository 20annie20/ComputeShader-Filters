#include <windows.h>
#include <d3d11.h>
#include <D3DX11.h>
#include "DXApplication.h"

HINSTANCE		g_hInst = NULL;
HWND			g_hWnd = NULL;
int				width = 1024;
int				height = 512*2;
DXApplication	application;

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch( message )
	{
	case WM_PAINT:
		hdc = BeginPaint( hWnd, &ps );
		EndPaint( hWnd, &ps );
		break;

	case WM_KEYUP:
		if(wParam == 112)
			application.runComputeShader(L"data/Desaturate.hlsl", 32, 64);
		if(wParam == 113)
			application.runComputeShader(L"data/Sepia.hlsl", 32, 64);
		if (wParam == 114)
			application.runComputeShader(L"data/InvertColors.hlsl", 32, 64);
		if (wParam == 115)
			application.runComputeShader(L"data/GaussianBlur.hlsl", 32, 64);
		if (wParam == 116)
			application.runComputeShader(L"data/Contrast.hlsl", 32, 64);
		if (wParam == 117)
			application.runComputeShader(L"data/Pixelite.hlsl", 64, 64);
		if (wParam == 118)
			application.runComputeShader(L"data/Bloom.hlsl", 32, 64);
		if (wParam == 119)
			application.runComputeShader(L"data/Posterize.hlsl", 32, 64);
		if (wParam == 120)
			application.runComputeShader(L"data/EdgeDetect.hlsl", 32, 64);
		if (wParam == 49)
			application.runComputeShader(L"data/RGBNoise.hlsl", 32, 64);
		if (wParam == 50)
			application.runComputeShader(L"data/ChromaticAberration.hlsl", 32, 64);
		if (wParam == 51)
			application.runComputeShader(L"data/Painting.hlsl", 32, 64);
		if (wParam == 52)
			application.runComputeShader(L"data/BilateralFilter.hlsl", 32, 64);
		if (wParam == 53)
			application.runComputeShader(L"data/EdgeSNN.hlsl", 32, 64);
		if (wParam == 54)
			application.runComputeShader(L"data/Sharpen.hlsl", 32, 64);
		break;

	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;

	default:
		return DefWindowProc( hWnd, message, wParam, lParam );
	}

	return 0;
}

HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIconSm = NULL;
	if( !RegisterClassEx( &wcex ) )
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, width, height };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	g_hWnd = CreateWindow( L"WindowClass", L"Compute Shader - Filters",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL );
	if( !g_hWnd )
		return E_FAIL;

	ShowWindow( g_hWnd, nCmdShow );

	return S_OK;
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
		return 0;

	if(!application.initialize(g_hWnd, width, height))
		return 0;

	// Main message loop
	MSG msg = {0};
	while( WM_QUIT != msg.message )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			application.render();
		}
	}

	return ( int )msg.wParam;
}