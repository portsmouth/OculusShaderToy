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


	OBJECTIVE_SOURCES += src/Mac.mm

	HEADERS += src/Renderer.h \
				src/Player.h \
				src/Shaders.h \
				src/Mac.h

	LIBS += -L/Developer/OculusSDK2/LibOVR/Lib/Mac/Release/ -lovr

	INCLUDEPATH +=  /Developer/OculusSDK2/LibOVR/Include/ \
					/Developer/OculusSDK2/LibOVR/Src \
					/opt/local/include/OpenEXR/

	LIBS += -framework cocoa -framework carbon -framework opengl

	DEFINES += SHADER_DIR="/Users/jamports/projects/OculusExperiments/shaders"

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/oculus
INSTALLS += target

OTHER_FILES += \
		shaders/shader.vs \
		shaders/shader.fs \
    shaders/menger.fs \
    shaders/mengerDistort.fs \
    shaders/repetition.fs \
    shaders/seascape.fs \
    shaders/waterpipe.fs \
    shaders/cune.fs \
    shaders/flyingCubes.fs \
    shaders/test.fs \
    shaders/mengerRotate.fs \
    shaders/aaSpheres.fs \
    shaders/spikes.fs \
    shaders/kifs.fs \
    shaders/metahex.fs \
    shaders/mengersDream.fs \
    shaders/loveTunnel.fs


