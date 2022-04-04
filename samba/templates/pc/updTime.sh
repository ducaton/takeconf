#!/usr/bin/sh

# Шаблон изменяет частоту запуска systemd таймера

path=/etc/systemd/system/takeconf.timer

if grep -Fq "OnUnitActiveSec=" $path; then
	sed 's/.*OnUnitActiveSec=.*/OnUnitActiveSec='"$1"'/g' -i $path || exit 1
else
	sed 's/\[Timer\]/\[Timer\]\nOnUnitActiveSec='"$1"'/g' -i $path || exit 1
fi

systemctl daemon-reload || exit 1