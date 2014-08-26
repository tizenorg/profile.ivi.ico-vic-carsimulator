Name:       ico-vic-carsimulator
Summary:    CarSimulator
Version:    0.9.10
Release:    0
Group:      System/Utilities
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.bz2
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig
Requires:           ico-vic-amb-plugin
Requires:           ico-uxf-utilities
BuildRequires:      make
BuildRequires:      automake
BuildRequires:      glib2-devel
BuildRequires:      json-glib-devel
BuildRequires:      ico-uxf-utilities-devel

%description
CarSimulator is simulated driving software

%prep
%setup -q -n %{name}-%{version}

%build
%reconfigure
%__make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install

# configurations
mkdir -p %{buildroot}%{_sysconfdir}/ico-vic-carsim/
mkdir -p %{buildroot}%{_unitdir_user}
install -m 0644 G25.conf %{buildroot}%{_sysconfdir}/ico-vic-carsim/
install -m 0644 G27.conf %{buildroot}%{_sysconfdir}/ico-vic-carsim/
install -m 0644 ico-vic-carsim.service %{buildroot}%{_unitdir_user}/ico-vic-carsim.service

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%attr(4755,root,root) %{_bindir}/ico-vic-carsim
%defattr(-,root,root,-)
%{_bindir}/*
%config %{_sysconfdir}/ico-vic-carsim/*
%{_unitdir_user}/ico-vic-carsim.service
