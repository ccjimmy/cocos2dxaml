//
// DirectXPage.xaml.cpp
// DirectXPage.xaml 类的实现。
//

#include "DirectXPage.xaml.h"

#include <pch.h>
#include "AppDelegate.h"
#include "exception\CCException.h"
#include "CCCommon.h"

#include "CCDirector.h"
#include "SimpleAudioEngine.h"
#include "DirectXRender.h"

using namespace SimpleGame;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Input;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

USING_NS_CC;

DirectXRender^ m_renderer;
AppDelegate app;	

DirectXPage::DirectXPage() :
	m_isLoaded(false)
{
	InitializeComponent();
	
	m_renderer = ref new DirectXRender();
		m_renderer->Initialize(
		Window::Current->CoreWindow,
		SwapChainPanel,
		DisplayProperties::LogicalDpi
		);
	CCApplication::sharedApplication().initInstance();

	this->Loaded += ref new RoutedEventHandler(this, &DirectXPage::PageLoaded);

	Window::Current->CoreWindow->SizeChanged += 
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &DirectXPage::OnWindowSizeChanged);

	DisplayProperties::LogicalDpiChanged +=
		ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnLogicalDpiChanged);

	DisplayProperties::OrientationChanged +=
        ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnOrientationChanged);

	DisplayProperties::DisplayContentsInvalidated +=
		ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnDisplayContentsInvalidated);
	
	m_eventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &DirectXPage::OnRendering));

	m_timer = ref new BasicTimer();
}

void DirectXPage::PageLoaded(Object^ sender, RoutedEventArgs^ args)
{	
		CCLog("CCFrameworkView::+Run()");
		 // if applicationDidFinishLaunching return false, exist.
		if(!CCApplication::sharedApplication().applicationDidFinishLaunching())
			return;
		m_isLoaded = true;
}

void DirectXPage::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_renderer->UpdateForWindowSizeChange();
}

void DirectXPage::OnLogicalDpiChanged(Object^ sender)
{
	m_renderer->SetDpi(DisplayProperties::LogicalDpi);
}

void DirectXPage::OnOrientationChanged(Object^ sender)
{
	m_renderer->UpdateForWindowSizeChange();
}

void DirectXPage::OnDisplayContentsInvalidated(Object^ sender)
{
	//m_renderer->ValidateDevice();
}

void DirectXPage::OnRendering(Object^ sender, Object^ args)
{
	if(m_isLoaded)
	{
		//CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
		CCDirector::sharedDirector()->mainLoop();
		CocosDenshion::SimpleAudioEngine::sharedEngine()->render();
		// The window is not visible, so just wait for next event and respond to it.
		//CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
	}
}

void DirectXPage::OnPreviousColorPressed(Object^ sender, RoutedEventArgs^ args)
{
}

void DirectXPage::OnNextColorPressed(Object^ sender, RoutedEventArgs^ args)
{
}

void DirectXPage::SaveInternalState(IPropertySet^ state)
{
}

void DirectXPage::LoadInternalState(IPropertySet^ state)
{
}
