Name:           xnvme
Version:        0.7.4
Release:        1%{?dist}
Summary:        Unified API and tools for traditional and emerging I/O interfaces

License:        BSD-3-Clause
URL:            https://github.com/OpenMPDK/xNVMe
Source:         %{url}/releases/download/v%{version}/xnvme-%{version}.tar.gz

BuildRequires:  meson gcc
BuildRequires:  libaio-devel liburing-devel

%global major_version %(echo %{version} | cut -b 1)

%description
%{summary}.
Minimal-overhead libraries and tools for cross-platform storage I/O and
NVMe-native development. A unified API encapsulating traditional block-I/O via
psync, libaio, and io_uring as well as user-space NVMe drivers.

%package devel
Summary:        Development library and header files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
%{summary}.

%package static
Summary:        Static library for %{name}

%description static
%{summary}.

%package devel-static
Summary:        Static development library and header files for %{name}
Requires:       %{name}-static%{?_isa} = %{version}-%{release}

%description devel-static
%{summary}.


%package cli
Summary:        Command-line tools for storage I/O and NVMe-native development
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description cli
%{summary}.

%prep
%autosetup

%build
%meson -Dforce_completions=true -Dwith-libvfn=disabled -Dwith-isal=disabled -Dwith-spdk=false -Dexamples=false -Dtests=false
%meson_build

%install
%meson_install

%files
%{_libdir}/lib%{name}.so
%{_libdir}/lib%{name}.so.%{major_version}
%{_libdir}/lib%{name}.so.%{version}

%files devel
%{_includedir}/lib%{name}*.h
%{_libdir}/pkgconfig/%{name}.pc

%files static
%{_libdir}/lib%{name}.a

%files devel-static
%{_includedir}/lib%{name}*.h
%{_libdir}/pkgconfig/%{name}.pc

%files cli
%{_bindir}/kvs
%{_bindir}/lblk
%{_bindir}/xdd
%{_bindir}/%{name}
%{_bindir}/%{name}-driver
%{_bindir}/%{name}_file
%{_bindir}/zoned
%{_mandir}/man1/kvs*
%{_mandir}/man1/lblk*
%{_mandir}/man1/xdd*
%{_mandir}/man1/%{name}-*
%{_mandir}/man1/%{name}.1.gz
%{_mandir}/man1/%{name}_file*
%{_mandir}/man1/zoned*
/usr/share/bash-completion/completions/kvs-completions
/usr/share/bash-completion/completions/lblk-completions
/usr/share/bash-completion/completions/xdd-completions
/usr/share/bash-completion/completions/%{name}-completions
/usr/share/bash-completion/completions/%{name}_file-completions
/usr/share/bash-completion/completions/zoned-completions
