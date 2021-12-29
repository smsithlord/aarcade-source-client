# Getting Started

## Install Microsoft Visual Studio Community 2013
**NOTE:** For additional resources on the Source engine, visit the [Valve Developer Wiki](https://developer.valvesoftware.com/wiki/Source_SDK_2013)

1. Have Steam installed & be logged in.
1. Download ```Visual Studio Community 2013 with Update 5``` [following the instructions on the wiki](https://developer.valvesoftware.com/wiki/Source_SDK_2013#Step_One:_Installing_Visual_Studio) (requires a Microsoft account). 
1. Extract the ISO into a folder & run the installer.
1. Have ```MFC``` checked - all other optional features can be unchecked.
1. Launch Visual Studio to make sure it works. (You will have to login.)  Then close it after you are sure it works.
1. Install the ```Multibyte MFC Library``` [from the wiki](https://download.microsoft.com/download/0/2/3/02389126-40A7-46FD-9D83-802454852703/vc_mbcsmfc.exe).

## Build AArcade Source Code

1. Clone the ```aarcade-source-client``` repo.
1. Run ```creategameprojects.bat```.
1. Open ```games.sln```.
1. Right-click on Solution (the top of the hierarchy) go to Properties, then Configuration Manager button and change the Active solution configuration from ```Debug``` to ```Release```.
1. Access the ```Client (HL2MP)``` Configuration Properties: Right-click on ```Client (HL2MP)``` project & go to Properties, then Configuration Properties.
1. Add the following to ```C/C++ > Additional Include Directories``` one at a time:
```
..\..\Awesomium\include
..\..\portaudio\include
..\..
```
1. Change ```C/C++ > Treat Warnings As Errors``` to ```No (/WX-)```.
1. Add the following to ```C/C++ > Preprocessor > Preprocessor Definitions```:
```
GLOWS_ENABLE=1
```
1. Add the following to ```Linker > Input > Additional Dependencies```:
```
..\..\Awesomium\lib\awesomium.lib
..\..\portaudio\lib\portaudio_x86.lib
..\..\sqlite\lib\sqlite3.lib
..\..\openvr\lib\openvr_api.lib
..\..\game\client\hlvr\d3d9.lib
```
1. Set ```Build Events > Pre-Build Events > Command Line``` to:
```
if EXIST ..\..\..\windows_content\frontend\bin\.\$(TargetFileName) for /f "delims=" %%A in ('attrib "..\..\..\windows_content\frontend\bin\.\$(TargetFileName)"') do set valveTmpIsReadOnly="%%A"
set valveTmpIsReadOnlyLetter=%valveTmpIsReadOnly:~6,1%
if "%valveTmpIsReadOnlyLetter%"=="R" del /q "$(TargetDir)"$(TargetFileName)
if exist "..\..\devtools\bin\vpc.exe" "..\..\devtools\bin\vpc.exe" -crc2 "client_hl2mp.vcxproj"
if ERRORLEVEL 1 exit /b 1
```
1. Set ```Build Events > Post-Build Events > Command Line``` to:
```
if not exist "..\..\..\windows_content\frontend\bin\." mkdir "..\..\..\windows_content\frontend\bin\."
copy "$(TargetDir)$(TargetFileName)" "..\..\..\windows_content\frontend\bin\.\$(TargetFileName)"
if ERRORLEVEL 1 goto BuildEventFailed
if exist "$(TargetDir)$(TargetName).map" copy "$(TargetDir)$(TargetName).map" ..\..\..\windows_content\frontend\bin\.\$(TargetName).map
copy "$(TargetDir)$(TargetName).pdb" ..\..\..\windows_content\frontend\bin\.\$(TargetName).pdb
if ERRORLEVEL 1 goto BuildEventFailed
goto BuildEventOK
:BuildEventFailed
echo *** ERROR! PostBuildStep FAILED for $(ProjectName)! EXE or DLL is probably running. ***
del /q "$(TargetDir)$(TargetFileName)"
exit 1
:BuildEventOK
```
1. Set ```Build Events > Post-Build Events > Description``` to: ```Publishing to ..\..\..\windows_content\frontend\bin\.```
1. Click ```OK``` to save your changes.
1. Add the following filters into the hierarchy of the ```Client (HL2MP)``` project:
| Filter  | Existing Items |
| ------------- |:-------------:|
| aarcade      | src/aarcade/client/*     |
| ges      | src/game/client/ges/c_ge_door_interp.cpp     |
| hlvr      | src/game/client/hlvr/vrmanager.cpp     |
1. Add the following Existing Item to the root of the Client (HL2MP) hierarchy as well: ```src/openvr/openvr.h```
1. Access the ```Server (HL2MP)``` Configuration Properties: Right-click on ```Server (HL2MP)``` project & go to Properties, then Configuration Properties.
1. Change ```C/C++ > Treat Warnings As Errors``` to ```No (/WX-)```.
1. Add the following to ```C/C++ > Preprocessor > Preprocessor Definitions```:
```
GLOWS_ENABLE=1
```
1. Set ```Build Events > Pre-Build Events > Command Line``` to:
```
if EXIST ..\..\..\windows_content\frontend\bin\.\$(TargetFileName) for /f "delims=" %%A in ('attrib "..\..\..\windows_content\frontend\bin\.\$(TargetFileName)"') do set valveTmpIsReadOnly="%%A"
set valveTmpIsReadOnlyLetter=%valveTmpIsReadOnly:~6,1%
if "%valveTmpIsReadOnlyLetter%"=="R" del /q "$(TargetDir)"$(TargetFileName)
if exist "..\..\devtools\bin\vpc.exe" "..\..\devtools\bin\vpc.exe" -crc2 "server_hl2mp.vcxproj"
if ERRORLEVEL 1 exit /b 1
```
1. Set ```Build Events > Post-Build Events > Command Line``` to:
```
if not exist "..\..\..\windows_content\frontend\bin\." mkdir "..\..\..\windows_content\frontend\bin\."
copy "$(TargetDir)$(TargetFileName)" "..\..\..\windows_content\frontend\bin\.\$(TargetFileName)"
if ERRORLEVEL 1 goto BuildEventFailed
if exist "$(TargetDir)$(TargetName).map" copy "$(TargetDir)$(TargetName).map" ..\..\..\windows_content\frontend\bin\.\$(TargetName).map
copy "$(TargetDir)$(TargetName).pdb" ..\..\..\windows_content\frontend\bin\.\$(TargetName).pdb
if ERRORLEVEL 1 goto BuildEventFailed
goto BuildEventOK
:BuildEventFailed
echo *** ERROR! PostBuildStep FAILED for $(ProjectName)! EXE or DLL is probably running. ***
del /q "$(TargetDir)$(TargetFileName)"
exit 1
:BuildEventOK
```
1. Set ```Build Events > Post-Build Events > Description``` to: ```Publishing to ..\..\..\windows_content\frontend\bin\.```
1. Click ```OK``` to save your changes.
1. Add the following filters into the hierarchy of the ```Server (HL2MP)```
| Filter  | Existing Items |
| ------------- |:-------------:|
| aarcade      | src/aarcade/server/*     |
| ges      | src/game/server/ges/ent/*     |
1. Compile ```games.sln```.
1. Success! You win! Next, try ```Run In Visual Studio Debugger```.
1. **ADVANCED USERS NOTE**: You only need to run ```games.sln```.  Advanced developers may choose to use ```everything.sln``` instead - but you'd have to configure the PREBUILD & POSTBUILD output directories for those projects as well to use ```windows_content/frontend``` instead of their default ```game/mod_hl2mp```.

## Run In Visual Studio Debugger

1. Clone the ```aarcade-source-release``` repo to your computer.
1. Copy the ```windows_content``` folder out of the repo & into the ```mp``` folder of your project, so that the following file exists: ```mp/windows_content/AArcade.exe```
1. You are now finished with the original ```aarcade-source-release``` repo.
1. Open up the ```games.sln``` solution.
1. Access the ```Client (HL2MP)``` Configuration Properties: Right-click on ```Client (HL2MP)``` project & go to Properties, then Configuration Properties.
1. Set the ```Debugging > Command``` value to be **your** absolute path to the AArcade.exe file you just copied into the ```mp/windows_content``` folder.  For example, ```C:\GitHub\aarcade-source-client\mp\windows_content\AArcade.exe```
1. Set the ```Debugging > Command Arguments``` value to the following:```-dev -game frontend -w 1920 -h 1080 -noborder -sw +mounts 0 +workshop 0```
1. You are now ready to try compiling & testing AArcade in the Visual Studio Debugger by pressing the F5 button.  But it is suggested you follow the ```Create Your First Console Command``` steps below to be sure that everything is working properly.

## Create Your First Console Command

1. Open the ```games.sln``` solution.
1. Open ```console.cpp``` in the ```Client (HL2MP)``` project.
1. Search the document for ```my_first_console_command```.
1. Read the comments in the function body.  Uncomment the ```DevMsg``` line, and customize the text with your own.
1. Save your changes.
1. Press F5 to build & launch AArcade in the debugger.
1. When you reach the ```Main Menu```, bring up the console with the ```~``` key, and type in the command ```developer 1``` followed by ```my_first_console_command```.
1. Success!  You will see the your custom text printed into the console.

## Join AArcade Community Discord
### Use this invite to join the AArcade Discord server: https://discord.gg/9FSCDuJ

# Original Source Engine License
```
                SOURCE 1 SDK LICENSE

Source SDK Copyright(c) Valve Corp.  

THIS DOCUMENT DESCRIBES A CONTRACT BETWEEN YOU AND VALVE 
CORPORATION ("Valve").  PLEASE READ IT BEFORE DOWNLOADING OR USING 
THE SOURCE ENGINE SDK ("SDK"). BY DOWNLOADING AND/OR USING THE 
SOURCE ENGINE SDK YOU ACCEPT THIS LICENSE. IF YOU DO NOT AGREE TO 
THE TERMS OF THIS LICENSE PLEASE DON’T DOWNLOAD OR USE THE SDK.  

  You may, free of charge, download and use the SDK to develop a modified Valve game 
running on the Source engine.  You may distribute your modified Valve game in source and 
object code form, but only for free. Terms of use for Valve games are found in the Steam 
Subscriber Agreement located here: http://store.steampowered.com/subscriber_agreement/ 

  You may copy, modify, and distribute the SDK and any modifications you make to the 
SDK in source and object code form, but only for free.  Any distribution of this SDK must 
include this LICENSE file and thirdpartylegalnotices.txt.  
 
  Any distribution of the SDK or a substantial portion of the SDK must include the above 
copyright notice and the following: 

    DISCLAIMER OF WARRANTIES.  THE SOURCE SDK AND ANY 
    OTHER MATERIAL DOWNLOADED BY LICENSEE IS PROVIDED 
    "AS IS".  VALVE AND ITS SUPPLIERS DISCLAIM ALL 
    WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS 
    OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED 
    WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, 
    TITLE AND FITNESS FOR A PARTICULAR PURPOSE.  

    LIMITATION OF LIABILITY.  IN NO EVENT SHALL VALVE OR 
    ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, 
    INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER 
    (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF 
    BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF 
    BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) 
    ARISING OUT OF THE USE OF OR INABILITY TO USE THE 
    ENGINE AND/OR THE SDK, EVEN IF VALVE HAS BEEN 
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
 
       
If you would like to use the SDK for a commercial purpose, please contact Valve at 
sourceengine@valvesoftware.com.
```