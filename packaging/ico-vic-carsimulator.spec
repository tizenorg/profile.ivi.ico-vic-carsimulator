Name:       ico-vic-carsimulator
Summary:    CarSimulator
Version:    0.9.02
Release:    1.1
Group:      System Environment/Daemons
License:    Apache 2.0
Source0:    %{name}-%{version}.tar.bz2
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Requires:       glib2 
Requires:       ico-vic-amb-plugin 
Requires:       ico-uxf-utilities
BuildRequires:  make
BuildRequires:  automake
BuildRequires:  boost-devel
#BuildRequires:  libwebsockets-devel
BuildRequires:  glib2-devel
BuildRequires:  json-glib-devel
#BuildRequires:  ico-uxf-utilities
BuildRequires:  ico-uxf-utilities-devel

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
mkdir -p %{buildroot}/etc/carsim/
mkdir -p %{buildroot}/usr/lib/systemd/system/
#install -m 0644 src/CarSim_Daemon.conf %{buildroot}%{carsim_conf}
install -m 0644 G25.conf %{buildroot}/etc/carsim/
install -m 0644 G27.conf %{buildroot}/etc/carsim/
install -m 0644 carsim.service %{buildroot}/usr/lib/systemd/system/

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_bindir}/*
/etc/carsim/*
/usr/lib/systemd/system/carsim.service
#%{carsim_conf}/CarSim_Daemon.conf
