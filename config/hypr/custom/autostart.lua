-------------------
---- AUTOSTART ----
-------------------

-- See https://wiki.hypr.land/Configuring/Basics/Autostart/

-- Autostart necessary processes (like notifications daemons, status bars, etc.)
-- Or execute your favorite apps at launch like this:

 hl.on("hyprland.start", function () 
     hl.exec_cmd("gsettings set org.gnome.desktop.interface cursor-theme 'Bibata-Modern-Ice'")
     hl.exec_cmd("gsettings set org.gnome.desktop.interface cursor-size 24")
     hl.exec_cmd("nm-applet")
     hl.exec_cmd("waybar")
     hl.exec_cmd("swaybg -m fill -i ~/.config/hypr/wallpapers/p3.jpg")
     hl.exec_cmd("/usr/lib/polkit-gnome/polkit-gnome-authentication-agent-1")
 end)
