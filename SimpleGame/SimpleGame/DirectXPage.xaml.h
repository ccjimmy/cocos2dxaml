//
// BlankPage.xaml.h
// BlankPage.xaml 类的声明。
//



#pragma once

#include "DirectXPage.g.h"
#include "BasicTimer.h"

namespace SimpleGame
{
	/// <summary>
	/// 一个可自行使用的 DirectX 页。请注意，不能在框架中使用该页。
	/// </summary>
    [Windows::Foundation::Metadata::WebHostHidden]
	public ref class DirectXPage sealed
	{
	public:
		DirectXPage();

		void OnPreviousColorPressed(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnNextColorPressed(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		void SaveInternalState(Windows::Foundation::Collections::IPropertySet^ state);
		void LoadInternalState(Windows::Foundation::Collections::IPropertySet^ state);

	private:
		void OnPointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args);
		void OnPointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args);
		void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
		void OnLogicalDpiChanged(Platform::Object^ sender);
		void OnOrientationChanged(Platform::Object^ sender);
		void OnDisplayContentsInvalidated(Platform::Object^ sender);
		void OnRendering(Object^ sender, Object^ args);
		void PageLoaded(Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);

		Windows::Foundation::EventRegistrationToken m_eventToken;

		//DirectXRender^ m_renderer;
		bool m_isLoaded;

		Windows::Foundation::Point m_lastPoint;
		
		BasicTimer^ m_timer;
	};
}
