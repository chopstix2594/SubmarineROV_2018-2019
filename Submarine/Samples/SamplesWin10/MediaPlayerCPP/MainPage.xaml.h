// Ryan Baszkowski, Brennan Lambert, Nathan Loughner
// EGR ECE 486 w/ Dr. Jenkins, Dr. Shultz, Dr. Shaw, and Dr. Wright
// Submarine Remote Operated Vehicle Project
// For client Dr. Anthony Choi
// With Dr. Choi, Dr. Fu, and Dr. Sumner as advisiors
// And with Dr. Wright as project manager
//// Submarine ROV Network Remote Controller
//// Specification/header file
// First build on 23 January 2019

//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace Submarine
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

	private:
		FFmpegInterop::FFmpegInteropMSS^ FFmpegMSS;

		void sensorData_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void controls_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void connect_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void kbdInput(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
		void kbdRelease(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
		void oscontroller_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Manualcont_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void override_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void brightness(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
		void Xboxc_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void pollPad(Object^ sender, Object^ e);
		void camfail(Platform::Object^ sender, Windows::UI::Xaml::ExceptionRoutedEventArgs^ e);
		void controlconv(Object^ sender, Object^ e);

		void sendRequest(Object^, Object^);
		void codeParse(std::wstring);
		void sensToScreen();
		void servToScreen();
		void hover();
		void approach(Object^,Object^);
	};
}