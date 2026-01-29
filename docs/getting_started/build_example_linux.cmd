# Build, link and make the example executable
gcc ../examples/xnvme_hello.c  $(pkg-config --libs xnvme) -o hello
chmod +x hello

# Run it
./hello /dev/nvme2n1
