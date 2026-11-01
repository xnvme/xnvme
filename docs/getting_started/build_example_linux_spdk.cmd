# Build, link and make the example executable
gcc ../examples/xnvme_hello.c  $(pkg-config --libs xnvme) -o hello
chmod +x hello

# Reserve hugepages, and move the controller off the kernel NVMe driver
sudo hugepages setup --count 1024
sudo xnvme driver --be spdk 0000:05:00.0

# Run it
sudo ./hello 0000:05:00.0
