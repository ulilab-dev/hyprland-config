#!/bin/sh

cp -r dunst/ fastfetch/ fish/ hypr/ kitty/ networkmanager-dmenu/ rofi/ waybar/ wlogout/ ~/.config && \
cp -r wallpapers/ ~/.local/share/ && \
cp -r Simp1e/ ~/.local/share/icons && \

if [ $? -eq 0 ]; then
  echo "Done []"
else
  echo "Meh! []"
fi
