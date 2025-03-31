Name:           libdynpm
Version:        0.1
Release:        1
Summary:        A library for dynamic process management.

License:        3-Clause BSD License
URL:            https://github.com/caps-tum/libdynpm
Source:         %{name}-%{version}.tar.gz
BuildRequires:  libevent-devel

%description
A library that includes reconfigurable components to help implement dynamic process managers and tools.

%prep
%autosetup

%build
%configure
%make_build

%install
%make_install
find %{buildroot} -type f -name '*.a' -delete
find %{buildroot} -type f -name '*.la' -delete

%post
%postun

%files
%defattr(-,root,root)
%config(missingok,noreplace) %{_sysconfdir}/dynpm.conf
%{_bindir}/printdynpmconfig
%{_libdir}/libdynpmcommon.so
%{_libdir}/libdynpmconfig.so
%{_libdir}/libdynpmnetwork.so
%{_libdir}/libdynpmshm.so
%{_includedir}

%changelog
