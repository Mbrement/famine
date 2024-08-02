#!/bin/bash

PROG_FILE="famine"
SERVICE_NAME="famine"
SOURCE_DIR="service/"
PLIST_FILE="famine.plist"
SERVICE_FILE="famine.service"

# Allow only root execution
if [ `id|sed -e s/uid=//g -e s/\(.*//g` -ne 0 ]; then
    echo "This script requires root privileges"
    exit 1
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
    if [ -f "/Library/LaunchDaemons/$PLIST_FILE" ]; then
        sudo launchctl unload -w /Library/LaunchDaemons/"$PLIST_FILE"
        sudo rm "/Library/LaunchDaemons/$PLIST_FILE"
    fi
elif [[ -f /etc/debian_version || -f /etc/redhat-release ]]; then
    if [ -f "/etc/systemd/system/$SERVICE_FILE" ]; then
        sudo systemctl stop "$SERVICE_NAME"
        sudo systemctl disable "$SERVICE_NAME"
        sudo rm "/etc/systemd/system/$SERVICE_FILE"
    fi
else
    echo "Unsupported OS."
    exit 1
fi

sudo rm -f /usr/local/bin/"$PROG_FILE"
