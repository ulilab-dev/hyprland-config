#!/bin/sh

chmod +x ~/hyprland-config/config/waybar/start.sh &&
chmod +x ~/hyprland-config/config/hypr/wallpaper-pick.sh &&

cp -r fish/ hypr/ kitty/ rofi/ waybar/ wlogout/ ~/.config &

if [ $? -eq 0 ]; then
    echo "Done []"
else
    echo "Meh! []"
fi
