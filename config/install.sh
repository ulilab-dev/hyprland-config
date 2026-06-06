#!/bin/sh

cp -r dunst/ fastfetch/ fish/ hypr/ kitty/ rofi/ waybar/ wlogout/ ~/.config && \
cp -r Simp1e/ ~/.local/share/icons 

if [ $? -eq 0 ]; then
    echo "Done []"
else
    echo "Meh! []"
fi
