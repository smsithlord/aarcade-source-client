# Early Access
AArcade: Source is still in Early Access & actively being developed.  Thank you for considering contributingg to this project!  Please join the AArcade community in the Discord w/ the invite in the ```Join AArcade Community Discord``` section!

# Getting Started

## Install Microsoft Visual Studio Community 2013
**NOTE:** For additional resources on the Source engine, visit the [Valve Developer Wiki](https://developer.valvesoftware.com/wiki/Source_SDK_2013)

1) Have Steam installed & be logged in.
1. Download ```Visual Studio Community 2013 with Update 5``` [following the instructions on the wiki](https://developer.valvesoftware.com/wiki/Source_SDK_2013#Step_One:_Installing_Visual_Studio) (requires a Microsoft account). 
1. Extract the ISO into a folder & run the installer.
1. Have ```MFC``` checked - all other optional features can be unchecked.
1. Launch Visual Studio to make sure it works. (You will have to login.)  Then close it after you are sure it works.
1. Install the ```Multibyte MFC Library``` [from the wiki](https://download.microsoft.com/download/0/2/3/02389126-40A7-46FD-9D83-802454852703/vc_mbcsmfc.exe).

## Build AArcade Source Code

7) Clone the ```aarcade-source-client``` repo.
8) Run ```creategameprojects.bat```.
9) Open ```games.sln```.
10) Right-click on Solution (the top of the hierarchy) go to Properties, then Configuration Manager button and change the Active solution configuration from ```Debug``` to ```Release```.
11) Access the ```Client (HL2MP)``` Configuration Properties: Right-click on ```Client (HL2MP)``` project & go to Properties, then Configuration Properties.
12) Add the following to ```C/C++ > Additional Include Directories``` one at a time:
```
..\..\Awesomium\include
..\..\portaudio\include
..\..
```
13) Change ```C/C++ > Treat Warnings As Errors``` to ```No (/WX-)```.
14) Add the following to ```C/C++ > Preprocessor > Preprocessor Definitions```:
```
GLOWS_ENABLE=1
```
15) Add the following to ```Linker > Input > Additional Dependencies```:
```
..\..\Awesomium\lib\awesomium.lib
..\..\portaudio\lib\portaudio_x86.lib
..\..\sqlite\lib\sqlite3.lib
..\..\openvr\lib\openvr_api.lib
..\..\game\client\hlvr\d3d9.lib
```
16) Set ```Build Events > Pre-Build Events > Command Line``` to:
```
if EXIST ..\..\..\windows_content\frontend\bin\.\$(TargetFileName) for /f "delims=" %%A in ('attrib "..\..\..\windows_content\frontend\bin\.\$(TargetFileName)"') do set valveTmpIsReadOnly="%%A"
set valveTmpIsReadOnlyLetter=%valveTmpIsReadOnly:~6,1%
if "%valveTmpIsReadOnlyLetter%"=="R" del /q "$(TargetDir)"$(TargetFileName)
if exist "..\..\devtools\bin\vpc.exe" "..\..\devtools\bin\vpc.exe" -crc2 "client_hl2mp.vcxproj"
if ERRORLEVEL 1 exit /b 1
```
17) Set ```Build Events > Post-Build Events > Command Line``` to:
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
18) Set ```Build Events > Post-Build Events > Description``` to:
```Publishing to ..\..\..\windows_content\frontend\bin\.```
19) Copy the contents of a vanilla version of the "Anarchy Arcade" folder from Steam into the ```mp/windows_content``` folder of your project, for debugging purposes.
20) Set ```Debugging > Command``` to your AArcade.exe in your windows_content folder:
```$(ProjectDir)../../../windows_content/AArcade.exe```
21) Set ```Command Arguments``` to the following so you can debug w/o mounts nor workshop content:
```-dev -game frontend -w 1920 -h 1080 -sw -noborder +mounts 0 +workshop 0```
22) Click ```OK``` to save your changes.
23) Add the following filters into the hierarchy of the ```Client (HL2MP)``` project:

| Filter        | Existing Items                             |
| :------------ |:-------------------------------------------|
| aarcade       | `src/aarcade/client/*`                     |
| ges           | `src/game/client/ges/c_ge_door_interp.cpp` |
| hlvr          | `src/game/client/hlvr/vrmanager.cpp`       |

24) Add the following Existing Item to the root of the Client (HL2MP) hierarchy as well: ```src/openvr/openvr.h```
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

1. Get Anarchy Arcade on Steam & let it completely download.
1. Find your ```[...]/steamapps/common/Anarchy Arcade``` folder & copy the following 15 files out of it & into ```mp/windows_content``` in your project folder.
  - bin
  - config
  - frontend
  - hl2
  - hl2mp
  - platform
  - sdktools
  - sourcetest
  - AArcade.exe
  - arcade_launcher.pak
  - ingamedialogconfig.vdf
  - serverbrowser.vdf
  - stats.txt
  - steam_appid.txt
  - thirdpartylegalnotices.txt
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

# License
Original code & art assets created for AArcade are licensed with the [Mozilla Public License 2.0](https://github.com/mozilla/hubs/blob/master/LICENSE)

Non-original code & art assset used in AArcade are subject to their own licenses.

The Source Engine itself is licensed with the license in the ```Original Source Engine License``` section.

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

# Credits & Contributions
Thank you to everybody who has contributed to AArcade over the years!  Below is a list of contributions that are part of the repo itself. 

**NOTE** A list of community contributors (people who create tutorials, addons, etc.) will be listed in a different repo.

| Contributor  | Contribution |
| ------------- |:-------------:|
| Valve/Steam      | Source SDK Base 2013 (SP) Singleplayer
||Source SDK Base 2013 (MP) Multiplayer|
||Steamworks SDK (Workshop, stats, achievements, web browser.)|
||OpenVR|
|Awesomium| HTML Rendering |
|Libretro|Libretro frontend functionality. (ie. ability to load cores.)|
|PortAudio|Plays audio from Libretro.|
|HLVR|Modified d3d9.dll that supports VR.
|GoldenEye: Source|Logic for doors to better support GE: Source maps.|
|CJ Collard|Created the stock maps sm_primo, sm_acreage, dralloc_gallery.|
|Zoey Robertson|Provided original tutorial voice acting.|
|Drew Peterson|Created the original Main Menu music.|
||Created original real-time pixel resizing logic for render-to-texture of videos.|
|Chris Sullivan|Beta tested nightly builds of the game leading up to initial release.|
|Bucky|Created the sm_apartmentsuite map.
||Contributed various Source engine tweaks to improve performance & compatibility.|
|Black_Stormy|Created an arcade cabinet bashkit to create derivative cabinet models with.|
||Created about a dozen cabinets from the bashkit.|
||Created hub_highrise & contributed various other Double Action assets.|
|Carlos Andrade|Created the original Anarchy Arcade logo.|
|OkeDoke|Contributed arcade cabinet models.|
|Invict|Contributed arcade cabinet models.|
|TurboSquid|Change Machine 02 (Dive Bar) - PBR Game Ready|
|Elijah Newman-Gomez|All other coding & design.|
