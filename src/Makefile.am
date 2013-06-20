bin_PROGRAMS = carsim

carsim_SOURCES = Websocket.h Websocket.cpp CJoyStick.h CJoyStick.cpp CJoyStickEV.h CJoyStickEV.cpp CConf.h CConf.cpp CGtCtrl.h CGtCtrl.cpp CCalc.h CCalc.cpp CAvgCar.h CAvgCar.cpp CarSim_Daemon.cpp
carsim_CPPFLAGS = $(GLIB_CFLAGS) $(JSON_CFLAGS) $(WEBSOCKET_CFLAGS)
carsim_LDADD = -lpthread -lrt $(GLIB_LIBS) $(JSON_LIBS) $(WEBSOCKET_LIBS)


