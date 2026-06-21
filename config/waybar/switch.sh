#!/bin/bash

# Folder konfigurasi
WAYBAR_DIR="$HOME/.config/waybar"
THEMES_DIR="$WAYBAR_DIR/themes"

# 1. Cek apakah Rofi terinstall (Menggunakan syntax yang kompatibel)
if ! command -v rofi >/dev/null 2>&1; then
  notify-send "Error!! where is rofi??"
  exit 1
fi

# 2. Ambil daftar tema dan tampilkan di Rofi
CHOSEN_THEME=$(ls "$THEMES_DIR" | rofi -dmenu -config "~/.config/rofi/themes.rasi" -p "select")

# Jika user membatalkan (pencet ESC)
if [ -z "$CHOSEN_THEME" ]; then
  exit 0
fi

SELECTED_THEME_DIR="$THEMES_DIR/$CHOSEN_THEME"

# 3. Validasi file dan copy
if [ -f "$SELECTED_THEME_DIR/config.jsonc" ] && [ -f "$SELECTED_THEME_DIR/style.css" ]; then
  cp "$SELECTED_THEME_DIR/config.jsonc" "$WAYBAR_DIR/config.jsonc"
  cp "$SELECTED_THEME_DIR/style.css" "$WAYBAR_DIR/style.css"

  # 4. Restart Waybar dengan jeda (Paling Penting!)
  killall waybar

  # Tunggu sampai proses waybar lama benar-benar mati
  while pgrep -x waybar >/dev/null; do sleep 0.1; done

  # Jalankan waybar baru di background tanpa mengikat terminal
  waybar >/dev/null 2>&1 &
fi
