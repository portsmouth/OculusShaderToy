#-------------------------------------------------
#
# Project created by QtCreator 2014-08-31T19:02:58
#
#-------------------------------------------------

QMAKE_MAC_SDK = macosx10.9
QT       += core gui widgets

TARGET = OculusExperiments
TEMPLATE = app

{
	QT += opengl

	SOURCES += src/main.cpp \
				src/Renderer.cpp \
				src/Player.cpp \
    src/Shaders.cpp

	HEADERS += src/Renderer.h \
				src/Player.h \
    src/Shaders.h

	LIBS += -L/Developer/OculusSDK2/LibOVR/Lib/Mac/Debug/ -lovr

	INCLUDEPATH +=  /Developer/OculusSDK2/LibOVR/Include/ \
					/Developer/OculusSDK2/LibOVR/Src \
					/opt/local/include/OpenEXR/

	LIBS += -framework cocoa -framework carbon -framework opengl -framework IOKit

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/oculus
INSTALLS += target

OTHER_FILES += \
    shaders/shader.vs \
    shaders/shader.fs

