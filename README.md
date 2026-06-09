# hyprland-config

### must install
```
rofi rofi-emoji swaybg hyprlock waybar wlogout waypaper fish kitty polkit-gnome starship hyprshot dunst power-profiles-daemon wl-clipboard wl-clip-persist cliphist 

#MediaPlayer

celluloid
```

### fix dolphin defaults apps
```
sudo pacman -S kde-cli-tools
sudo pacman -S archlinux-xdg-menu
```

### set kitty for defaults terminal on dolphin
edit `/etc/xdg/kdeglobals`

add this line
```
[General]
TerminalApplication=kitty
TerminalService=kitty.desktop
```
if u not using kitty, change that to whatever terminal u use

pliss uset `qt6ct-kde` from AUR

### Font 

```
JetBrainsMono Nerd Font
```

### don't forget use this on pacman.conf
```
# put this on Misc options

ILoveCandy
```

