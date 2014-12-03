#
#	makefile for make Gb.aiton
#
CROSS_COMPILE = /home/lurenhua/arm-2011.03/bin/arm-none-linux-gnueabi-
CFLAGS  = -Wall  -I/home/lurenhua/arm-2011.03/arm-none-linux-gnueabi/libc/usr/include 
SQLLIB  = /home/lurenhua/sqlite-autoconf-3080403/build/lib/libsqlite3.so
CC  = $(CROSS_COMPILE)g++
LIB     =  -L $(ACE_ROOT)/ace  -l pthread  -l rt -l dl  # -L$(ACE_ROOT)/ace -lACE
ACELIB =  $(ACE_ROOT)/ace/libACE.so
#
#CROSS_COMPILE = arm-linux-
#CFLAGS  = -Wall  -I/usr/local/arm/4.3.2/arm-none-linux-gnueabi/libc/usr/include 
#LDFLAGS = /usr/local/arm/4.3.2/lib
#SQLLIB  = /opt/sqlite-autoconf-3071401/build/lib/libsqlite3.so
#LINKFLAG = #-static
#CC  = arm-linux-g++
#CC  = g++
# -DNDEBUG -g 
#LIB     =  -L $(ACE_ROOT)/ace  -l pthread  -l rt -l dl  # -L$(ACE_ROOT)/ace -lACE
#ACELIB =  $(ACE_ROOT)/ace/libACE.so
#PTHREADLIB= # $(LDFLAGS)/libpthread.a
#RTLIB= # $(LDFLAGS)/librt.a
#DLLIB= # $(LDFLAGS)/libdl.a

DEST =  Gb.aiton
all:	$(DEST) Makefile
%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@ $(INCLUDE) -I$(ACE_ROOT) -L$(ACE_ROOT)/ace
clean:
	rm -rf *.o *.aiton
Gb.aiton: GbtTimer.o      PowerBoard.o    IoOperate.o  	\
          DbInstance.o   GbtDb.o         PscMode.o       TscMsgQueue.o     	\
          Detector.o     Gps.o           Rs485.o         TscTimer.o         \
          FlashMac.o     LampBoard.o     Cdkey.o  				  WatchDog.o         \
          GaCountDown.o  Log.o           Coordinate.o       \
          GbtExchg.o     MainBoardLed.o  TimerManager.o  ManaKernel.o 		\
          GbtMsgQueue.o  Gb.o            Can.o         	ComFunc.o		\
	 	  MacControl.o	 MainBackup.o		 SignalDefaultData.o  SerialCtrl.o	Configure.o	 Gsm.o \
	 	  WirelessButtons.o


	$(CC) $(LIB) $(CFLAGS) \
	$(ACELIB) $(SQLLIB) \
		GbtTimer.o      PowerBoard.o    IoOperate.o     \
        DbInstance.o   GbtDb.o         PscMode.o       TscMsgQueue.o     \
        Detector.o     Gps.o           Rs485.o         TscTimer.o        \
        FlashMac.o     LampBoard.o     Cdkey.o 					WatchDog.o        \
        GaCountDown.o  Log.o           Coordinate.o     \
        GbtExchg.o     MainBoardLed.o  TimerManager.o  ManaKernel.o      \
        GbtMsgQueue.o  Gb.o         	Can.o 			ComFunc.o      \
		MacControl.o 	MainBackup.o 		SignalDefaultData.o	  SerialCtrl.o	Configure.o	 Gsm.o   \
		WirelessButtons.o   -o Gb.aiton 

release:
	tar zcvf Gb.tar Gb.aiton rcS run.sh stop.sh install.sh update.sh uninstall.sh
