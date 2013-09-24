Name:       vconf
Summary:    Configuration system library
Version:    0.2.51
Release:    1
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    vconf-setup.service
Source2:    vconf-setup.conf
Requires(post): /sbin/ldconfig, systemd
Requires(postun): /sbin/ldconfig, systemd
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:	  pkgconfig(libsmack)

%description 
Configuration system library

%package devel
Summary:    vconf (devel)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   pkgconfig(glib-2.0)
Requires:   vconf = %{version}-%{release}
Requires:   vconf-keys-devel = %{version}-%{release}

%description devel
Vconf library (devel)

%package keys-devel
Summary:    vconf (devel)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   vconf = %{version}-%{release}
Requires:   vconf-internal-keys-devel

%description keys-devel
Vconf key management header files

%prep
%setup -q -n %{name}-%{version}

%build
%cmake .

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/etc/rc.d/rc3.d
mkdir -p %{buildroot}/etc/rc.d/rc4.d
ln -sf /etc/rc.d/init.d/vconf-init %{buildroot}/etc/rc.d/rc3.d/S04vconf-init
ln -sf /etc/rc.d/init.d/vconf-init %{buildroot}/etc/rc.d/rc4.d/S04vconf-init
mkdir -p %{buildroot}/opt/var/kdb/db
mkdir -p %{buildroot}/opt/var/kdb/db/.backup
#mkdir -p %{buildroot}/tmp
#touch %{buildroot}/tmp/vconf-initialized
mkdir -p %{buildroot}%{_prefix}/lib/systemd/system/basic.target.wants
mkdir -p %{buildroot}%{_prefix}/lib/tmpfiles.d
install -m0644 %SOURCE1 %{buildroot}%{_prefix}/lib/systemd/system/
install -m0644 %SOURCE2 %{buildroot}%{_prefix}/lib/tmpfiles.d/
ln -sf ../vconf-setup.service %{buildroot}%{_prefix}/lib/systemd/system/basic.target.wants/
mkdir -p %{buildroot}/usr/share/license
install LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}

%post
/sbin/ldconfig
systemctl daemon-reload

%postun
/sbin/ldconfig
systemctl daemon-reload

%files
%manifest vconf.manifest
%defattr(-,root,root,-)
%attr(755,root,root) %{_sysconfdir}/rc.d/init.d/vconf-init
%{_sysconfdir}/rc.d/rc3.d/S04vconf-init
%{_sysconfdir}/rc.d/rc4.d/S04vconf-init
%attr(755,root,root) %{_sysconfdir}/preconf.d/vconf-setup
%{_bindir}/vconftool
%config(missingok) %attr(644,root,root) /opt/var/kdb/kdb_first_boot
%{_libdir}/*.so.*
%dir %attr(777,root,root) /opt/var/kdb/db
%dir %attr(777,root,root) /opt/var/kdb/db/.backup
#/tmp/vconf-initialized
%{_prefix}/lib/systemd/system/basic.target.wants/vconf-setup.service
%{_prefix}/lib/systemd/system/vconf-setup.service
%{_prefix}/lib/tmpfiles.d/vconf-setup.conf
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
%{_includedir}/vconf/vconf.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/*.so

%files keys-devel
%defattr(-,root,root,-)
%{_includedir}/vconf/vconf-keys.h

%changelog
* Tue Oct 23 2012 - SeungYeup Kim <sy2004.kim@samsung.com>
- Add thread safe code

* Tue Sep 18 2012 - SeungYeup Kim <sy2004.kim@samsung.com>
- Add 4 public keys (Browser User Agent)

* Tue Aug 28 2012 - SeungYeup Kim <sy2004.kim@samsung.com>
- Remove memory leak
- Remove use after free

* Tue Aug 14 2012 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Fix issues related prevent
- Fix warning msg at build time

* Mon Jul 23 2012 - SeungYeup Kim <sy2004.kim@samsung.com>
- Enable -f option for force update
