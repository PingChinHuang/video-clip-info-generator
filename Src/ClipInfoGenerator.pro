#-------------------------------------------------
#
# Project created by QtCreator 2013-09-22T11:33:05
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = clip_info_generator
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

#unix|win32: LIBS += -lQt5Multimediad\
#                    -lQt5MultimediaWidgets

RESOURCES += \
    ClipInfoGenerator.qrc
