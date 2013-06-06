Name:       ico-vic-carsimulator
Summary:    CarSimulator
Version:    0.1.3
Release:    1
Group:      System Environment/Daemons
License:    Apache 2.0
Source0:    %{name}-%{version}.tar.bz2
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Requires:       glib2 
Requires:       ico-vic-amb-plugin 
BuildRequires:  make
BuildRequires:  automake
BuildRequires:  boost-devel
BuildRequires:  libwebsockets-devel
BuildRequires:  glib2-devel
BuildRequires:  json-glib-devel

%description 
CarSimulator is simulated driving software

%prep
%setup -q -n %{name}-%{version}

%build
autoreconf --install

%configure
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install

# configurations
%define carsim_conf /usr/bin/
install -m 0644 src/CarSim_Daemon.conf %{buildroot}%{carsim_conf}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_bindir}/*
%{carsim_conf}/CarSim_Daemon.conf
