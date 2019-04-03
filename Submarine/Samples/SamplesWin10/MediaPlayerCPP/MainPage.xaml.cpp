// Ryan Baszkowski, Brennan Lambert, Nathan Loughner
// ECE 486, ECE 487, MAE 487, w/ Dr. Jenkins, Dr. Shultz, Dr. Shaw, and Dr. Wright
// Submarine Remote Operated Vehicle Project
// For client Dr. Anthony Choi
// With Dr. Choi, Dr. Fu, and Dr. Sumner as advisiors
// And with Dr. Wright as project manager
//// Submarine ROV Network Remote Controller
//// Implementation file
// First build on 23 January 2019

// Ryan is the primary author of the code, comments are from him unless otherwise noted.
// More so than the submarine groups before us, we want to take into account that the project
// will be continued by future senior design groups and provide adequate documentation.

// You might question the decision to go with a Universal Windows Program application.
// I knew I definately wanted to write for Windows because I wanted access to Xinput/Windows.Gaming.Input
// due to the simplicity of interfacing with game controllers they provide.
// There are two alternatives to writing an application for Windows in Visual Studio, 
// Windows Forms and WPF. WPF is the newer standard, I think introduced in
// Vista - 7 timeframe, but I had heard  from peers in the engineering school about how unpleasant
// developing in it is. Windows Forms is rather antiquated, so I decided to give the UWP a try.
// If the entire Universal Windows Platform has died out by the time my successor is given this - Sorry!

// The language you see here is C++/CX, which is the newest language that can be referred to as Visual C++.
// It's for the UWP exclusively, and uses WinRT rather than the older .NET, though they are similar.
// While it is still C++ at its core, there are a lot of things which do not necessarily follow
// ISO C++, and a few more that do not follow C++/CLI, used in WPF and Windows Forms.
// I will attempt to point these things out and explain them in comments here.

// Modified from Microsoft's default implementation of the MainPage in C++/CX,
// available from them at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

// Default inclusions
#include "pch.h"
#include "MainPage.xaml.h"

// Stuff that my additions required the inclusion of
#include <ppltasks.h>
#include <sstream>
#include <ppl.h>

using namespace Submarine;

// This is all stuff generated by default upon project creation
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// These are namespaces are my own inclusions
using namespace Windows::Web::Http; // For network communication
using namespace Windows::Gaming::Input; // For gamepads
using namespace concurrency; // For running tasks in the background, like network communication
using namespace FFmpegInterop; // For the video feed. Getting this working almost killed me
using namespace Windows::Media::Core; // Further stuff for the camera
using namespace Windows::UI::Popups; // To yell at the user when things go wrong

// Global Variables and Objects
auto client = ref new HttpClient(); // Object which handles network communication
String^ camAddr = "192.168.1.189"; // Camera URL, this is the manufacturer's default
String^ contAddr = "192.168.1.182"; // Arduino URL, this is the static IP assigned

//// Dispatcher Timers, which loop certain functions
DispatcherTimer^ network_timer; // Network communication timer
DispatcherTimer^ gamepad_timer; // Gamepad polling timer
DispatcherTimer^ movement_timer; // Digital controls timer

//// For mouse and keyboard controls
float digital_threshold = 0.3; // Percentage of max power to use for digital inputs (mouse, keyboard)
int digital_difference; // How much to add or subtract from 1500 for ESC control, calculated from digital_threshold upon initialization

//// For gamepad controls
Gamepad^ gamepad = nullptr; // Instantiate the gamepad class so we can get the list of connected gamepads later
float analog_threshold = 0.3; // Max percentage of power to use for analog input (xinput gamepad)
float deadzone = 0.10; // Analog deadzone
int x, y, z, pitch, yaw, roll; // Desired axes movement values, applied to thrusters in poll_pad

// Thrusters are less effective backwards, ratio to compensate
float back_comp = 0.03;

// Sensor Data
String^ internalTemp;
String^ externalTemp;
String^ pressure;
String^ depth;
String^ alt;
String^ x_o;
String^ y_o;
String^ z_o;
String^ water1;
String^ water2;

// ESC Microsecond Values
int left = 1500, top = 1500, front = 1500, back = 1500, bottom = 1500, right = 1500, light = 1100;

//// These hold the user's requested controls. Each of the three input devices modifies these.
//// They are named for the key configuration, and are only for digital input methods (on-screen buttons and keyboard)
bool c_w = false, c_a = false, c_s = false, c_d = false, c_i = false, c_j = false, c_k = false,
c_l = false, c_q = false, c_e = false, c_sh = false, c_sp = false;



MainPage::MainPage()
{
	InitializeComponent(); // Given automatically by VS

	sensToScreen(); // Apply the proper formatting to the TextBlocks in the GUI at launch
	servToScreen();
	contip->Text = contAddr;
	camip->Text = camAddr;

	network_timer = ref new DispatcherTimer(); // Initialize global objects
	gamepad_timer = ref new DispatcherTimer();
	movement_timer = ref new DispatcherTimer();

	// Here's where we actually move the submarine based on the user's input (DIGITAL INPUTS ONLY)
	TimeSpan ts; // For some reason this needs it's own datatype, the span of time between timer ticks
	ts.Duration = 166667; // Set the time between timer ticks here. For some reason this is in 0.0000001 seconds, so this is 16.667 ms, or one frame at 60 Hz
	movement_timer->Interval = ts; // Set the interval
	auto doit = movement_timer->Tick += ref new EventHandler<Object^>(this, &MainPage::controlconv); // Attach conversion to the timer
	movement_timer->Start(); // Start the timer

	// Do some simple calculations we only need to do once
	digital_difference = 400 * digital_threshold;

	// Yell at the user
	auto welcome = ref new MessageDialog("Welcome to the Submarine!\n\n" +
	"Please keep a few things in mind:\n" +
	"If you wish to use a gamepad, you must attach it to the system BEFORE starting this software.\n" +
	"Gamepads must be xinput compatible. (Xbox One and Xbox 360 gamepads and compatibles)\n" +
	"Press \"Enable Gamepad\" in the top-right to use an attached gamepad for control.\n" +
	"Note that using a gamepad disables keyboard and on-screen button controls.\n\n"
	"ON-SCREEN BUTTON CONTROLS ARE STICKY:\n" +
	"Make sure to use the \"Kill all Thrusters\" button to stop thruster movement.\n\n" +
	"KEYBOARD CONTROLS:\n" +
	"W = Move Forwards (Y+)\n" +
	"S = Move Backwards (Y-)\n"
	"A = Strafe Left (X-)\n"
	"D = Strafe Right (X+)\n" +
	"O = Ascend (Z+)\n" +
	"U = Descend (Z-)\n" +
	"J = Turn Left (Yaw-)\n" +
	"L = Turn Right (Yaw+)\n" +
	"I = Point Up (Pitch+)\n" +
	"K = Point Down (Pitch-)\n"
	"E = Twist Clockwise (Roll+)\n" +
	"Q = Twist Counterclockwise (Roll-)\n\n" +
	"GAMEPAD CONTROLS:\n" +
	"Left Analog Stick = Forwards and Backwards, Strafe Left and Right (Y and X)\n" +
	"Right Analog Stick = Turn Left, Right, Up, and Down (Yaw and Pitch)\n" +
	"Bumpers = Ascend and Descend (Z)\n" +
	"Triggers = Twist (Roll)\n\n" +
	"Press \"Connect Submarine ROV\" in the top-left to get started piloting.\n" +
	"You should only need to override the IP addresses if you have changed hardware.");
	welcome->ShowAsync();
}

//////////////////////////////////////////////GUI EVENT HANDLERS/////////////////////////////////////////////

// Show and hide the sensor data upon clicking the appropriate button
// I make these early on while I was still unfamiliar with all of this.
// With what I've learned now, you could definately consolodate these functions into one by casting the "sender" as a Button^
// and checking which button was pressed. I do this with buttons handling pilot control inputs because having a function for each
// would have been frankly absurd, so figuring the right way out was necessary. I don't really have the time to go back and fix these, however.
// I originally had the sensor and ESC data seperately handled, but I realized that that's stupid
void Submarine::MainPage::sensorData_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (sensors->Visibility == Windows::UI::Xaml::Visibility::Visible) { //
		sensors->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		sensorData->Content = "Show Sensor and ESC Data";
	}
	else {
		sensors->Visibility = Windows::UI::Xaml::Visibility::Visible;
		sensorData->Content = "Hide Sensor and ESC Data";
	}
	if (servos->Visibility == Windows::UI::Xaml::Visibility::Visible) {
		servos->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	else {
		servos->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}
}

// Show and hide the on screen controls upon clicking the appropriate button
void Submarine::MainPage::controls_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (OSControls->Visibility == Windows::UI::Xaml::Visibility::Visible) {
		OSControls->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		controls->Content = "Show On-Screen Controls";
	}
	else {
		OSControls->Visibility = Windows::UI::Xaml::Visibility::Visible;
		controls->Content = "Hide On-Screen Controls";
	}
	ESCOverrides->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	manualcont->Content = "Show ESC Overrides";
}

// Connect to the Arduino and the IP Camera
void Submarine::MainPage::connect_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// Take in the user's input in the GUI as the value of the global variables for the IP addresses
	camAddr = camip->Text;
	contAddr = contip->Text;

	// Connect the Arduino and begin communication
	TimeSpan ts; // For some reason this needs it's own datatype, the span of time between timer ticks
	ts.Duration = 1000000; // Set the time between timer ticks here. For some reason this is in 0.0000001 seconds, so this is 10 ms
	network_timer->Interval = ts; // Set the interval
	auto doit = network_timer->Tick += ref new EventHandler<Object^>(this, &MainPage::sendRequest); // Attach network communication to the timer
	network_timer->Start(); // Start the timer

	// Connect the camera and display its feed
	// Make a string for the full address for the RTSP stream.
	// Annoyingly, this URL format is different for just about every model of IPCam, so this only applies to the ESCAM QH002 that we used
	// We intend for everything in this project to be upgradeable, so this is a problem if students/Dr. Choi down the line want to upgrade the camera
	// Luckily, I found some software called ONVIF that you can use to give you the stream to any IP Camera. Give it a Google.
	String^ camStream = "rtsp://" + camAddr + "/ch01.264?dev=1";
	// Make a PropertySet that we could use to tweak ffmpeg if we need or want.
	PropertySet^ options = ref new PropertySet();
	// Below are some sample options that you can set to configure RTSP streaming
	options->Insert("rtsp_flags", "prefer_tcp");
	// options->Insert("stimeout", 100000);
	camfeed->Stop();
	// THIS ABSOLUTELY MUST BE DECLARED IN THE .h FILE!! Only took me three hours to figure that out...
	FFmpegMSS = FFmpegInteropMSS::CreateFFmpegInteropMSSFromUri(camStream, true, true, options); // Despite the name, this wants a String^ not a Uri^
	if (FFmpegMSS != nullptr) {
		MediaStreamSource^ mss = FFmpegMSS->GetMediaStreamSource();
		if (mss) {
			// Pass MediaStreamSource to Media Element
			camfeed->SetMediaStreamSource(mss);
		}
		else {
			auto errorDialog = ref new MessageDialog("Camera Error 1"); errorDialog->ShowAsync(); // Handle stuff going wrong with popups
		}
	}
	else {
		auto errorDialog = ref new MessageDialog("Camera Error 2"); errorDialog->ShowAsync();
	}
}

// Give the ability to show and hide the ESC control overrides, this also collapses the normal controls
void Submarine::MainPage::Manualcont_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (ESCOverrides->Visibility == Windows::UI::Xaml::Visibility::Visible) {
		ESCOverrides->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		manualcont->Content = "Show ESC Overrides";
	}
	else {
		ESCOverrides->Visibility = Windows::UI::Xaml::Visibility::Visible;
		manualcont->Content = "Hide ESC Overrides";
	}
	OSControls->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	controls->Content = "Show On-Screen Controls";
}

// Handle sliding the brightness slider, maybe I should make all the manual ESC overrides a slider like this...
void Submarine::MainPage::brightness(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	light = ((((Slider^)sender)->Value * 8) + 1100);
	servToScreen();
}

// Handle clicking of the servo override buttons
void Submarine::MainPage::oscontroller_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (((Button^)sender)->Name == "yplus")
		c_w = true;
	if (((Button^)sender)->Name == "yminus")
		c_s = true;
	if (((Button^)sender)->Name == "xplus")
		c_d = true;
	if (((Button^)sender)->Name == "xminus")
		c_a = true;
	if (((Button^)sender)->Name == "pitchplus")
		c_i = true;;
	if (((Button^)sender)->Name == "pitchminus")
		c_k = true;
	if (((Button^)sender)->Name == "yawplus")
		c_l = true;;
	if (((Button^)sender)->Name == "yawminus")
		c_j = true;
	if (((Button^)sender)->Name == "zplus")
		c_sp = true;
	if (((Button^)sender)->Name == "zminus")
		c_sh = true;
	if (((Button^)sender)->Name == "rollplus")
		c_e = true;
	if (((Button^)sender)->Name == "rollminus")
		c_q = true;
	if (((Button^)sender)->Name == "kill" || ((Button^)sender)->Name == "killov") { // Make sure the kill button kills everything
		c_w = false;
		c_s = false;
		c_a = false;
		c_d = false;
		c_i = false;
		c_k = false;
		c_l = false;
		c_j = false;
		c_e = false;
		c_q = false;
		c_sh = false;
		c_sp = false;
	}
	servToScreen();
}

// Handle clicking of the servo override buttons
void Submarine::MainPage::override_click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (((Button^)sender)->Name == "leftplus")
		left = 1500 + digital_difference;
	if (((Button^)sender)->Name == "topplus")
		top = 1500 + digital_difference;
	if (((Button^)sender)->Name == "frontplus")
		front = 1500 + digital_difference;
	if (((Button^)sender)->Name == "backplus")
		back = 1500 + digital_difference;
	if (((Button^)sender)->Name == "bottomplus")
		bottom = 1500 + digital_difference;
	if (((Button^)sender)->Name == "rightplus")
		right = 1500 + digital_difference;
	if (((Button^)sender)->Name == "leftminus")
		left = 1500 - digital_difference;
	if (((Button^)sender)->Name == "topminus")
		top = 1500 - digital_difference;
	if (((Button^)sender)->Name == "frontminus")
		front = 1500 - digital_difference;
	if (((Button^)sender)->Name == "backminus")
		back = 1500 - digital_difference;
	if (((Button^)sender)->Name == "bottomminus")
		bottom = 1500 - digital_difference;
	if (((Button^)sender)->Name == "rightminus")
		right = 1500 - digital_difference;
	if (((Button^)sender)->Name == "kill" || ((Button^)sender)->Name == "killov") { // Make sure the kill button kills everything
		left = 1500;
		top = 1500;
		front = 1500;
		back = 1500;
		bottom = 1500;
		right = 1500;
	}
	servToScreen();
}

// Handling control inputs given through the keyboard
void Submarine::MainPage::kbdInput(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	e->Handled = true;
	if (e->Key == Windows::System::VirtualKey::W)
		c_w = true;
	if (e->Key == Windows::System::VirtualKey::A)
		c_a = true;
	if (e->Key == Windows::System::VirtualKey::S)
		c_s = true;
	if (e->Key == Windows::System::VirtualKey::D)
		c_d = true;
	if (e->Key == Windows::System::VirtualKey::I)
		c_i = true;
	if (e->Key == Windows::System::VirtualKey::J)
		c_j = true;
	if (e->Key == Windows::System::VirtualKey::K)
		c_k = true;
	if (e->Key == Windows::System::VirtualKey::L)
		c_l = true;
	if (e->Key == Windows::System::VirtualKey::Q)
		c_q = true;
	if (e->Key == Windows::System::VirtualKey::E)
		c_e = true;
	if (e->Key == Windows::System::VirtualKey::U)
		c_sh = true;
	if (e->Key == Windows::System::VirtualKey::O)
		c_sp = true;

}

// Handling letting go of the controls above
void Submarine::MainPage::kbdRelease(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	e->Handled = true;
	if (e->Key == Windows::System::VirtualKey::W)
		c_w = false;
	if (e->Key == Windows::System::VirtualKey::A)
		c_a = false;
	if (e->Key == Windows::System::VirtualKey::S)
		c_s = false;
	if (e->Key == Windows::System::VirtualKey::D)
		c_d = false;
	if (e->Key == Windows::System::VirtualKey::I)
		c_i = false;
	if (e->Key == Windows::System::VirtualKey::J)
		c_j = false;
	if (e->Key == Windows::System::VirtualKey::K)
		c_k = false;
	if (e->Key == Windows::System::VirtualKey::L)
		c_l = false;
	if (e->Key == Windows::System::VirtualKey::Q)
		c_q = false;
	if (e->Key == Windows::System::VirtualKey::E)
		c_e = false;
	if (e->Key == Windows::System::VirtualKey::U)
		c_sh = false;
	if (e->Key == Windows::System::VirtualKey::O)
		c_sp = false;
}


// Enable the gamepad when the user wants.
// In Windows::Gaming::Input, the pad must be polled, it does not generate events.
// So, when we start this polling, we need to disable the keyboard and mouse inputs.
void Submarine::MainPage::Xboxc_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (gamepad->Gamepads->Size > 0) {
		gamepad = gamepad->Gamepads->GetAt(0); // Turn our gamepad global variable into our actual gamepad
		movement_timer->Stop(); // Stop the digital controls from happening
		TimeSpan ts; // For some reason this needs it's own datatype, the span of time between timer ticks
		ts.Duration = 166667; // Poll the gamepad once per frame at 60 Hz
		//ts.Duration = 10000; // Poll the gamepad at 1000 Hz because why not
		gamepad_timer->Interval = ts; // Set the interval
		auto doit = gamepad_timer->Tick += ref new EventHandler<Object^>(this, &MainPage::pollPad); // Attach gamepad polling to the timer
		gamepad_timer->Start(); // Start the timer
		// Hide and disable the other controls because the gamepad interferes with them.
		OSControls->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		ESCOverrides->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		controls->IsEnabled = false;
		manualcont->IsEnabled = false;
	}

}

// Get the state of the gamepad, map that to our thrusters
void Submarine::MainPage::pollPad(Object^ sender, Object^ e) {
	auto state = gamepad->GetCurrentReading(); // Read the state of the gamepad
	// Left and right thrusters, which facilitate X and Yaw axes
	if (state.LeftThumbstickY > deadzone || state.LeftThumbstickY < -deadzone)
		x = 400 * analog_threshold * state.LeftThumbstickY;
	else if (deadzone > state.LeftThumbstickY > -deadzone)
		x = 0;
	if (-deadzone > state.RightThumbstickX || state.RightThumbstickX > deadzone)
		yaw = 400 * analog_threshold * state.RightThumbstickX;
	else
		yaw = 0;
	left = 1500 + x + yaw;
	right = 1500 + x - yaw;
	// Top and bottom thrusters, which facilitate Y and Roll axes
	if (state.LeftThumbstickX > deadzone || state.LeftThumbstickX < -deadzone)
		y = 400 * analog_threshold * -state.LeftThumbstickX;
	else if (deadzone > state.LeftThumbstickX > -deadzone)
		y = 0;
	if (state.LeftTrigger > deadzone|| state.RightTrigger > deadzone)
		roll = 400 * analog_threshold * (state.LeftTrigger - state.RightTrigger);
	else
		roll = 0;
	top = 1500 + y + roll;
	bottom = 1500 + y - roll;
	// Front and back thrusters, which facilitate Z and Pitch axes
	if (state.Buttons.ToString() == "LeftShoulder") // The Buttoms come in some strange bitwise-combined enumeration that I DONT
		z = 400 * analog_threshold * -1;		    // have the time to figure out... this works but is really gross
	else if (state.Buttons.ToString() == "RightShoulder")
		z = 400 * analog_threshold * 1;
	else z = 0;
	if (-deadzone > state.RightThumbstickY || state.RightThumbstickY > deadzone)
		pitch = 400 * analog_threshold * state.RightThumbstickY;
	else
		pitch = 0;
	front = 1500 + z + pitch;
	back = 1500 + z - pitch;

	if ((-deadzone < state.LeftThumbstickY < deadzone) && (-deadzone < state.LeftThumbstickX < deadzone)
		&& (-deadzone < state.RightThumbstickY < deadzone) && (-deadzone < state.RightThumbstickX < deadzone)
		&& (deadzone > state.LeftTrigger) && (deadzone > state.RightTrigger) && (state.Buttons.ToString() == "None")) { // If no input is given...
		hover(); // Employ a software control system to stabilize orientation and hover in place
	}

	// Compensate for the fact that the thrusters are less powerful spinning backwards
	if (left < 1500)
		left -= 400 * analog_threshold * back_comp;
	if (top < 1500)
		top -= 400 * analog_threshold * back_comp;
	if (front < 1500)
		front -= 400 * analog_threshold * back_comp;
	if (back < 1500)
		back -= 400 * analog_threshold * back_comp;
	if (bottom < 1500)
		bottom -= 400 * analog_threshold * back_comp;
	if (right < 1500)
		right -= 400 * analog_threshold * back_comp;
	servToScreen();
}

// Handling the MediaElement screwing up. Not necessary, but failing silently is non-ideal
void Submarine::MainPage::camfail(Platform::Object^ sender, Windows::UI::Xaml::ExceptionRoutedEventArgs^ e)
{
	auto errorDialog = ref new MessageDialog("Media Failed"); errorDialog->ShowAsync();
}

// Convert directional control inputs into thruster values
void Submarine::MainPage::controlconv(Object^ sender, Object^ e) {
	// Left and right thrusters, which facilitate Y and Yaw axes
	if (c_w) { // Y (forward and backward)
		left = 1500 + digital_difference;
		right = 1500 + digital_difference;

	}
	if (c_s) {
		left = 1500 - digital_difference;
		right = 1500 - digital_difference;

	}
	if (c_j) { // Yaw (steering left and right)
		left = 1500 - digital_difference;
		right = 1500 + digital_difference;
	}
	if (c_l) {
		left = 1500 + digital_difference;
		right = 1500 - digital_difference;
	}
	if (!(c_w || c_s || c_j || c_l)) { // Remember to turn stuff off
		left = 1500;
		right = 1500;
	}
	// Top and bottom thrusters, which facilitate X and Roll axes
	if (c_a) { // X (strafing left and right)
		top = 1500 + digital_difference;
		bottom = 1500 + digital_difference;
	}
	if (c_d) {
		top = 1500 - digital_difference;
		bottom = 1500 - digital_difference;
	}
	if (c_q) { // Roll (twisting clockwise and counterclockwise)
		top = 1500 + digital_difference;
		bottom = 1500 - digital_difference;
	}
	if (c_e) {
		top = 1500 - digital_difference;
		bottom = 1500 + digital_difference;
	}
	if (!(c_a || c_d || c_q || c_e)) { // Remember to turn stuff off
		top = 1500;
		bottom = 1500;
	}
	// Front and back thrusters, which facilitate Z and Pitch axes
	if (c_sp) { // Z (ascending and descending)
		front = 1500 + digital_difference;
		back = 1500 + digital_difference;
	}
	if (c_sh) {
		front = 1500 - digital_difference;
		back = 1500 - digital_difference;
	}
	if (c_i) { // Pitch (pointing up and down)
		front = 1500 + digital_difference;
		back = 1500 - digital_difference;
	}
	if (c_k) {
		front = 1500 - digital_difference;
		back = 1500 + digital_difference;
	}
	if (!(c_sp || c_sh || c_i || c_k)) { // Remember to turn stuff off
		front = 1500;
		back = 1500;
	}

	if (!c_w && !c_s && !c_a && !c_d && !c_i && !c_k && !c_j && !c_l && !c_q && !c_e && !c_sh && !c_sp) { // If no input is given...
		hover(); // Employ a software control system to stabilize orientation and hover in place
	}

	// Compensate for the fact that the thrusters are less powerful spinning backwards
	if (left < 1500)
		left -= 400 * digital_threshold * back_comp;
	if (top < 1500)
		top -= 400 * digital_threshold * back_comp;
	if (front < 1500)
		front -= 400 * digital_threshold * back_comp;
	if (back < 1500)
		back -= 400 * digital_threshold * back_comp;
	if (bottom < 1500)
		bottom -= 400 * digital_threshold * back_comp;
	if (right < 1500)
		right -= 400 * digital_threshold * back_comp;
	servToScreen();
}

////////////////////////////////////////////////////////////////OTHER FUNCTIONS////////////////////////////////////////////////////////////////

// Perform a HTML GET request.
// This implementation of submarine drone and controller does control communication with GET requests.
// At first I thought this would be a terrible implementation, but after creating it (and tearing my hair out over it for a while), it's grown on me.
// The computer (this program) tells the Arduino what to do, the Arduino responds to the request with the sensor data, and then controlls the ESCs accordingly
// It's something of an elegant exchange. The dispatcher timer in controls_click attaches to this function.
void Submarine::MainPage::sendRequest(Object^ sender, Object^ e) {
	// This is the formatted control code which is sent as a GET request and parsed by the Arduino
	String^ encodedReq = "http://" + contAddr + "/$" + left + "-" + top + "-" + front + "-" + back + "-" + bottom + "-" + right + "-" + light + "__opcode";
	auto uri = ref new Uri(encodedReq); // Go from string to Uri for the address of the Arduino and the command to be sent
	try { // Exception handling is absolutely necessary for network stuff
		// Sending the GET request and saving the reponse (the sensor data code)
		// Network communication is an Asynchronous operation, and figuring this out was a nightmare
		// There are a couple of ways it can store the response for you, "GetStringAsync" is what I picked and what is important in this line 
		auto talk = create_task(client->GetStringAsync(uri)); // Create the task and execute it Asynchronously
		talk.then([this](String^ sens) { // Provide the lambda of what to do, the "codeParse" function in our case
			codeParse(sens->Data()); // Parse the sensor code "->Data()" gives a std::wstring; this is necessary for parsing
		})
			.then([](task<void> t) {
			try {
				t.get();
			}
			catch (COMException^ e) {

			}
		});
	}
	catch (Exception^ ex) {
		auto errorDialog = ref new MessageDialog("Controller Communication Failure"); errorDialog->ShowAsync();
	}
}



// Get the data from the Arduino into a form this application can use
void Submarine::MainPage::codeParse(std::wstring item) {
	std::wstringstream stream(item); // On the Arduino, we did C-type parsing to decude the code sent by this app
	std::wstring token; // I tried that, and it led me on a long journey to nowhere (like a lot of other things in this project)
	std::wstring data[10]; // I eventually settled on this C++ type parsing with getline, just like I did for Dr. Digh's spellchecker
	int c = 0; // but with wstring and related stuff because of Windows
	while (std::getline(stream, token, (wchar_t)'#')) { // Jesus Christ on a barstool WHY do I have to cast the the '#' to wide when EVERYTHING else defauls to wide!?
		data[c] = token; // Have to save to an array first, doesn't work if I don't for some reason if I use if statements to go directly to the String^s
		c++;
	}
	internalTemp = ref new String(data[0].c_str()); // Go from that array to our WinRT String^ global variables
	externalTemp = ref new String(data[1].c_str()); // A conversion to c_str and calling the String^ constructor is also necessary here for God knows why
	pressure = ref new String(data[2].c_str()); // internalTemp->Data = data[0]; might also work, too lazy to try it
	depth = ref new String(data[3].c_str());
	alt = ref new String(data[4].c_str());
	x_o = ref new String(data[5].c_str());
	y_o = ref new String(data[6].c_str());
	z_o = ref new String(data[7].c_str());
	water1 = ref new String(data[8].c_str());
	water2 = ref new String(data[9].c_str());
	sensToScreen(); // Update
	servToScreen();
}

// Update the TextBlock that displays the sensor data
void Submarine::MainPage::sensToScreen() { // Getting the sensor data into the TextBlock and formatting it nicely
	sensors->Text = "Sensor Readings\n" + // VS2017 doesn't light up the fstring literals like every other IDE, just gave it a go anyways
		"Internal Temperature\t" + internalTemp + " °C\n" + // and they work as expected
		"External Temperature\t" + externalTemp + " °C\n" +
		"Pressure\t\t\t" + pressure + " mbar\n" +
		"Depth\t\t\t" + depth + " m\n" +
		"Altitude\t\t\t" + alt + " m above mean sea level\n" +
		"\nOrientation Data\n"
		"X Orientation\t\t" + x_o + "\n" +
		"Y Orientation\t\t" + y_o + "\n" +
		"Z Orientation\t\t" + z_o + "\n" +
		"\nWater Breach Sensors (DISCONNECT AND RETRIEVE IF NONZERO)\n" +
		"Front\t\t\t" + water1 + "\n" +
		"Back\t\t\t" + water2 + "\n";// + 
		//"\nRaw\n" + rawRec;
}

// Update the TextBlock that displays the ESC data
void Submarine::MainPage::servToScreen() { // Getting the ESC and headlight data into the TextBlock and formatting it nicely
	servos->Text = "ESC Data\n" +
		"Left\t" + ((left - 1500) / 4) + " %\t" + left + " microseconds\n" +
		"Top\t" + ((top - 1500) / 4) + " %\t" + top + " microseconds\n" +
		"Front\t" + ((front - 1500) / 4) + " %\t" + front + " microseconds\n" +
		"Back\t" + ((back - 1500) / 4) + " %\t" + back + " microseconds\n" +
		"Bottom\t" + ((bottom - 1500) / 4) + " %\t" + bottom + " microseconds\n" +
		"Right\t" + ((right - 1500) / 4) + " %\t" + right + " microseconds\n" +
		"\nHeadlight Brightness\n" + ((light - 1100) / 8) + " %\t" + light + " microseconds\n";
}

// A software control system which uses orientation data to make the submarine hover in place
void Submarine::MainPage::hover() {
	double x_orientation = _wtof(x_o->Data()); // Convert the strings with the orientation data from the Arduino back to doubles
	double y_orientation = _wtof(y_o->Data());
	double z_orientation = _wtof(z_o->Data());
	// WRITE THE ACTUAL CONTROL SYSTEM HERE
}
