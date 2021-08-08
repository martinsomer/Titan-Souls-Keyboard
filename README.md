# Titan Souls Keyboard
Keyboard and mouse support for Titan Souls

## Features
* Toggle keyboard mode on/off
* Configure controls
* Adjust mouse sensitivity
* Adjust delay between updates

## Usage
* Install [ScpVBus driver](https://github.com/shauleiz/ScpVBus/releases)
* Download this project's [newest release](https://github.com/martinsomer/Titan-Souls-Keyboard/releases)
* Run the executable and launch Titan Souls
* Activate keyboard mode by pressing the hotkey

## Configuration
The following settings can be adjusted in `config.ini`:
| Option | Description | Default Value |
| :---: | :---: | :---: |
| Up | Move up | W (0x57) |
| Down | Move down | S (0x53) |
| Left | Move left | A (0x41) |
| Right | Move right | D (0x44) |
| Roll | Press to roll <br> Hold to run | Space (0x20) |
| Fire | Hold to aim with mouse, release to fire <br> Hold to call arrow | Left Mouse (0x01) |
| Camera | Hold to move camera with mouse | Right Mouse (0x02) |
| Sensitivity | Lower is more sensitive | 100 |
| Delay | Number of milliseconds between updates | 10 |
| Hotkey | Toggle keyboard mode on/off | F12 (0x7B) |

## Dependencies
* [vXboxInterface](https://github.com/shauleiz/vXboxInterface)

## License
[MIT License](LICENSE.txt)
