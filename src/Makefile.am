bin_PROGRAMS = carsim

carsim_SOURCES = AmbpiComm.h AmbpiComm.cpp CJoyStick.h CJoyStick.cpp CJoyStickEV.h CJoyStickEV.cpp CJoyStickG25.h CJoyStickG25.cpp CJoyStickG27.h CJoyStickG27.cpp CConf.h CConf.cpp CGtCtrl.h CGtCtrl.cpp CCalc.h CCalc.cpp CAvgCar.h CAvgCar.cpp CarSim_Daemon.cpp
carsim_LDADD = -lpthread -ljson-glib-1.0 -lgobject-2.0 -lglib-2.0 -lrt -lico-util
carsim_CPPFLAGS = -I/usr/include/glib-2.0 -I/usr/include/json-glib-1.0 -I/usr/lib/glib-2.0/include -std=c++0x
carsim_LDFLAGS = 
