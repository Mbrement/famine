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

# Fonction pour configurer et démarrer le service avec launchd (macOS)
setup_launchd() {
    echo "Setting up $SERVICE_NAME with launchd (macOS)..."
    if [ ! -f "$SOURCE_DIR$PLIST_FILE" ]; then
        echo "Error: $PLIST_FILE not found!"
        exit 1
    fi
    sudo cp "$SOURCE_DIR$PLIST_FILE" /Library/LaunchDaemons/
    sudo launchctl load -w /Library/LaunchDaemons/"$PLIST_FILE"
    sudo launchctl start "$SERVICE_NAME"
    echo "$SERVICE_NAME started with launchd."
}

# Fonction pour configurer et démarrer le service avec systemd (Linux)
setup_systemd() {
    echo "Setting up $SERVICE_NAME with systemd (Linux)..."
    if [ ! -f "$SOURCE_DIR$SERVICE_FILE" ]; then
        echo "Error: $SERVICE_FILE not found!"
        exit 1
    fi
    sudo cp "$SOURCE_DIR$SERVICE_FILE" /etc/systemd/system/
    sudo systemctl daemon-reload
    sudo systemctl enable "$SERVICE_FILE"
    sudo systemctl start "$SERVICE_NAME"
    echo "$SERVICE_NAME started with systemd."
}

sudo mkdir -p /etc/webserv

sudo cp $PROG_FILE /usr/local/bin

# Détection du système d'exploitation et appel des fonctions appropriées
if [[ "$OSTYPE" == "darwin"* ]]; then
    if [ -f "/Library/LaunchDaemons/$PLIST_FILE" ]; then
        sudo launchctl unload -w /Library/LaunchDaemons/"$PLIST_FILE"
        sudo rm "/Library/LaunchDaemons/$PLIST_FILE"
    fi
    setup_launchd
elif [[ -f /etc/debian_version || -f /etc/redhat-release ]]; then
    if [ -f "/etc/systemd/system/$SERVICE_FILE" ]; then
        sudo systemctl stop "$SERVICE_NAME"
        sudo systemctl disable "$SERVICE_NAME"
        sudo rm "/etc/systemd/system/$SERVICE_FILE"
    fi
    setup_systemd
else
    echo "Unsupported OS."
    exit 1
fi