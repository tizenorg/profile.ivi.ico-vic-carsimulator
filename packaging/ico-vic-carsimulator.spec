Name:       ico-vic-carsimulator
Summary:    CarSimulator
Version:    0.9.05
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
mkdir -p %{buildroot}/etc/ico-vic-carsim/
mkdir -p %{buildroot}/usr/lib/systemd/system/
install -m 0644 G25.conf %{buildroot}/etc/ico-vic-carsim/
install -m 0644 G27.conf %{buildroot}/etc/ico-vic-carsim/
install -d %{buildroot}/%{_unitdir_user}/weston.target.wants
install -m 0644 ico-vic-carsim.service %{buildroot}%{_unitdir_user}/ico-vic-carsim.service
install -m 0644 ico-vic-carsim-wait-amb-ready.path %{buildroot}%{_unitdir_user}/ico-vic-carsim-wait-amb-ready.path
ln -sf ../ico-vic-carsim-wait-amb-ready.path %{buildroot}/%{_unitdir_user}/weston.target.wants/

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_bindir}/*
/etc/ico-vic-carsim/*
/usr/lib/systemd/user/ico-vic-carsim.service
/usr/lib/systemd/user/ico-vic-carsim-wait-amb-ready.path
/usr/lib/systemd/user/weston.target.wants/ico-vic-carsim-wait-amb-ready.path
