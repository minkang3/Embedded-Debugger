A bare metal debugger built for the Raspberry Pi Pico.

# Install for Windows
You will need [cmake](https://cmake.org/download/) installed.

First, we will install the Pico SDK and necessary tools all in one step.

Go to this link: https://www.raspberrypi.com/news/raspberry-pi-pico-windows-installer/

Scroll down and find and click "Download Windows Pico Installer"
![image](https://github.com/user-attachments/assets/4dc342c9-6fec-4c78-aeba-2268cf0a6da3)

Run the setup which will install the necessary pico sdk, compilers, and even its own version of VS Code.

After it finished installing, look for "Pico - Visual Studio Code" in your start menu and open it.
![image](https://github.com/user-attachments/assets/a52c0dec-7e39-40ea-9852-785f3deea1ac)


On the left bar, click on extensions and search for "Raspberry Pi Pico" and install it.
![image](https://github.com/user-attachments/assets/1cc2be72-abe2-49c6-8580-d361f1156447)

Clone this repo if you haven't already and place it in a convinient location.

Go back to VS Code, and press on the pico icon on the left bar, and press "Import Project".
![image](https://github.com/user-attachments/assets/7a7fcd51-f816-426a-afc3-58ccd6bcca0d)

For location, select the location of the repo, for the Pico SDK version, select the latest, and then press Import.

Once it opens, select the Pico icon once more and press "Compile Project".
![image](https://github.com/user-attachments/assets/14ca1ec1-3ace-4759-b503-28dcee61d856)

Now, plug in the Pico while holding down the white button labled "BOOTSEL". (BOOTSEL location showed below)
![image](https://github.com/user-attachments/assets/68aa3ca8-23ec-4262-aca7-775d7ef81ad9)

Then press "Run Project".

The debugger should be running on your board.

# Setup
![image](https://github.com/user-attachments/assets/b6d38f91-84c0-47f7-9c6b-427cb132d43b)
The two pins that are needed are pins 19 and 20. However, you can change this to your liking by changing the macro definitions in `inc/macros.h`. Pin 19 is `SWDIO` and pin 20 is `SWCLK`.

To debug a target board, connect pin 19 to the target's `SWDIO` and pin 20 to the target's `SWCLK`. If necessary, connect the power from pin 36 and ground from pin 38  as well.

# How to use
We can take advantage of VS Code's serial monitor. Open up the terminal bar by pressing Ctrl-` (backtick next to 1) and press "Serial Monitor".

Press "Start Monitoring" and press the terminal icon to toggle terminal mode.

Now you can start typing to the debug interface.

First initialize core debug for the board:
```
> init
```
Now, you can do anything you want. Enter `> help` to get an idea of what you can do.
