# Build, link and make the example executable
sh "gcc ..\getting_started\hello.c -I..\..\toolbox\third-party\windows $(pkg-config --libs xnvme) -o hello"

# Run it
.\hello.exe .\PhysicalDrive1