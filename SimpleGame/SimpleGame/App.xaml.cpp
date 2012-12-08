//
// App.xaml.cpp
// App 类的实现。
//

#include "pch.h"
#include "DirectXPage.xaml.h"

using namespace SimpleGame;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

/// <summary>
/// 初始化单一实例应用程序对象。这是执行的创作代码的第一行，
/// 逻辑上等同于 main() 或 WinMain()。
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
}

/// <summary>
/// 在应用程序由最终用户正常启动时进行调用。
/// 当启动应用程序以打开特定的文件或显示搜索结果等操作时，
/// 将使用其他入口点。
/// </summary>
/// <param name="args">有关启动请求和过程的详细信息。</param>
void App::OnLaunched(LaunchActivatedEventArgs^ args)
{
	m_directXPage = ref new DirectXPage();

	if (args->PreviousExecutionState == ApplicationExecutionState::Terminated)
	{
		m_directXPage->LoadInternalState(ApplicationData::Current->LocalSettings->Values);
	}

	// 将页面放在当前窗口中并确保其处于活动状态。
	Window::Current->Content = m_directXPage;
	Window::Current->Activate();
}

/// <summary>
/// 在应用程序挂起时调用。
/// </summary>
/// <param name="sender">有关事件起源的详细信息。</param>
/// <param name="args">有关挂起的事件的详细信息。</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ args)
{
	(void) sender; // 未使用的参数。
	(void) args; // 未使用的参数。

	m_directXPage->SaveInternalState(ApplicationData::Current->LocalSettings->Values);
}