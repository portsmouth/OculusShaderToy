OculusShaderToy
===============

An experimental Qt-based port of [ShaderToy](https://www.shadertoy.com/) code running in OculusVR (DK2, SDK v0.4).
Currently this is Mac OS X only (tested only on Yosemite), though the code is almost pure Qt and porting to other platforms should be straightforward.

Ships with the following ported shaders:

![alt tag](https://raw.githubusercontent.com/portsmouth/OculusShaderToy/master/images/seascape.png)
![alt tag](https://raw.githubusercontent.com/portsmouth/OculusShaderToy/master/images/lovetunnel.png)
![alt tag](https://raw.githubusercontent.com/portsmouth/OculusShaderToy/master/images/metaballs.png)
![alt tag](https://raw.githubusercontent.com/portsmouth/OculusShaderToy/master/images/mengerDistort.png)
![alt tag](https://raw.githubusercontent.com/portsmouth/OculusShaderToy/master/images/mengersDream.png)
![alt tag](https://raw.githubusercontent.com/portsmouth/OculusShaderToy/master/images/waterpipe.png)
![alt tag](https://raw.githubusercontent.com/portsmouth/OculusShaderToy/master/images/mirrorRoom.png)
![alt tag](https://raw.githubusercontent.com/portsmouth/OculusShaderToy/master/images/repetition.png)
![alt tag](https://raw.githubusercontent.com/portsmouth/OculusShaderToy/master/images/minecraft.png)

Some videos of it in action:
  - [Love Tunnel](https://www.youtube.com/watch?v=p5O6fMofPjo)
  - [Menger Distort](https://www.youtube.com/watch?v=p5O6fMofPjo)
  - [Menger's Dream](https://www.youtube.com/watch?v=CIAn3LVNZO0)
  - [Seascape](https://www.youtube.com/watch?v=0_Cj7mXfFUM)

The original ShaderToy shaders these are based on can be found here:
  - [Seascape](https://www.shadertoy.com/view/Ms2SD1)
  - [Love Tunnel](https://www.shadertoy.com/view/XdBGDd)
  - [MetaHexaBalls](https://www.shadertoy.com/view/Mss3WN)
  - [Menger Journey](https://www.shadertoy.com/view/Mdf3z7)
  - [Menger's Dream](https://www.shadertoy.com/view/lsSSDV)
  - [Spout](https://www.shadertoy.com/view/lsXGzH)
  - [Mirror Room](https://www.shadertoy.com/view/4sS3zc)
  - [Infinite Repetition](https://www.shadertoy.com/view/4dXGRN)
  - [Minecraft](https://www.shadertoy.com/view/4ds3WS)

Controls
========

(Note that on startup, a mouse click may be needed for the application to enter fullscreen correctly. This is a bug).

  - Look around as usual via head motion in Oculus VR.
  - Change body rotation (yaw) via left-right mouse movement. 
  - Press **Space** to move to next demo.
  - Press **P** key to pause the animation. (Note that in some demos, the camera position can only be changed in paused mode).
  - Move in world space via **AWSD** keys, and cursor keys.
  - Press **G** to show a GL-rendered grid overlaid on top of the raymarching render.

Installation
============

You need Qt5 installed, and Oculus SDK v0.4. Obviously an Oculus DK2 devkit would be handy as well.

In the [OculusShaderToy.pro](https://github.com/portsmouth/OculusShaderToy/blob/master/OculusShaderToy.pro) file, set the variable OCULUS_SDK2 to the directory of your local installation of the Oculus SDK 2. This should then build in QtCreator (on Mac OS X Yosemite).

