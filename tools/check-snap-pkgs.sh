#!/bin/bash
for p in qt6-svg-plugins qt6-gtk-platformtheme qt6-qpa-plugins qt6-image-formats-plugins gstreamer1.0-libav gstreamer1.0-plugins-good libqt6multimediawidgets6; do
  if apt-cache show "$p" >/dev/null 2>&1; then
    echo "OK $p"
  else
    echo "MISSING $p"
  fi
done
