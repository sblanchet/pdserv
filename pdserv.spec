#
# spec file for pdserv library
#
# Copyright (c) 2011 Florian Pose <fp@igh-essen.com>
#
# This file is part of the pdserv package.
#
# pdserv is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# pdserv is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with pdserv. See COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#

Name: 			pdserv
Version:		0.99.0
Release:		1

Summary:		Process-data server library
License:		GPL
Group: 			Development/Libraries
Url: 			http://etherlab.org/en/pdserv
Source: 		http://etherlab.org/download/pdserv/%name-%version.tar.bz2
BuildRoot: 		%_tmppath/%name-%version-root
BuildRequires: 	gcc-c++ libyaml-devel cmake

%description
PdServ is a process data communication server for realtime processes.

%package -n pdserv-devel
Summary: 		Development files for pdserv
Group: 			Development/Libraries
Requires: 		%name = %version

%description -n pdserv-devel
The %name-devel package contains the static libraries and header files
needed for development with %name.

%prep
%setup
find -name CMakeCache.txt -exec rm -v {} \;
cmake -DPREFIX=%{_prefix} .

%build
make

%install
make install DESTDIR=$RPM_BUILD_ROOT

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,755)
%{_libdir}/libpdserv.so*

%files -n pdserv-devel
%defattr(-,root,root,755)
%{_includedir}/pdserv/pdserv.h
%{_libdir}/pkgconfig/libpdserv.pc

%changelog
* Fri Dec 30 2011 fp@igh-essen.com
- Initial package.
