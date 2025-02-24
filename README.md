A bare metal debugger built for the rasberry pi pico.

# Install for Windows
You will need [cmake](https://cmake.org/download/) installed.

Go to this link: https://www.raspberrypi.com/news/raspberry-pi-pico-windows-installer/

Scroll down and find and click "Download Windows Pico Installer"

Run the setup which will install the necessary pico sdk, compilers, and even its own version of VS Code.

After it finished installing, look for "Pico - Visual Studio Code" in your start menu and open it.

On the left bar, click on extensions and search for "Rasberry Pi Pico" and install it.

Clone this repo if you haven't already and place it in a convinient location.

Go back to VS Code, and press on the pico icon on the left bar, and press "Import Project".

For location, select the location of the repo, for the Pico SDK version, select the latest, and then press Import.

Once it opens, select the Pico icon once more and press "Compile Project".

Now, plug in the Pico while holding down the white button labled "BOOTSEL".

Then press "Run Project".

It should be installed!

# How to use
We can take advantage of VS Code's serial monitor. Open up the bottom bar by pressing Ctrl-` (backtick next to 1) and press "Serial Monitor".

Press "Start Monitoring" and press the terminal icon to toggle terminal mode.

Now you can start typing to the debug interface.
