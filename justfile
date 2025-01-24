device := "tty.usbmodem*"
alias u := uart
alias f := flash
alias r := reboot

all:
    make
    picotool load -f debugger.uf2
    sleep 1.2
    tmux new-window screen /dev/{{device}} 115200

uart:
    tmux new-window screen /dev/{{device}} 115200

flash:
    make
    picotool load -f debugger.uf2

reboot:
    picotool reboot -f
