#!/bin/bash
apt install git cmake python3 build-essential opencl-headers ocl-icd-opencl-dev zlib1g-dev libzip-dev libssl-dev python-is-python3 python3.11-venv

# Exit on error
set -e

echo "--- Updating Package Lists ---"
sudo apt update

echo "--- Installing Kernel Headers and Build Tools ---"
# linux-headers are required for the NVIDIA driver to build properly
sudo apt install -y linux-headers-$(uname -r) build-essential dkms git cmake python3

echo "--- Installing NVIDIA Drivers ---"
# We choose the 595-open driver which is recommended for the RTX 4090
sudo apt install -y nvidia-driver-595-open

echo "--- Installing OpenCL and Compression Libraries ---"
# Essential for compiling KataGo/KataGomo
sudo apt install -y opencl-headers ocl-icd-opencl-dev zlib1g-dev libzip-dev libssl-dev

echo "--- Triggering Driver Build (DKMS) ---"
# Ensures the driver is actually compiled against your current kernel
sudo dkms autoinstall

echo "--------------------------------------------------------"
echo "INSTALLATION COMPLETE"
echo "--------------------------------------------------------"
echo "IMPORTANT: You MUST reboot for the driver to load."
echo "After rebooting, run 'nvidia-smi' to verify."
echo "--------------------------------------------------------"

read -p "Reboot now? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
    sudo reboot
fi
