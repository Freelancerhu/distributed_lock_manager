QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    hiredis/examples/example-qt.cpp \
    main.cpp \
    hiredis/examples/example-ae.c \
    hiredis/examples/example-glib.c \
    hiredis/examples/example-ivykis.c \
    hiredis/examples/example-libev.c \
    hiredis/examples/example-libevent.c \
    hiredis/examples/example-libuv.c \
    hiredis/examples/example-macosx.c \
    hiredis/examples/example.c \
    hiredis/async.c \
    hiredis/dict.c \
    hiredis/hiredis.c \
    hiredis/net.c \
    hiredis/read.c \
    hiredis/sds.c \
    hiredis/test.c

HEADERS += \
    hiredis/adapters/ae.h \
    hiredis/adapters/glib.h \
    hiredis/adapters/ivykis.h \
    hiredis/adapters/libev.h \
    hiredis/adapters/libevent.h \
    hiredis/adapters/libuv.h \
    hiredis/adapters/macosx.h \
    hiredis/adapters/qt.h \
    hiredis/examples/example-qt.h \
    hiredis/async.h \
    hiredis/dict.h \
    hiredis/fmacros.h \
    hiredis/hiredis.h \
    hiredis/net.h \
    hiredis/read.h \
    hiredis/sds.h \
    hiredis/sdsalloc.h \
    hiredis/win32.h

DISTFILES += \
    .gitignore
