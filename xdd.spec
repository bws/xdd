#
# spec file for package xdd
#
# Copyright (c) 2015 SUSE LINUX GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#


# See also http://en.opensuse.org/openSUSE:Specfile_guidelines

Name:           xdd
Version:        0.6
Release:        0
Summary:       	IO performace Test Tool dd with an X factor
License:        GPL-2.0
Group:         	Benchmarking
Url: https://github.com/bws/xdd
Source0:	xdd-0.6.tgz
Source1:	xdd-rpmlintrc
patch0:		xdd.patch
patch1:		xdd01.patch
BuildRequires:  autoconf xfsprogs-devel python-pycrypto python-ecdsa python-setuptools python-devel
#Requires:       

%description
IO Test tool that can do so much more.

%prep
%setup -q

%patch -P 0 -p1
%patch -P 1 -p1

%build
autoconf
%configure --prefix=/usr --enable-xfs
make %{?_smp_mflags}

%install
make install INSTALL_DIR=%{buildroot}/usr

%files
%defattr(-,root,root,-)
%doc
%{_bindir}/*
%{_libexecdir}/*

%changelog
* Fri Jan 29 2016 Russell Cattelan
- 
