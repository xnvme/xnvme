Name:           xnvme
Version:        0.3.0
Release:        1%{?dist}
Summary:        Unified API and tools for traditional and emerging I/O interfaces

License:        Apache License v2.0
URL:            https://github.com/OpenMPDK/xNVMe
Source:         %{url}/releases/download/v%{version}/xnvme-%{version}.tar.gz

BuildRequires:  gcc gcc-g++ meson pkgconfig
BuildRequires:  libaio-devel

BuildRequires:  CUnit-devel libuuid-devel numactl-devel openssl-devel python3-pyelftools

%description
%{summary}.
Minimal-overhead libraries and tools for cross-platform storage I/O and
NVMe-native development. A unified API encapsulating traditional block-I/O via
psync, libaio, and io_uring as well as user-space NVMe drivers.

%package devel
Summary:        Development libraries and header files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
%{summary}.
This packages provides header files to include an libraries to link with for
cross-platform I/O and NVMe-native development.

%package tools
Summary:        Command-line tools for storage I/O and NVMe-native development
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description tools
%{summary}.
This packages provides header files to include an libraries to link with for
cross-platform I/O and NVMe-native development.

%package fio-engine
Summary:        xNVMe fio I/O engine
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description fio-engine
%{summary}.
Providing a single fio I/O engine implementation using the xNVMe unified
storage API. Enabling the use of fio with traditional I/O interfaces, fallback
implementations as well as user space NVMe drivers.

%prep
%autosetup

%build
%meson
%meson_build

%install
%meson_install

%files
%{_libdir}/lib%{name}-shared.so
%{_libdir}/lib%{name}-static.a
%{_libdir}/lib%{name}.a
%{_libdir}/pkgconfig/%{name}.pc

%files devel
%{_includedir}/lib%{name}*.h
%{_libdir}/lib%{name}-shared.so
%{_libdir}/lib%{name}-static.a
%{_libdir}/lib%{name}.a
%{_libdir}/pkgconfig/%{name}.pc

%files tools
%{_bindir}/%{name}
%{_bindir}/%{name}-driver
%{_bindir}/%{name}_*
%{_bindir}/lblk
%{_bindir}/nvmec
%{_bindir}/xdd
%{_bindir}/zoned
%{_bindir}/zoned_*
%{_mandir}/man1/%{name}*
%{_mandir}/man1/lblk*
%{_mandir}/man1/nvmec*
%{_mandir}/man1/xdd*
%{_mandir}/man1/zoned*

%files fio-engine
%{_libdir}/lib%{name}-fio-engine.so
%{_libdir}/pkgconfig/%{name}-fio-engine.pc
%{_datadir}/%{name}/*.fio
