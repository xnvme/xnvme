# Build, link and make the example executable
gcc ../getting_started/hello.c  $(pkg-config --libs xnvme) -o hello
chmod +x hello

# Detach NVMe-devices from OS NVMe driver and bind to vfio-pci / uio-generic
sudo xnvme-driver

# Run it
sudo ./hello 0000:05:00.0
