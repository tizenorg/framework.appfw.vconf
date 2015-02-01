Name:       vconf
Summary:    Configuration system library
Version:    0.2.83
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
BuildRequires:	pkgconfig(libsmack)

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
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
export CFLAGS="$CFLAGS -Wall -Werror"
export CFLAGS="$CFLAGS -Wno-unused-function -Wno-unused-but-set-variable"

cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/opt/var/kdb/db
mkdir -p %{buildroot}/opt/var/kdb/db/.backup
mkdir -p %{buildroot}/opt/var/kdb/file
mkdir -p %{buildroot}/opt/var/kdb/file/.backup
mkdir -p %{buildroot}/tmp
#touch %{buildroot}/opt/var/kdb/.vconf_lock
mkdir -p %{buildroot}%{_libdir}/systemd/system/basic.target.wants
mkdir -p %{buildroot}%{_libdir}/tmpfiles.d
install -m0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/
install -m0644 %SOURCE2 %{buildroot}%{_libdir}/tmpfiles.d/
ln -sf ../vconf-setup.service %{buildroot}%{_libdir}/systemd/system/basic.target.wants/
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
%{_bindir}/vconftool
%config(missingok) %attr(644,root,root) /opt/var/kdb/kdb_first_boot
%{_libdir}/*.so.*
%dir %attr(777,root,root) /opt/var/kdb/db
%dir %attr(777,root,root) /opt/var/kdb/db/.backup
%dir %attr(777,root,root) /opt/var/kdb/file
%dir %attr(777,root,root) /opt/var/kdb/file/.backup
#/opt/var/kdb/.vconf_lock
%{_libdir}/systemd/system/basic.target.wants/vconf-setup.service
%{_libdir}/systemd/system/vconf-setup.service
%{_libdir}/tmpfiles.d/vconf-setup.conf
/usr/share/license/%{name}
#/etc/opt/upgrade/001.vconf.patch.sh
#%attr(755,root,root) /etc/opt/upgrade/001.vconf.patch.sh

%files devel
%defattr(-,root,root,-)
%{_includedir}/vconf/vconf.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/*.so

%files keys-devel
%defattr(-,root,root,-)
%{_includedir}/vconf/vconf-keys.h

%changelog
* Wed Jun 25 2014 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Fix vconf unset recursive error under COMBINE FOLDER define
- Make folder & change attr folder for file key

* Wed Mar 19 2014 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Add ag connected value for VCONFKEY_BT_DEVICE key

* Wed Feb 05 2014 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Directory smack label set time is changed (from first boot to creation time)

* Fri Sep 06 2013 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Add new api for getting last file error at vconf_get*,vconf_set* api

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
