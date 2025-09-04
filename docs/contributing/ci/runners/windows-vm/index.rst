.. _sec-ci-runners-hetzner-windowsvm:

Virtual Machines with Windows
=============================

Windows are not provided as a downloadable image, and we must build and provide our own
``qcow2`` images for running virtual machines with Windows in qemu.

Prerequisites
-------------

On the host machine, install the necessary packages:

.. code-block:: bash

  sudo apt install \
    netcat-openbsd  \
    qemu-system  \
    qemu-utils  \
    libvirt-daemon-system  \
    libvirt-clients  \
    virtinst  \
    bridge-utils  \
    ovmf  \
    libguestfs-tools  \
    dnsmasq-base \
    swtpm \
    swtpm-tools

And download a `Windows 11 Disk Image`_ to the ``/var/lib/libvirt/images`` directory.

On your development machine, install `virt-manager`_ and create a connection to the
host machine through the interface.

Creating a ``qcow2`` Image
--------------------------

Install the Virtual Machine
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create a new virtual machine on the host:

- Select the host and click the "New VM" button. On Mac, you might want to resize the
  popup that appears to properly see the options.

- Select "Local install media (ISO image or CDROM)" and click forward.

- Click "Browse" and fine the Windows 11 iso, you have downloaded. Select "Windows 11"
  as operating system and click forward.

- Don't touch the memory and CPU settings, but set the disk image to be of size 64 GiB to
  ensure the image doesn't get unnecessarily large.

- On the final page, click "Customize configuration before install" and click finish.

- For Mac, you want to remove spice-stuff:

  - Remove “spice”-related hardware: USB drivers and “spice drive”.
  - Change the display to VNC instead of spice.

- Start installation in top left corner.

Go through the Windows installation screen:

- Just select English US.

- Accept that you want to install and delete everything.

- There is no product key.

- Choose Windows 11 Pro.

- Go next until installation starts and wait for it to be done.

Windows configuration:

- Keyboard: USA and US layout, but add your preferred keyboard layout as secondary.

- Skip naming device.

- Select "Setup for work and school".

- When prompted to login, click "Sign in options" and select "Domain join instead".

  - Just select ``root`` for username, password, answers to all security questions.

- Turn off all the experiences.

- Wait for download and install (takes a while).

Provisioning of the Virtual Machine
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Look and feel (optional):

- Uninstall everything unnecessary (OneDrive, Outlook, Paint, Teams etc.).

- I usually also personalise it by changing the layout of the progress bar and start menu.

xNVMe provisioning:

- Download `windows-2022.ps1`_ from xNVMe GitHub repository.

- Open a Terminal as Administrator.

- Run:
    
.. code-block:: console

  powershell.exe -ExecutionPolicy Bypass -File C:\Users\root\Downloads\windows-2022.ps1
    
- This might fail because of ``PATH`` not being updates, so open a new tab in Terminal
  and run again.
    
.. code-block:: console

  C:\Users\root\Downloads\windows-2022.ps1
    

OpenSSH:

- Open a Terminal as Administrator and check whether OpenSSH Server is available.

.. code-block:: console

  Get-WindowsCapability -Online | Where-Object Name -like 'OpenSSH*'

  # Expected output:
  Name  : OpenSSH.Server~~~~0.0.1.0
  State : NotPresent

- If the output is empty, you need to install the capability first:

  - Download `OpenSSH-Win64.zip`_.
  - Unzip the contents to ``C:/Program Files/OpenSSH``.
  - Open a Terminal as Administrator and run the following commands (takes a while).
      
.. code-block:: console

  cd "C:\Program Files\OpenSSH"
  powershell.exe -ExecutionPolicy Bypass -File install-sshd.ps1

- Add the OpenSSH Server capability and start it as a service.

.. code-block:: console

  Add-WindowsCapability -Online -Name OpenSSH.Server
  Set-Service -Name sshd -StartupType Automatic
  Start-Service sshd
  New-NetFirewallRule -Name sshd -DisplayName 'OpenSSH Server (sshd)' `
      -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22
    
- Set the default shell to msys2:
    
.. code-block:: console

  New-ItemProperty -Path "HKLM:\SOFTWARE\OpenSSH" -Name DefaultShell `
  -Value "C:\tools\msys64\usr\bin\bash.exe" -PropertyType String -Force
    
- Change the SSH config (``C:\ProgramData\ssh\sshd_config``) to with the following keys.
  You might need to save to documents folder, remove file ending and then replace the
  original file.
    
.. code-block:: console

    PermitRootLogin yes
    PasswordAuthentication yes
    
- Restart the sshd service.
    
.. code-block:: console

    Restart-Service sshd
    

VirtIO drivers:

- Find the latest stable `VirtIO`_ driver as a disk image, probably called something 
  like ``virtio-win.iso``.

- Open the downloaded ISO, and run the ``virtio-win-guest-tools.exe`` installer.

- Click next through it all to install it.

Enable WMIC:

- Open a Terminal as Administrator and run the following command (takes a while).
    
.. code-block:: console

  Add-WindowsCapability -Online -Name WMIC

Check
~~~~~

Test whether the SSH server has been configured correctly:

- Inside the Windows VM, run ``ipconfig`` to find the IPv4 Address.

- On the host machine, run ``ssh root@<ipv4>`` with password ``root`` to see that you can
  ssh into the machine and it starts a msys2 shell.

- Close down the VM through the GUI and close virt-manager.

You can also follow the guide below to test that you can start a QEMU guest manually with the qcow2 image.

Upload the qcow image to the Hetzner storage box.

.. code-block:: bash

  scp /var/lib/libvirt/images/<qcow image> <storage-box url>:~/win11.qcow2


Starting QEMU with qcow2 image
------------------------------

Assuming you want to run qemu as user ``ghr`` with home directory ``/home/ghr``. This
might be necessary to setup as ``root``.

Setup home directory
~~~~~~~~~~~~~~~~~~~~

Create subdirectories:

.. code-block:: bash

  mkdir -p /home/ghr/system_imaging
  cd /home/ghr/system_imaging
  mkdir -p disk fd iso


Get the qcow2 image:

- If created on this machine, copy from `libvirt` directory:
    
.. code-block:: bash
    
  cp /var/lib/libvirt/images/<qcow image> /home/ghr/system_imaging/disk
    
- If not, copy from Hetzner storage box:
    
.. code-block:: bash

  scp <storage-box url>:~/win11.qcow2 /home/ghr/system_imaging/disk
    

Get drivers:

- Copy the Windows BIOS drivers to the `fd` directory
    
.. code-block:: bash

  cp /usr/share/OVMF/OVMF_CODE_4M.ms.fd /home/ghr/system_imaging/fd/win11_VARS.fd
    
- Download the VirtIO driver to the `iso` directory
    
.. code-block:: bash

  cd /home/ghr/system_imaging/iso && wget https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.271-1/virtio-win.iso

Change ownership:

.. code-block:: bash

  chown -R ghr:ghr /home/ghr/system_imaging

Check
~~~~~

Login as user ``ghr``.

.. code-block:: bash

  su - ghr

Test with QEMU:

- Copy the original image to somewhere safe.
    
.. code-block:: bash

  mkdir -p /home/ghr/guests/tmp
  rm -rf /home/ghr/guests/tmp/*
  cp /var/lib/libvirt/images/<qcow2 image> /home/ghr/guests/tmp/boot.img

- Start a swtpm socket
    
.. code-block:: bash

  swtpm socket --tpm2 --ctrl type=unixio,path=/home/ghr/guests/tmp/swtpm.sock --tpmstate dir=/home/ghr/guests/tmp --flags not-need-init --daemon
    
- Start the qemu guest:
    
.. code-block:: bash

  /opt/qemu/bin/qemu-system-x86_64 \
  -cpu host \
  -smp 6 \
  -m 32G \
  -accel kvm \
  -pidfile /home/ghr/guests/tmp/guest.pid \
  -monitor unix:/home/ghr/guests/tmp/monitor.sock,server,nowait \
  -display none \
  -serial file:/home/ghr/guests/tmp/serial.output \
  -daemonize \
  -netdev user,id=n1,ipv6=off,hostfwd=tcp:127.0.0.1:4200-:22 \
  -device virtio-net-pci,netdev=n1 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.ms.fd \
  -drive if=pflash,format=raw,file=/home/ghr/system_imaging/fd/win11_VARS.fd \
  -blockdev driver=qcow2,node-name=boot,file.driver=file,file.filename=/home/ghr/guests/tmp/boot.img \
  -device ide-hd,drive=boot \
  -drive file=/home/ghr/system_imaging/iso/virtio-win.iso,media=cdrom \
  -M q35,kernel_irqchip=split \
  -chardev socket,id=chrtpm,path=/home/ghr/guests/tmp/swtpm.sock \
  -tpmdev emulator,id=tpm0,chardev=chrtpm \
  -device tpm-tis,tpmdev=tpm0 \
  -device pcie-root-port,id=rp_nvme,chassis=2,slot=2 \
  -device vfio-pci,host=0000:02:00.0,bus=rp_nvme
    
- Wait for it to start, and maybe give it 10 seconds to boot all the way
- Then try to login
    
.. code-block:: bash

  ssh -p 4200 root@localhost
    
- Hopefully it works! 

- Remember to close down the qemu guest.

.. code-block:: bash
  
  kill -9 $(cat /home/ghr/guests/tmp/guest.pid)


.. _Windows 11 Disk Image: https://www.microsoft.com/en-us/software-download/windows11
.. _virt-manager: https://virt-manager.org/
.. _windows-2022.ps1: https://github.com/xnvme/xnvme/blob/next/toolbox/pkgs/windows-2022.ps1
.. _OpenSSH-Win64.zip: https://github.com/PowerShell/Win32-OpenSSH/releases
.. _VirtIO: https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/