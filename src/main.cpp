#include <iostream>
#include <shlwapi.h>
#include <csignal>
#include <ctime>
#include <math.h>
#include "vXboxInterface.h"

volatile bool execute = true;
void signalHandler(int signal) {
	execute = false;
}

bool isPressed(int key) {
	return GetAsyncKeyState(key) & 0x8000;
}

bool transitionedToPressed(int key) {
	return GetAsyncKeyState(key) & 0x0001;
}

int getPrefsInt(const char* section, const char* key, int defaultValue) {
	int result = GetPrivateProfileIntA(section, key, -1, "./config.ini");
	return result == -1 ? defaultValue : result;
}

int getPrefsHex(const char* section, const char* key, int defaultValue) {
	int length = 5;
	char value[length];
	GetPrivateProfileStringA(section, key, NULL, value, length, "./config.ini");
	if (value == NULL) return defaultValue;

	int result = -1;
	StrToIntExA(value, STIF_SUPPORT_HEX, &result);
	return result == -1 ? defaultValue : result;
}

int main(void) {
	HWND desktopHandle;
	RECT desktopRect;
	POINT center;
	POINT mousePos;

	unsigned int device;
	bool paused = true;

	bool rollBtnWasPressed = false;
	bool fireBtnWasPressed = false;
	bool cameraBtnWasPressed = false;

	struct Prefs {
		int up, down, left, right, roll, fire, camera, sensitivity, delay, hotkey;
	} prefs = {
		.up = getPrefsHex("Controls", "Up", 0x57),
		.down = getPrefsHex("Controls", "Down", 0x53),
		.left = getPrefsHex("Controls", "Left", 0x41),
		.right = getPrefsHex("Controls", "Right", 0x44),
		.roll = getPrefsHex("Controls", "Roll", 0x20),
		.fire = getPrefsHex("Controls", "Fire", 0x01),
		.camera = getPrefsHex("Controls", "Camera", 0x02),
		.sensitivity = getPrefsInt("Camera", "Sensitivity", 100),
		.delay = getPrefsInt("General", "Delay", 10),
		.hotkey = getPrefsHex("General", "Hotkey", 0x73),
	};

	// Get desktop window's center
	desktopHandle = GetDesktopWindow();
	if (!GetWindowRect(desktopHandle, &desktopRect)) {
		std::cout << "Failed to get handle to desktop window. Code: " << GetLastError() << std::endl;
		return 1;
	}

	center = {.x = desktopRect.right / 2, .y = desktopRect.bottom / 2};

	// Check if ScpVBus driver is installed
	if (!isVBusExists()) {
		std::cout << "ScpVBus driver is not installed." << std::endl;
		return 1;
	}

	// Plug a device into first available slot
	for (device = 1; device < 5; device++) {
		if (PlugIn(device)) break;
		if (device == 4) {
			std::cout << "Failed to plug in device." << std::endl;
			return 1;
		}
	}

	// Register signal handler for program termination
	signal(SIGINT, signalHandler);

	// Now we are all set up
	std::cout << "Device ready. ID: " << device << std::endl;
	
	while (execute) {
		// Turn keyboard mode on/off
		if (transitionedToPressed(prefs.hotkey)) {
			paused = !paused;
			std::cout << "Keyboard mode " << (paused ? "OFF" : "ON") << std::endl;
		}
		if (paused) continue;

		// Get current key state
		bool rollBtnIsPressed = isPressed(prefs.roll);
		bool fireBtnIsPressed = isPressed(prefs.fire);
		bool cameraBtnIsPressed = isPressed(prefs.camera);

		// Movement
		SetAxisX(device, -32767 * isPressed(prefs.left) + isPressed(prefs.right) * 32767);
		SetAxisY(device, -32767 * isPressed(prefs.down) + isPressed(prefs.up) * 32767);

		// Roll button pressed/released
		if (!rollBtnWasPressed && rollBtnIsPressed) {
			SetBtnA(device, true);
			rollBtnWasPressed = true;
		} else if (rollBtnWasPressed && !rollBtnIsPressed) {
			SetBtnA(device, false);
			rollBtnWasPressed = false;
		}
		
		// Camera button pressed/released
		if (!cameraBtnWasPressed && cameraBtnIsPressed) {
			SetCursorPos(center.x, center.y);
			cameraBtnWasPressed = true;
		} else if (cameraBtnWasPressed && !cameraBtnIsPressed) {
			SetAxisRx(device, 0);
			SetAxisRy(device, 0);
			cameraBtnWasPressed = false;
		}

		// Fire button pressed/released
		if (!fireBtnWasPressed && fireBtnIsPressed) {
			SetCursorPos(center.x, center.y);
			fireBtnWasPressed = true;
			SetBtnX(device, true);
		} else if (fireBtnWasPressed && !fireBtnIsPressed) {
			SetAxisX(device, 0);
			SetAxisY(device, 0);
			fireBtnWasPressed = false;
			SetBtnX(device, false);
		}
		
		// Fire or camera button held down
		if ((fireBtnWasPressed && fireBtnIsPressed) || (cameraBtnWasPressed && cameraBtnIsPressed)) {
			GetCursorPos(&mousePos);

			// Calculate mouse distance from center of screen
			int distX = mousePos.x - center.x;
			int distY = mousePos.y - center.y;

			float dist = sqrt(distX * distX + distY * distY);
			if (dist == 0) continue;

			// If mouse is outside allowed radius, move to closest valid position
			if (dist > prefs.sensitivity) {
				int mx = center.x + (float) distX / dist * prefs.sensitivity;
				int my = center.y + (float) distY / dist * prefs.sensitivity;

				SetCursorPos(mx, my);
			}

			// Normalize mouse X, Y value to 0...1
			float normalizedX = (float) (mousePos.x - (center.x - prefs.sensitivity)) / (float) (2 * prefs.sensitivity);
			if (normalizedX < 0) normalizedX = 0;
			if (normalizedX > 1) normalizedX = 1;

			float normalizedY = (float) (mousePos.y - (center.y - prefs.sensitivity)) / (float) (2 * prefs.sensitivity);
			if (normalizedY < 0) normalizedY = 0;
			if (normalizedY > 1) normalizedY = 1;

			// Calculate joystick X, Y
			int joystickX = -32767 + normalizedX * 32767 * 2;
			int joystickY = 32767 - normalizedY * 32767 * 2;

			// Update Left / Right joystick position
			if (fireBtnIsPressed) {
				SetAxisX(device, joystickX);
				SetAxisY(device, joystickY);
			} else if (cameraBtnIsPressed) {
				SetAxisRx(device, joystickX);
				SetAxisRy(device, joystickY);
			}
		}

		Sleep(prefs.delay);
	}

	// Unplug device on exit
	if (!UnPlug(device)) {
		if (!UnPlugForce(device)) {
			std::cout << "Failed to unplug device." << std::endl;
			return 1;
		}
	}

	return 0;
}
