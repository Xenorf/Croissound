# Croissound

Croissound is a fun Windows utility that changes the system's sound with custom provided `WAV` files. Inspired by [this reddit post](https://www.reddit.com/r/goodanimemes/comments/iczgmu/yamete_kudasai_usbsan/?rdt=64372)

## Features

* Changes Windows system sound by modifying registry values under `HKEY_CURRENT_USER\AppEvents\Schemes`.
* Simple and quick operation just provide a folder containing `WAV` files and it will automatically apply one of them as the system sound.

## Usage

1. Drop your custom `WAV` file in the `Croissound` subdirectory in the executable path.
2. Rename the files with used system sound names.
3. Run the **Croissound.exe** program.

Each `WAV` file should be named like a system sound, here are some examples:

* DeviceConnect
* DeviceDisconnect
* WindowsLogon

## Improvements (Suggested Features)

### Restore Function

Implement a restore function to revert the sound back to the default system sound.

### Shuffle Function

A function shuffling the existing system sound. Resulting in using the critical battery sound for message notification for example.

### Argument Handling

The program can be extended to accept arguments for more customization (e.g., choose specific sound, directory path).

## License

This project is licensed under the GNU General Public License v3.0.
