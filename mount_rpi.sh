#!/usr/bin/env bash

device=$1

if [ -z "$device" ]; then
    echo "No device specified"
    exit 1
fi

if [ ! -b "$device" ]; then
    echo "Device does not exist or is not a block device"
    exit 1
fi

echo "Device: $device"

mountpoint="/mnt/pico"
sudo mkdir -p "$mountpoint"

if mountpoint -q "$mountpoint"; then
    echo "Already mounted at $mountpoint"
    exit 0
fi

if ! sudo mount -o uid=$(id -u),gid=$(id -g) "$device" "$mountpoint"; then
    echo "Mount failed"
    exit 1
fi

echo "Mounted $device at $mountpoint"