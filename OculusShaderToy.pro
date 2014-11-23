
QMAKE_MAC_SDK = macosx10.9
QT       += core gui widgets
TARGET = OculusShaderToy
TEMPLATE = app

# Set this to your local installation of the Oculus SDK 2
OCULUS_SDK2 = /Developer/OculusSDK2

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

LIBS += -L$${OCULUS_SDK2}/LibOVR/Lib/Mac/Release/ -lovr

INCLUDEPATH +=  $${OCULUS_SDK2}/LibOVR/Include/ \
				$${OCULUS_SDK2}/LibOVR/Src

LIBS += -framework cocoa -framework carbon -framework opengl
DEFINES += SHADER_DIR=$$PWD/shaders

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/oculus
INSTALLS += target

OTHER_FILES += \
		shaders/shader.vs \
		shaders/shader.fs \
		shaders/mengerDistort.fs \
		shaders/mengersDream.fs \
		shaders/mengerSpiral.fs \
		shaders/repetition.fs \
		shaders/seascape.fs \
		shaders/waterpipe.fs \
		shaders/metahex.fs \
		shaders/loveTunnel.fs \
		shaders/minecraft.fs \
		shaders/mirror.fs
