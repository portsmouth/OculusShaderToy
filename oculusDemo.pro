#-------------------------------------------------
#
# Project created by QtCreator 2014-08-31T19:02:58
#
#-------------------------------------------------

QMAKE_MAC_SDK = macosx10.9
QT       += core gui widgets

TARGET = oculusDemo
TEMPLATE = app

{
	QT += opengl

	SOURCES += main.cpp Renderer.cpp Player.cpp

	HEADERS += Renderer.h Player.h

	LIBS += -L/Developer/OculusSDK2/LibOVR/Lib/Mac/Debug/ -lovr

	INCLUDEPATH +=  /Developer/OculusSDK2/LibOVR/Include/ \
					/Developer/OculusSDK2/LibOVR/Src \
					/opt/local/include/OpenEXR/

	LIBS += -framework cocoa -framework carbon -framework opengl -framework IOKit

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/oculus
INSTALLS += target

