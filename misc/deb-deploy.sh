#!/bin/bash
ln=/usr/bin/ln
chmod=/usr/bin/chmod
chown=/usr/bin/chown
apt=/usr/bin/apt-get
nginx=/usr/sbin/nginx

$apt update && $apt upgrade && $apt install -y nginx

$chown www-data:www-data -R /var/repo

$ln -s /etc/nginx/sites-available/apt-repo /etc/nginx/sites-enabled/apt-repo
$nginx -g "daemon off;"
