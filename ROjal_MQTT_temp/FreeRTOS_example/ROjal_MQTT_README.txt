ROjal_MQTT in modified FreeRTOS + TCP 9.0.0 example
FreeRTOS is running in Windows 10 emulator and using Visual Studio tools (both Visual Studio and
Visual Studio Community edition tested)
Visual Studio project file is located in
FreeRTOS-Plus\Demo\FreeRTOS_Plus_TCP_and_FAT_Windows_Simulator\FreeRTOS_Plus_TCP_and_FAT.sln
See ReadMe.txt for more information about how to build FreeRTOS with Visual Studio.

To get network working you need to set following define in FreeRTOSConfig.h file to correct. It might
require Hyper-v's network adapter, which can be turned on from windows 10 additional features. 
configNETWORK_INTERFACE_TO_USE
