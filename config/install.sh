#!/bin/sh

chmod +x ~/hyprland-config/config/install.sh
chmod +x ~/hyprland-config/config/waybar/start.sh
chmod +x ~/hyprland-config/config/hypr/wallpaper-pick.sh

cp -r fish/ gtk-3.0/ gtk-4.0/ hypr/ kitty/ nvim/ rofi/ waybar/ wlogout/ ~/.config

if [ $? -eq 0 ]; then
    echo "Done []"
else
    echo "Meh! []"
fi
