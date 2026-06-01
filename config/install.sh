#!/bin/sh

chmod +x ~/hyprland-config/config/waybar/toggle_waybar.sh &&
chmod +x ~/hyprland-config/config/hypr/wallpaper-pick.sh &&

cp -r fastfetch/ fish/ hypr/ kitty/ rofi/ waybar/ wlogout/ ~/.config && 
cp -r Bibata-Modern-Ice/ ~/.local/share/icons &

if [ $? -eq 0 ]; then
    echo "Done []"
else
    echo "Meh! []"
fi
