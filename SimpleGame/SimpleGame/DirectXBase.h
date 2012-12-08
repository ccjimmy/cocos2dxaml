#pragma once

#include "DirectXHelper.h"

// 为 2D 和 3D 呈现初始化 DirectX API 的帮助器类。 
// 如果只使用 2D 或只使用 3D 呈现，则可能会省略此类中的某些代码。
ref class DirectXBase abstract
{
internal:
	DirectXBase();

public:
	virtual void Initialize(Windows::UI::Core::CoreWindow^ window, Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ panel, float dpi);
	virtual void HandleDeviceLost();
	virtual void CreateDeviceIndependentResources();
	virtual void CreateDeviceResources();
	virtual void SetDpi(float dpi);
	virtual void CreateWindowSizeDependentResources();
	virtual void UpdateForWindowSizeChange();
	virtual void Render() = 0;
	virtual void Present();
	virtual float ConvertDipsToPixels(float dips);
	void ValidateDevice();


protected private:
	Platform::Agile<Windows::UI::Core::CoreWindow> m_window;
	Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ m_panel;

	// DirectWrite 和 Windows 图像处理组件对象。
	Microsoft::WRL::ComPtr<IDWriteFactory1> m_dwriteFactory;
	Microsoft::WRL::ComPtr<IWICImagingFactory2> m_wicFactory;

	// DirectX 核心对象。对于 2D 和 3D 而言是必需的。
	Microsoft::WRL::ComPtr<ID3D11Device1> m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_d3dContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_d3dRenderTargetView;

	// Direct2D 呈现对象。对于 2D 而言是必需的。
	Microsoft::WRL::ComPtr<ID2D1Factory1> m_d2dFactory;
	Microsoft::WRL::ComPtr<ID2D1Device> m_d2dDevice;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_d2dContext;
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_d2dTargetBitmap;

	// Direct3D 呈现对象。对于 3D 而言是必需的。
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_d3dDepthStencilView;

	// 缓存的呈现器属性。
	D3D_FEATURE_LEVEL m_d3dFeatureLevel;
	Windows::Foundation::Size m_d3dRenderTargetSize;
	Windows::Foundation::Rect m_windowBounds;
	float m_dpi;
	Windows::Graphics::Display::DisplayOrientations m_orientation;

	// 用于显示方向的转换。
	D2D1::Matrix3x2F m_orientationTransform2D;
	DirectX::XMFLOAT4X4 m_orientationTransform3D;
};
