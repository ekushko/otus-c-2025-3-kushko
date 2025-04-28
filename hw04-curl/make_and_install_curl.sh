#!/bin/bash

# установка зависимостей
apt install psl libpsl-dev openssl libssl-dev

# создание директории установки
mkdir $HOME/custom_curl

# скачивание и установка
git clone https://github.com/curl/curl.git
cd curl
autoreconf -fi
./configure -prefix=$HOME/custom_curl --with-ssl --disable-ftp --disable-file --disable-ipfs --disable-ldap --disable-ldaps --disable-rtsp --disable-proxy --disable-dict --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-mqtt --disable-websockets
make
make install

# проверка установки
cd $HOME/custom_curl
./curl --version

# В случае успеха вывод выглядит примерно таким образом
# curl 8.14.0-DEV (x86_64-pc-linux-gnu) libcurl/8.14.0-DEV OpenSSL/3.0.2 zlib/1.2.11 libpsl/0.21.0
# Release-Date: [unreleased]
# Protocols: http https telnet
# Features: alt-svc AsynchDNS HSTS IPv6 Largefile libz NTLM PSL SSL threadsafe TLS-SRP UnixSockets
