bin_PROGRAMS = carsim

carsim_SOURCES = Websocket.h Websocket.cpp CJoyStick.h CJoyStick.cpp CJoyStickEV.h CJoyStickEV.cpp CConf.h CConf.cpp CGtCtrl.h CGtCtrl.cpp CCalc.h CCalc.cpp CAvgCar.h CAvgCar.cpp CarSim_Daemon.cpp
carsim_LDADD = -L../lib
carsim_CPPFLAGS = -I/usr/include/glib-2.0 -I/usr/include/json-glib-1.0 -I/usr/lib/glib-2.0/include -I../include
carsim_LDFLAGS = -lTimeMeasureSend -lpthread -ljson-glib-1.0 -lgobject-2.0 -lglib-2.0 -lwebsockets -lrt
