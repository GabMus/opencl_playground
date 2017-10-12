TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    main.cpp
QMAKE_CXXFLAGS += -std=c++0x
LIBS += -lOpenCL

DISTFILES += \
    vecinit_kernel.cl \
    vaddSimple_kernel.cl

copyfiles.commands = cp $$PWD/*.cl $$OUT_PWD/

QMAKE_EXTRA_TARGETS += copyfiles
POST_TARGETDEPS += copyfiles
