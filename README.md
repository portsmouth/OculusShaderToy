OculusShaderToy
===============

An experimental Qt-based port of [ShaderToy](https://www.shadertoy.com/) code running in OculusVR.
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

The original ShaderToy shaders these are based on can be found here:
[Seascape](https://www.shadertoy.com/view/Ms2SD1)
[Love Tunnel](https://www.shadertoy.com/view/XdBGDd)
[MetaHexaBalls](https://www.shadertoy.com/view/Mss3WN)
[Menger Journey](https://www.shadertoy.com/view/Mdf3z7)
[Menger's Dream](https://www.shadertoy.com/view/lsSSDV)
[Spout](https://www.shadertoy.com/view/lsXGzH)
[Mirror Room](https://www.shadertoy.com/view/4sS3zc)
[Infinite Repetition](https://www.shadertoy.com/view/4dXGRN)
[Minecraft](https://www.shadertoy.com/view/4ds3WS)

Installation
============

In the [OculusShaderToy.pro](https://github.com/portsmouth/OculusShaderToy/blob/master/OculusShaderToy.pro) file, set the variable OCULUS_SDK2 to the directory of your local installation of the Oculus SDK 2. This should then build in QtCreator (on Mac OS X Yosemite).

