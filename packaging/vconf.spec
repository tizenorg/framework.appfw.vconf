Name:       vconf
Summary:    Configuration system library
Version:    0.2.84
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    vconf-setup.service
Source2:    vconf-setup.conf
Requires(post): /sbin/ldconfig, systemd
Requires(postun): /sbin/ldconfig, systemd
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
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

%define appfw_feature_vconf_module_dump 1
%define appfw_feature_vconf_restore_key 1
%define appfw_feature_vconf_smack_whitelist 0

%if 0%{?appfw_feature_vconf_module_dump}
_APPFW_FEATURE_VCONF_MODULE_DUMP=ON
%endif

%if 0%{?appfw_feature_vconf_restore_key}
 _APPFW_FEATURE_VCONF_MODULE_RESTORE_KEY=ON
%endif

%if 0%{?appfw_feature_vconf_smack_whitelist}
 _APPFW_FEATURE_VCONF_SMACK_WHITELIST=ON
%endif


 _APPFW_FEATURE_VCONF_ZONE=ON

cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} \
	-D_APPFW_FEATURE_VCONF_ZONE:BOOL=${_APPFW_FEATURE_VCONF_ZONE} \
	-D_APPFW_FEATURE_VCONF_MODULE_DUMP:BOOL=${_APPFW_FEATURE_VCONF_MODULE_DUMP} \
	-D_APPFW_FEATURE_VCONF_MODULE_RESTORE_KEY:BOOL=${_APPFW_FEATURE_VCONF_MODULE_RESTORE_KEY} \
	.

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/opt/var/kdb/db
mkdir -p %{buildroot}/opt/var/kdb/file
mkdir -p %{buildroot}/opt/var/kdb/memory_init
mkdir -p %{buildroot}/tmp
#touch %{buildroot}/opt/var/kdb/.vconf_lock
mkdir -p %{buildroot}%{_libdir}/systemd/system/basic.target.wants
mkdir -p %{buildroot}%{_libdir}/tmpfiles.d
install -m0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/
install -m0644 %SOURCE2 %{buildroot}%{_libdir}/tmpfiles.d/
ln -sf ../vconf-setup.service %{buildroot}%{_libdir}/systemd/system/basic.target.wants/
mkdir -p %{buildroot}/usr/share/license
install LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}
%if 0%{?appfw_feature_vconf_restore_key}
mkdir -p %{buildroot}/opt/var/kdb/.restore/.history
%endif


%post
/sbin/ldconfig
systemctl daemon-reload
%if 0%{?appfw_feature_vconf_restore_key}
chsmack -a "*" /opt/var/kdb/.restore
%endif

%postun
/sbin/ldconfig
systemctl daemon-reload

%files
%manifest vconf.manifest
%defattr(-,root,root,-)
%attr(755,root,root) %{_sysconfdir}/rc.d/init.d/vconf-init
%{_bindir}/vconftool
%{_bindir}/vconftool2
%config(missingok) %attr(644,root,root) /opt/var/kdb/kdb_first_boot
%{_libdir}/*.so.*
%dir %attr(777,root,root) /opt/var/kdb/db
%dir %attr(777,root,root) /opt/var/kdb/file
%dir %attr(777,root,root) /opt/var/kdb/memory_init
#/opt/var/kdb/.vconf_lock
%{_libdir}/systemd/system/basic.target.wants/vconf-setup.service
%{_libdir}/systemd/system/vconf-setup.service
%{_libdir}/tmpfiles.d/vconf-setup.conf
/usr/share/license/%{name}
%if 0%{?appfw_feature_vconf_module_dump}
%attr(755,root,root) /opt/etc/dump.d/module.d/vconf_dump.sh
%endif
%if 0%{?appfw_feature_vconf_restore_key}
%dir %attr(777,root,root) /opt/var/kdb/.restore/
%dir %attr(700,root,root) /opt/var/kdb/.restore/.history/
%attr(700,root,root) %{_bindir}/vconf-restore-key.sh
/usr/share/dbus-1/system-services/org.vconf.restore.service
%endif

%files devel
%defattr(-,root,root,-)
%{_includedir}/vconf/vconf.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/*.so

%files keys-devel
%defattr(-,root,root,-)
%{_includedir}/vconf/vconf-keys.h
