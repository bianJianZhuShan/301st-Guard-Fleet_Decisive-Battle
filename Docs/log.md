LogUObjectHash: Compacting FUObjectHashTables data took   0.52ms
LogPSOHitching: Encountered 100 PSO creation hitches so far (11 graphics, 89 compute). 0 of them were precached.
LogTurnkeySupport: Selected target: newProject
LogLauncherProfile: Unable to use promoted target - ../../../../project/newProject/Binaries/Win64/newProject.target does not exist.
LogMonitoredProcess: Running Serialized UAT: [ cmd.exe /c ""D:/ue/UE_5.7/Engine/Build/BatchFiles/RunUAT.bat"  -ScriptsForProject="D:/ue/project/newProject/newProject.uproject" Turnkey -command=VerifySdk -platform=Win64 -UpdateIfNeeded -EditorIO -EditorIOPort=59576  -project="D:/ue/project/newProject/newProject.uproject" BuildCookRun -nop4 -utf8output -nocompileeditor -skipbuildeditor -cook  -project="D:/ue/project/newProject/newProject.uproject" -target=newProject  -unrealexe="D:\ue\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" -platform=Win64 -installed -SkipCookingErrorSummary -JsonStdO
ut -stage -archive -package -build -pak -iostore -compressed -prereqs -archivedirectory="D:/ue/project/test" -clientconfig=Development" -nocompile -nocompileuat ]
LogAutomationController: Ignoring very large delta of 2.61 seconds in calls to FAutomationControllerManager::Tick() and not penalizing unresponsive tests
UATHelper: 打包 (Windows): Running AutomationTool...
UATHelper: 打包 (Windows): Using bundled DotNet SDK version: 8.0.412 win-x64
UATHelper: 打包 (Windows): Starting AutomationTool...
UATHelper: 打包 (Windows): Parsing command line: -ScriptsForProject=D:/ue/project/newProject/newProject.uproject Turnkey -command=VerifySdk -platform=Win64 -UpdateIfNeeded -EditorIO -EditorIOPort=59576 -project=D:/ue/project/newProject/newProject.uproject BuildCookRun -nop4 -utf8output -nocompileeditor -skipbuildeditor -cook -project=D:/ue/project/newProject/newProject.uproject -target=newProject -unrealexe=D:\ue\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe -platform=Win64 -installed -SkipCookingErrorSummary -JsonStdOut -stage -archive -package -build -pak -iostore -compressed -prereqs -arc
hivedirectory=D:/ue/project/test -clientconfig=Development -nocompile -nocompileuat
LogHttp: Warning: HTTP request timed out after 3.00 seconds URL=https://www.google.com/generate_204
UATHelper: 打包 (Windows): Initializing script modules...
UATHelper: 打包 (Windows): Total script module initialization time: 0.98 s.
UATHelper: 打包 (Windows): Using D:\ue\bjq\MSBuild\Current\Bin\MSBuild.exe
UATHelper: 打包 (Windows): Executing commands...
UATHelper: 打包 (Windows): Installed Sdk validity:
UATHelper: 打包 (Windows): Win64: (Status=Valid, MinAllowed_Sdk=10.0.19041.0, MaxAllowed_Sdk=10.9.99999.0, Current_Sdk=10.0.22621.0, Allowed_AutoSdk=10.0.22621.0, Current_AutoSdk=, Flags="InstalledSdk_ValidVersionExists, Sdk_HasBestVersion")
UATHelper: 打包 (Windows): Scanning for envvar changes...
UATHelper: 打包 (Windows): ... done! 
UATHelper: 打包 (Windows): Cleaning Temp Paths...
UATHelper: 打包 (Windows): BUILD SUCCESSFUL
UATHelper: 打包 (Windows): Setting up ProjectParams for D:\ue\project\newProject\newProject.uproject
UATHelper: 打包 (Windows): ********** BUILD COMMAND STARTED **********
UATHelper: 打包 (Windows): Running: D:\ue\UE_5.7\Engine\Binaries\ThirdParty\DotNet\8.0.412\win-x64\dotnet.exe "D:\ue\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" newProject Win64 Development -Project=D:\ue\project\newProject\newProject.uproject  -remoteini="D:\ue\project\newProject"  -skipdeploy  -log="C:\Users\ZhuanZ（无密码）\AppData\Roaming\Unreal Engine\AutomationTool\Logs\D+ue+UE_5.7\UBA-newProject-Win64-Development.txt"
UATHelper: 打包 (Windows): Log file: C:\Users\ZhuanZ�������룩\AppData\Roaming\Unreal Engine\AutomationTool\Logs\D+ue+UE_5.7\UBA-newProject-Win64-Development.txt
UATHelper: 打包 (Windows): Building newProject...
UATHelper: 打包 (Windows): Using Visual Studio 2022 14.44.35221 toolchain (D:\ue\bjq\VC\Tools\MSVC\14.44.35207) and Windows 10.0.22621.0 SDK (C:\Program Files (x86)\Windows Kits\10).
UATHelper: 打包 (Windows): Determining max actions to execute in parallel (6 physical cores, 12 logical cores)
UATHelper: 打包 (Windows):   Executing up to 6 processes, one per physical core
UATHelper: 打包 (Windows):   Requested 1.5 GB memory per action, 5.87 GB available: limiting max parallel actions to 3
UATHelper: 打包 (Windows): Using Unreal Build Accelerator local executor to run 3 action(s)
UATHelper: 打包 (Windows):   Storage capacity 40Gb
UATHelper: 打包 (Windows): ---- Starting trace: 251225_181434 ----
UATHelper: 打包 (Windows): UbaServer - Listening on 0.0.0.0:1345
UATHelper: Error: 打包 (Windows): [1/3] Compile [x64] UnitActor.cpp
UATHelper: Error: 打包 (Windows): D:\ue\project\newProject\Source\newProject\UnitActor.cpp(50,21): error C2039: "bDisplayVertexColors": ���� "UStaticMeshComponent" �ĳ�Ա
UATHelper: Error: 打包 (Windows):   CubeMeshComponent->bDisplayVertexColors = false;
UATHelper: Error: 打包 (Windows):                      ^
UATHelper: Error: 打包 (Windows): D:\ue\UE_5.7\Engine\Source\Runtime\Engine\Classes\Components\StaticMeshComponent.h(102,7): note: �μ���UStaticMeshComponent��������
UATHelper: Error: 打包 (Windows): class UStaticMeshComponent : public UMeshComponent
UATHelper: Error: 打包 (Windows):       ^
UATHelper: 打包 (Windows): Trace written to file C:/Users/ZhuanZ�������룩/AppData/Roaming/Unreal Engine/AutomationTool/Logs/D+ue+UE_5.7/UBA-newProject-Win64-Development.uba with size 2.7kb
UATHelper: 打包 (Windows): Total time in Unreal Build Accelerator local executor: 2.05 seconds
LogSlate: Warning: Could not find Glyph Index 0 with codepoint U+531, getting last resort font data ../../../Engine/Content/Slate/Fonts/DroidSansFallback.ttf
UATHelper: 打包 (Windows): 
UATHelper: 打包 (Windows): Result: Failed (OtherCompilationError)
UATHelper: 打包 (Windows): Total execution time: 4.54 seconds
UATHelper: 打包 (Windows): Took 5.57s to run dotnet.exe, ExitCode=6
UATHelper: 打包 (Windows): UnrealBuildTool failed. See log for more details. (C:\Users\ZhuanZ（无密码）\AppData\Roaming\Unreal Engine\AutomationTool\Logs\D+ue+UE_5.7\UBA-newProject-Win64-Development.txt)
UATHelper: 打包 (Windows): AutomationTool executed for 0h 0m 18s
UATHelper: 打包 (Windows): AutomationTool exiting with ExitCode=6 (6)
UATHelper: 打包 (Windows): BUILD FAILED
PackagingResults: Error: Unknown Error
LogHttp: Warning: HTTP request timed out after 3.00 seconds URL=https://www.google.com/generate_204
LogHttp: Warning: HTTP request timed out after 3.00 seconds URL=https://www.google.com/generate_204