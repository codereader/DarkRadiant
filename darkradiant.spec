Name:		darkradiant
Version:	0.9.12
Release:	2%{?dist}
Summary:	Level editor for Doom 3 and The Dark Mod
Group:		Applications/Editors
License:	GPLv2 and LGPLv2 and BSD
URL:		http://darkradiant.sourceforge.net/
Source0:	%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:	automake, autoconf, libtool, desktop-file-utils

%description
 DarkRadiant is a 3D level editor for Doom 3 and The Dark Mod, based on the GPL
 release of GtkRadiant.

%prep
%setup -q

%build
%configure --enable-darkmod-plugins --enable-debug --prefix=/usr
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
desktop-file-install					\
  --dir=${RPM_BUILD_ROOT}%{_datadir}/applications	\
  ${RPM_BUILD_ROOT}%{_datadir}/applications/darkradiant.desktop

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc README
%{_bindir}/*
%{_libdir}/darkradiant/lib*
%{_libdir}/darkradiant/modules
%{_libdir}/darkradiant/scripts
%{_libdir}/darkradiant/plugins/eclasstree*
%{_datadir}/*

%package plugins-darkmod
Summary:	DarkMod-specific plugins for DarkRadiant
Group:		Applications/Editors
Requires:	darkradiant

%description plugins-darkmod
 These plugins are used for editing Dark Mod missions.

%files plugins-darkmod
%defattr(-,root,root,-)
%doc README.linux
%{_libdir}/darkradiant/plugins/dm_*

%changelog
* Tue Mar 26 2009 ibix <ibixian@gmail.com> - 0.9.12-2
- patches upstream. Removed here.

* Tue Mar 24 2009 ibix <ibixian@gmail.com> - 0.9.12-1
- spec file.
- patch for sound detection on fedora.
- patch for valid desktop entry.

