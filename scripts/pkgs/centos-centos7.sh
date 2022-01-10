#!/bin/sh
# Query the linker version
ld --version || true

# Query the (g)libc version
ldd --version || true

# Install Developer Tools
yum install -y centos-release-scl
yum-config-manager --enable rhel-server-rhscl-7-rpms
yum install -y devtoolset-8

# Install packages via yum
yum install -y $(cat "scripts/pkgs/centos-centos7.txt")

# Install packages via PyPI
pip3 install meson ninja pyelftools

# Source them in for usage before building
source /opt/rh/devtoolset-8/enable

gcc --version
g++ --version

# Add to bash-profile if it makes sense to you
#echo "#!/bin/bash\nsource scl_source enable devtoolset-8" >> /etc/profile.d/devtoolset.sh
#echo "#!/bin/bash\nsource scl_source enable devtoolset-8" >> ~/.bashrc
#echo "#!/bin/bash\nsource scl_source enable devtoolset-8" >> ~/.bash_profile
