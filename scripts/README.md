# Bash скрипт для отправки файлов на wigle.net

На FTP сервере отправляет все файлы с раширением csv из папки, куда сбрасываются файлы с WiFi AP, на wigle.net
Отправленные файлы перекладываются в другую папку.

Файл со скриптом необходимо скопировать, например, в /usr/local/bin, и установить бит исполнения.

Файл wigle-cron скопировать в /etc/cron.d, изменить <ftpuser> на имя пользователя в вашей системе и перезапустить crond
