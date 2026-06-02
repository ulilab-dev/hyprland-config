#!/bin/sh

chmod +x waybar/toggle_waybar.sh &&
chmod +x hypr/wallpaper-pick.sh &&

cp -r dunst/ fastfetch/ fish/ hypr/ kitty/ rofi/ waybar/ wlogout/ ~/.config && 
cp -r Bibata-Modern-Ice/ ~/.local/share/icons &

if [ $? -eq 0 ]; then
    echo "Done []"
else
    echo "Meh! []"
fi
