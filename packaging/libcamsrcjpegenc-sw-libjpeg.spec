Name:       libcamsrcjpegenc-sw-libjpeg
Summary:    Multimedia Framework Camera Src Jpeg Encoder Library (libjpeg)
Version:    0.1.4
Release:    2
Group:      libdevel
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(camsrcjpegenc)
BuildRequires:  pkgconfig(mmutil-imgp)
BuildRequires:  libjpeg-devel

%description
Multimedia Framework Camera Src Jpeg Encoder Library (libjpeg)

%prep
%setup -q -n %{name}-%{version}

%build
./autogen.sh
%configure --disable-static --enable-dlog
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%files
%defattr(-,root,root,-)
%{_libdir}/libcamsrcjpegenc-sw.so*

