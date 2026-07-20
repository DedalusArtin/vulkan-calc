; Inno Setup 安装包脚本 — VulkanCalc 2.0
; 使用 Inno Setup 6 (https://jrsoftware.org/isinfo.php) 编译

#define MyAppName "VulkanCalc"
#define MyAppFullName "VulkanCalc 2.0"
#define MyAppVersion "2.0.0"
#define MyAppPublisher "VulkanCalc Team"
#define MyAppURL "https://github.com/DedalusArtin/vulkan-calc"
#define MyAppExeName "vulkan_calc.exe"
#define MyAppIcoName "app.ico"

[Setup]
; 基本设置
AppName={#MyAppFullName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
; 输出设置
OutputDir=F:\vulkan-calc\installer
OutputBaseFilename=VulkanCalc-2.0.0-Setup
; 图标
SetupIconFile=icon\{#MyAppIcoName}
UninstallDisplayIcon={app}\{#MyAppIcoName}
; 压缩
Compression=lzma2/max
SolidCompression=yes
; 权限
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
; 中文界面
LanguageDetectionMethod=locale
ShowLanguageDialog=auto

[Languages]
; 简体中文为主，英文为备用
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; 主程序
Source: "F:\vulkan-calc\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
; 字体文件
Source: "F:\vulkan-calc\fonts\NotoSansCJK-Regular.ttc"; DestDir: "{app}\fonts"; Flags: ignoreversion
; 图标
Source: "icon\{#MyAppIcoName}"; DestDir: "{app}"; Flags: ignoreversion
; DLL运行时库（从vcpkg安装目录复制）
Source: "E:\Programming\vcpkg\installed\x64-windows\bin\glfw3.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "E:\Programming\vcpkg\installed\x64-windows\bin\glm.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
; Vulkan DLL
Source: "E:\Programming\vulkan\1.4.350.0\Bin\vulkan-1.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
; 其他运行时库
Source: "F:\vulkan-calc\*.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Dirs]
Name: "{app}"; Permissions: users-modify

[Icons]
; 开始菜单快捷方式
Name: "{group}\{#MyAppFullName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\{#MyAppIcoName}"; Comment: "VulkanCalc 2.0 科学计算器"
Name: "{group}\卸载 {#MyAppName}"; Filename: "{uninstallexe}"; IconFilename: "{app}\{#MyAppIcoName}"
; 桌面快捷方式
Name: "{autodesktop}\{#MyAppFullName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\{#MyAppIcoName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式(&D)"; GroupDescription: "快捷方式："; Flags: checkedonce

[Run]
; 安装完成后自动运行
Filename: "{app}\{#MyAppExeName}"; Description: "运行 VulkanCalc 2.0"; Flags: postinstall nowait skipifsilent shellexec; WorkingDir: "{app}"

[UninstallRun]
; 卸载前关闭程序（如果正在运行）
Filename: "{sys}\taskkill.exe"; Parameters: "/f /im {#MyAppExeName}"; Flags: runhidden skipifdoesntexist

[UninstallDelete]
; 卸载后删除用户配置（可选）
Type: filesandordirs; Name: "{localappdata}\VulkanCalc"

[Code]
// 检查 Vulkan 运行时是否安装（非强制，仅提示）
function IsVulkanInstalled: Boolean;
var
  VkDllPath: String;
begin
  VkDllPath := ExpandConstant('{sys}\vulkan-1.dll');
  if FileExists(VkDllPath) then
    Result := True
  else begin
    // 也检查 System32
    VkDllPath := ExpandConstant('{syswow64}\vulkan-1.dll');
    Result := FileExists(VkDllPath);
  end;
end;

function InitializeSetup: Boolean;
begin
  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ErrorCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    if not IsVulkanInstalled then
    begin
      if MsgBox('未检测到 Vulkan Runtime。程序需要 Vulkan 1.3+ 才能运行。'#13#13 +
                 '是否前往 Vulkan SDK 官网下载？',
                 mbConfirmation, MB_YESNO) = IDYES then
      begin
        ShellExec('open', 'https://vulkan.lunarg.com/sdk/home', '', '', SW_SHOW, ewNoWait, ErrorCode);
      end;
    end;
  end;
end;
