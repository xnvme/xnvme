# Build, link and make the example executable
gcc ../getting_started/hello.c  $(pkg-config --libs xnvme) -o hello
chmod +x hello

# Run it
./hello /dev/nvme0ns1
