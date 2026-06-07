-------------------
---- AUTOSTART ----
-------------------

-- See https://wiki.hypr.land/Configuring/Basics/Autostart/

-- Autostart necessary processes (like notifications daemons, status bars, etc.)
-- Or execute your favorite apps at launch like this:

hl.on("hyprland.start", function()
  hl.exec_cmd("nm-applet")
  hl.exec_cmd("waybar")
  hl.exec_cmd("swaybg -m fill -i ~/.config/hypr/wallpapers/tree.png")
  hl.exec_cmd("/usr/lib/polkit-gnome/polkit-gnome-authentication-agent-1")

  -- clipboard
  hl.exec_cmd("wl-paste --type text --watch cliphist store")
  hl.exec_cmd("wl-clip-persist --clipboard regular --reconnect-tries 0 &")
end)
