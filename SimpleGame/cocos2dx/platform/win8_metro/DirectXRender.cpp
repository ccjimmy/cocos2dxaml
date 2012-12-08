/****************************************************************************
Copyright (c) 2012 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "DirectXRender.h" 
#include "DXGI.h"
#include "exception\CCException.h"
#include "CCEGLView.h"
#include "CCApplication.h"
#include <windows.ui.xaml.media.dxinterop.h>
#include <math.h>

using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Microsoft::WRL;
using namespace D2D1;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace DirectX;

USING_NS_CC;

static CCPoint getCCPointFromScreen(Point point)
{
	CCSize viewSize = CCEGLView::sharedOpenGLView().getSize();

	CCPoint ccPoint;
	ccPoint.x = ceilf(point.X);
	ccPoint.y = ceilf(point.Y);

	return ccPoint;
}

static DirectXRender^ s_pDXRender;

// Constructor.
DirectXRender::DirectXRender()
    : m_dpi(-1.0f)
{
    s_pDXRender = this;
}


// Initialize the Direct3D resources required to run.
void DirectXRender::Initialize(CoreWindow^ window, SwapChainBackgroundPanel^ panel, float dpi)
{
    m_window = window;
	m_panel = panel;
	m_textPainter = ref new DXTextPainter();


    window->SizeChanged += 
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &DirectXRender::OnWindowSizeChanged);

    window->PointerPressed += 
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &DirectXRender::OnPointerPressed);

    window->PointerReleased +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &DirectXRender::OnPointerReleased);

    window->PointerMoved +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &DirectXRender::OnPointerMoved);

    window->CharacterReceived += 
        ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(this, &DirectXRender::OnCharacterReceived);

    CreateDeviceIndependentResources();
    CreateDeviceResources();
    SetDpi(dpi);
    SetRasterState();
}

// These are the resources required independent of hardware.
void DirectXRender::CreateDeviceIndependentResources()
{
	D2D1_FACTORY_OPTIONS options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	 // 如果项目处于调试生成阶段，请通过 SDK 层启用 Direct2D 调试。
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	DX::ThrowIfFailed(
		D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory1),
			&options,
			&m_d2dFactory
			)
		);

	DX::ThrowIfFailed(
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			&m_dwriteFactory
			)
		);

	DX::ThrowIfFailed(
		CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wicFactory)
			)
		);
}

// These are the resources that depend on the device.
void DirectXRender::CreateDeviceResources()
{
	// 此标志为与 API 默认设置具有不同颜色渠道顺序的图面
	// 添加支持。要与 Direct2D 兼容，必须满足此要求。
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	ComPtr<IDXGIDevice> dxgiDevice;

#if defined(_DEBUG)
	// 如果项目处于调试生成过程中，请通过带有此标志的 SDK 层启用调试。
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// 此数组定义此应用程序将支持的 DirectX 硬件功能级别组。
	// 请注意，应保留顺序。
	// 请不要忘记在应用程序的说明中声明其所需的
	// 最低功能级别。除非另行说明，否则假定所有应用程序均支持 9.1。
	D3D_FEATURE_LEVEL featureLevels[] = 
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// 创建 Direct3D 11 API 设备对象和对应的上下文。
	ComPtr<ID3D11Device> d3dDevice;
	ComPtr<ID3D11DeviceContext> d3dContext;
	DX::ThrowIfFailed(
		D3D11CreateDevice(
			nullptr, // 指定 nullptr 以使用默认适配器。
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			creationFlags, // 设置调试和 Direct2D 兼容性标志。
			featureLevels, // 此应用程序可以支持的功能级别的列表。
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION, // 对于 Windows 应用商店应用程序，始终将此值设置为 D3D11_SDK_VERSION。
			&d3dDevice, // 返回创建的 Direct3D 设备。
			&m_featureLevel, // 返回所创建设备的功能级别。
			&d3dContext // 返回设备的即时上下文。
			)
		);

	// 获取 Direct3D 11.1 API 设备和上下文接口。
	DX::ThrowIfFailed(
		d3dDevice.As(&m_d3dDevice)
		);

	DX::ThrowIfFailed(
		d3dContext.As(&m_d3dContext)
		);

	// 获取 Direct3D 设备的基础 DXGI 设备。
	DX::ThrowIfFailed(
		m_d3dDevice.As(&dxgiDevice)
		);

	// 创建 Direct2D 设备对象和对应的上下文。
	DX::ThrowIfFailed(
		m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
		);

	DX::ThrowIfFailed(
		m_d2dDevice->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			&m_d2dContext
			)
		);

	// Release the swap chain (if it exists) as it will be incompatible with
    // the new device.
    m_swapChain = nullptr;

	//Init textPainter
	m_textPainter->Initialize(
		m_d2dContext.Get(),
		m_wicFactory.Get(),
		m_dwriteFactory.Get()
		);
}

// Helps track the DPI in the helper class.
// This is called in the dpiChanged event handler in the view class.
void DirectXRender::SetDpi(float dpi)
{
    if (dpi != m_dpi)
    {
        // Save the DPI of this display in our class.
        m_dpi = dpi;
        
        // Update Direct2D's stored DPI.
        m_d2dContext->SetDpi(m_dpi, m_dpi);

        // Often a DPI change implies a window size change. In some cases Windows will issues
        // both a size changed event and a DPI changed event. In this case, the resulting bounds 
        // will not change, and the window resize code will only be executed once.
        UpdateForWindowSizeChange();
    }
}

// This routine is called in the event handler for the view SizeChanged event.
void DirectXRender::UpdateForWindowSizeChange()
{
	   // Only handle window size changed if there is no pending DPI change.
    if (m_dpi != DisplayProperties::LogicalDpi)
    {
        return;
    }

    if (m_window->Bounds.Width  != m_windowBounds.Width ||
        m_window->Bounds.Height != m_windowBounds.Height)
    {
		ID3D11RenderTargetView* nullViews[] = {nullptr};
		m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
        m_d2dContext->SetTarget(nullptr);
        m_d2dTargetBitmap = nullptr;
        m_renderTargetView = nullptr;
        m_depthStencilView = nullptr;
        CreateWindowSizeDependentResources();
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void DirectXRender::CreateWindowSizeDependentResources()
{
	// 存储窗口绑定，以便下次我们获取 SizeChanged 事件时，可避免
	// 在大小相同的情况下重新生成所有内容。
	m_windowBounds = m_window->Bounds;

	// 计算必要的交换链并呈现目标大小(以像素为单位)。
	float windowWidth = ConvertDipsToPixels(m_windowBounds.Width);
	float windowHeight = ConvertDipsToPixels(m_windowBounds.Height);

	// 交换链的宽度和高度必须基于窗口的
	// 横向宽度和高度。如果窗口在纵向
	// 方向，则必须使尺寸反转。
	m_orientation = DisplayProperties::CurrentOrientation;
	bool swapDimensions =
		m_orientation == DisplayOrientations::Portrait ||
		m_orientation == DisplayOrientations::PortraitFlipped;
	m_renderTargetSize.Width = swapDimensions ? windowHeight : windowWidth;
	m_renderTargetSize.Height = swapDimensions ? windowWidth : windowHeight;

	if (m_swapChain != nullptr)
	{
		// 如果交换链已存在，请调整其大小。
		HRESULT hr = m_swapChain->ResizeBuffers(
			2, // 双缓冲交换链。
			static_cast<UINT>(m_renderTargetSize.Width),
			static_cast<UINT>(m_renderTargetSize.Height),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
			);

		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			Initialize(m_window.Get(), m_panel, m_dpi);
			return;
		}
		else
		{
			DX::ThrowIfFailed(hr);
		}
	}
	else
	{
		// 否则，使用与现有 Direct3D 设备相同的适配器新建一个。
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
		swapChainDesc.Width = static_cast<UINT>(m_renderTargetSize.Width); // 匹配窗口的大小。
		swapChainDesc.Height = static_cast<UINT>(m_renderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 这是最常用的交换链格式。
		swapChainDesc.Stereo = false; 
		swapChainDesc.SampleDesc.Count = 1; // 请不要使用多采样。
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2; // 使用双缓冲最大程度地减小延迟。
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // 所有 Windows 应用商店应用程序都必须使用此 SwapEffect。
		swapChainDesc.Flags = 0;

		ComPtr<IDXGIDevice1> dxgiDevice;
		DX::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter)
			);

		ComPtr<IDXGIFactory2> dxgiFactory;
		DX::ThrowIfFailed(
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
			);

		DX::ThrowIfFailed(
			dxgiFactory->CreateSwapChainForComposition(
				m_d3dDevice.Get(),
				&swapChainDesc,
				nullptr,
				&m_swapChain
				)
			);

		// 将新交换链与 SwapChainBackgroundPanel 元素关联。
		ComPtr<ISwapChainBackgroundPanelNative> panelNative;
		DX::ThrowIfFailed(
			reinterpret_cast<IUnknown*>(m_panel)->QueryInterface(IID_PPV_ARGS(&panelNative))
			);
		
		DX::ThrowIfFailed(
			panelNative->SetSwapChain(m_swapChain.Get())
			);

		// 确保 DXGI 不会一次对多个帧排队。这样既可以减小延迟，
		// 又可以确保应用程序将只在每个 VSync 之后呈现，从而最大程度地降低功率消耗。
		DX::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1)
			);
	}

	// Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
        );

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(
        m_d3dDevice->CreateRenderTargetView(
            backBuffer.Get(),
            nullptr,
            &m_renderTargetView
            )
        );

    // Cache the rendertarget dimensions in our helper class for convenient use.
    D3D11_TEXTURE2D_DESC backBufferDesc = {0};
    backBuffer->GetDesc(&backBufferDesc);
    m_renderTargetSize.Width  = static_cast<float>(backBufferDesc.Width);
    m_renderTargetSize.Height = static_cast<float>(backBufferDesc.Height);

    // Create a descriptor for the depth/stencil buffer.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT, 
        backBufferDesc.Width,
        backBufferDesc.Height,
        1,
        1,
        D3D11_BIND_DEPTH_STENCIL
        );

    // Allocate a 2-D surface as the depth/stencil buffer.
    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(
        m_d3dDevice->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            &depthStencil
            )
        );

    // Create a DepthStencil view on this surface to use on bind.
    DX::ThrowIfFailed(
        m_d3dDevice->CreateDepthStencilView(
            depthStencil.Get(),
            &CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D),
            &m_depthStencilView
            )
        );

    // Create a viewport descriptor of the full window size.
    CD3D11_VIEWPORT viewport(
        0.0f,
        0.0f,
        static_cast<float>(backBufferDesc.Width),
        static_cast<float>(backBufferDesc.Height)
        );

    // Set the current viewport using the descriptor.
    m_d3dContext->RSSetViewports(1, &viewport);

    // Now we set up the Direct2D render target bitmap linked to the swapchain. 
    // Whenever we render to this bitmap, it will be directly rendered to the 
    // swapchain associated with the window.
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = 
        BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            m_dpi,
            m_dpi
            );

    // Direct2D needs the dxgi version of the backbuffer surface pointer.
    ComPtr<IDXGISurface> dxgiBackBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
        );

    // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
    DX::ThrowIfFailed(
        m_d2dContext->CreateBitmapFromDxgiSurface(
            dxgiBackBuffer.Get(),
            &bitmapProperties,
            &m_d2dTargetBitmap
            )
        );

    // So now we can set the Direct2D render target.
    m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());

    // Set D2D text anti-alias mode to Grayscale to ensure proper rendering of text on intermediate surfaces.
    m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}

//void DirectXRender::CreateWindowSizeDependentResources()
//{
//    // Store the window bounds so the next time we get a SizeChanged event we can
//    // avoid rebuilding everything if the size is identical.
//    m_windowBounds = m_window->Bounds;
//
//    // If the swap chain already exists, resize it.
//    if(m_swapChain != nullptr)
//    {
//        DX::ThrowIfFailed(
//            m_swapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0)
//            );
//    }
//    // Otherwise, create a new one.
//    else
//    {
//        // Allocate a descriptor.
//        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
//        swapChainDesc.Width = 0;                                     // use automatic sizing
//        swapChainDesc.Height = 0;
//        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;           // this is the most common swapchain format
//        swapChainDesc.Stereo = false; 
//        swapChainDesc.SampleDesc.Count = 1;                          // don't use multi-sampling
//        swapChainDesc.SampleDesc.Quality = 0;
//        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//        swapChainDesc.BufferCount = 2;                               // use double buffering to enable flip
//        swapChainDesc.Scaling = DXGI_SCALING_NONE;
//        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // all Metro style apps must use this SwapEffect
//        swapChainDesc.Flags = 0;
//
//        // Once the desired swap chain description is configured, it must be created on the same adapter as our D3D Device
//
//        // First, retrieve the underlying DXGI Device from the D3D Device.
//        ComPtr<IDXGIDevice1>  dxgiDevice;
//        DX::ThrowIfFailed(
//            m_d3dDevice.As(&dxgiDevice)
//            );
//
//        // Identify the physical adapter (GPU or card) this device is running on.
//        ComPtr<IDXGIAdapter> dxgiAdapter;
//        DX::ThrowIfFailed(
//            dxgiDevice->GetAdapter(&dxgiAdapter)
//            );
//
//        // And obtain the factory object that created it.
//        ComPtr<IDXGIFactory2> dxgiFactory;
//        DX::ThrowIfFailed(
//            dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
//            );
//
//		DX::ThrowIfFailed(
//			dxgiFactory->CreateSwapChainForComposition(
//				m_d3dDevice.Get(),
//				&swapChainDesc,
//				nullptr,
//				&m_swapChain
//				)
//			);
//
//		// 将新交换链与 SwapChainBackgroundPanel 元素关联。
//		ComPtr<ISwapChainBackgroundPanelNative> panelNative;
//		DX::ThrowIfFailed(
//			reinterpret_cast<IUnknown*>(m_panel)->QueryInterface(IID_PPV_ARGS(&panelNative))
//			);
//		
//		DX::ThrowIfFailed(
//			panelNative->SetSwapChain(m_swapChain.Get())
//			);
//
//        // Ensure that DXGI does not queue more than one frame at a time. This both reduces 
//        // latency and ensures that the application will only render after each VSync, minimizing 
//        // power consumption.
//        DX::ThrowIfFailed(
//            dxgiDevice->SetMaximumFrameLatency(1)
//            );
//
//    }
//
//    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
//    ComPtr<ID3D11Texture2D> backBuffer;
//    DX::ThrowIfFailed(
//        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
//        );
//
//    // Create a view interface on the rendertarget to use on bind.
//    DX::ThrowIfFailed(
//        m_d3dDevice->CreateRenderTargetView(
//            backBuffer.Get(),
//            nullptr,
//            &m_renderTargetView
//            )
//        );
//
//    // Cache the rendertarget dimensions in our helper class for convenient use.
//    D3D11_TEXTURE2D_DESC backBufferDesc = {0};
//    backBuffer->GetDesc(&backBufferDesc);
//    m_renderTargetSize.Width  = static_cast<float>(backBufferDesc.Width);
//    m_renderTargetSize.Height = static_cast<float>(backBufferDesc.Height);
//
//    // Create a descriptor for the depth/stencil buffer.
//    CD3D11_TEXTURE2D_DESC depthStencilDesc(
//        DXGI_FORMAT_D24_UNORM_S8_UINT, 
//        backBufferDesc.Width,
//        backBufferDesc.Height,
//        1,
//        1,
//        D3D11_BIND_DEPTH_STENCIL
//        );
//
//    // Allocate a 2-D surface as the depth/stencil buffer.
//    ComPtr<ID3D11Texture2D> depthStencil;
//    DX::ThrowIfFailed(
//        m_d3dDevice->CreateTexture2D(
//            &depthStencilDesc,
//            nullptr,
//            &depthStencil
//            )
//        );
//
//    // Create a DepthStencil view on this surface to use on bind.
//    DX::ThrowIfFailed(
//        m_d3dDevice->CreateDepthStencilView(
//            depthStencil.Get(),
//            &CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D),
//            &m_depthStencilView
//            )
//        );
//
//    // Create a viewport descriptor of the full window size.
//    CD3D11_VIEWPORT viewport(
//        0.0f,
//        0.0f,
//        static_cast<float>(backBufferDesc.Width),
//        static_cast<float>(backBufferDesc.Height)
//        );
//
//    // Set the current viewport using the descriptor.
//    m_d3dContext->RSSetViewports(1, &viewport);
//
//    // Now we set up the Direct2D render target bitmap linked to the swapchain. 
//    // Whenever we render to this bitmap, it will be directly rendered to the 
//    // swapchain associated with the window.
//    D2D1_BITMAP_PROPERTIES1 bitmapProperties = 
//        BitmapProperties1(
//            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
//            PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
//            m_dpi,
//            m_dpi
//            );
//
//    // Direct2D needs the dxgi version of the backbuffer surface pointer.
//    ComPtr<IDXGISurface> dxgiBackBuffer;
//    DX::ThrowIfFailed(
//        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
//        );
//
//    // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
//    DX::ThrowIfFailed(
//        m_d2dContext->CreateBitmapFromDxgiSurface(
//            dxgiBackBuffer.Get(),
//            &bitmapProperties,
//            &m_d2dTargetBitmap
//            )
//        );
//
//    // So now we can set the Direct2D render target.
//    m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());
//
//    // Set D2D text anti-alias mode to Grayscale to ensure proper rendering of text on intermediate surfaces.
//    m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
//}

void DirectXRender::Render()
{
    //float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    //m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), color);
    //m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

// Method to deliver the final image to the display.
void DirectXRender::Present()
{
	// The application may optionally specify "dirty" or "scroll" rects to improve efficiency
    // in certain scenarios.  In this sample, however, we do not utilize those features.
    DXGI_PRESENT_PARAMETERS parameters = {0};
    parameters.DirtyRectsCount = 0;
    parameters.pDirtyRects = nullptr;
    parameters.pScrollRect = nullptr;
    parameters.pScrollOffset = nullptr;
    
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present1(1, 0, &parameters);

    // If the device was removed either by a disconnect or a driver upgrade, we 
    // must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
		Initialize(m_window.Get(), m_panel, m_dpi);
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

void DirectXRender::SetBackBufferRenderTarget()
{
	m_d3dContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView.Get());
}


DirectXRender^ DirectXRender::SharedDXRender()
{
    return s_pDXRender;
}

bool DirectXRender::SetRasterState()
{
	ID3D11RasterizerState* rasterState;
	D3D11_RASTERIZER_DESC rasterDesc;
	ZeroMemory(&rasterDesc,sizeof(D3D11_RASTERIZER_DESC));
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.AntialiasedLineEnable = TRUE;
	rasterDesc.FrontCounterClockwise = TRUE;
	rasterDesc.MultisampleEnable = FALSE;
	rasterDesc.ScissorEnable = FALSE;

	// Create the rasterizer state from the description we just filled out.
	if(FAILED(m_d3dDevice->CreateRasterizerState(&rasterDesc, &rasterState)))
	{
		return FALSE;
	}
	m_d3dContext->RSSetState(rasterState);
	if(rasterState)
	{
		rasterState->Release();
		rasterState = 0;
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Event callback functions
////////////////////////////////////////////////////////////////////////////////

void DirectXRender::OnWindowClosed(
    _In_ CoreWindow^ sender,
    _In_ CoreWindowEventArgs^ args
    )
{
    m_windowClosed = true;
}

void DirectXRender::OnWindowVisibilityChanged(
        _In_ Windows::UI::Core::CoreWindow^ sender,
        _In_ Windows::UI::Core::VisibilityChangedEventArgs^ args
        )
{
	//m_windowVisible = args->Visible;
 //   if (m_windowVisible)
 //   {
 //       CCApplication::sharedApplication().applicationWillEnterForeground();
 //   } 
 //   else
 //   {
 //       CCApplication::sharedApplication().applicationDidEnterBackground();
 //   }
}

void DirectXRender::OnWindowSizeChanged(
    _In_ CoreWindow^ sender,
    _In_ WindowSizeChangedEventArgs^ args
    )
{
    UpdateForWindowSizeChange();
    cocos2d::CCEGLView::sharedOpenGLView().OnWindowSizeChanged();
}

void DirectXRender::OnPointerPressed(
    _In_ Windows::UI::Core::CoreWindow^ sender,
    _In_ Windows::UI::Core::PointerEventArgs^ args
    )
{
	CCPoint point = getCCPointFromScreen(args->CurrentPoint->Position);
    cocos2d::CCEGLView::sharedOpenGLView().OnPointerPressed(args->CurrentPoint->PointerId, point);
}

void DirectXRender::OnPointerReleased(
    _In_ Windows::UI::Core::CoreWindow^ sender,
    _In_ Windows::UI::Core::PointerEventArgs^ args
    )
{
	CCPoint point = getCCPointFromScreen(args->CurrentPoint->Position);
	cocos2d::CCEGLView::sharedOpenGLView().OnPointerReleased(args->CurrentPoint->PointerId, point);
}

void DirectXRender::OnPointerMoved(
    _In_ Windows::UI::Core::CoreWindow^ sender,
    _In_ Windows::UI::Core::PointerEventArgs^ args
    )
{
	CCPoint point = getCCPointFromScreen(args->CurrentPoint->Position);
    cocos2d::CCEGLView::sharedOpenGLView().OnPointerMoved(args->CurrentPoint->PointerId, point);
}

void DirectXRender::OnCharacterReceived(
    _In_ Windows::UI::Core::CoreWindow^ sender,
    _In_ Windows::UI::Core::CharacterReceivedEventArgs^ args
    )
{
    cocos2d::CCEGLView::sharedOpenGLView().OnCharacterReceived(args->KeyCode);
}

// 用于将以与设备无关的像素(DIP)表示的长度转换为以物理像素表示的长度的方法。
float DirectXRender::ConvertDipsToPixels(float dips)
{
	static const float dipsPerInch = 96.0f;
	return floor(dips * m_dpi / dipsPerInch + 0.5f); // 舍入到最接近的整数。
}
