<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN" sourcelanguage="en">
<context>
    <name>Aurora::AboutPage</name>
    <message>
        <location filename="../src/ui/pages/AboutPage.cpp" line="+73"/>
        <source>About</source>
        <translation>关于</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Unofficial, fan-made desktop client for your self-hosted Immich library.</source>
        <translation>用于自托管 Immich 媒体库的非官方、爱好者制作桌面客户端。</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>What is this?</source>
        <translation>这是什么？</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>immich desktop is an unofficial, fan-made desktop app for &lt;a href=&quot;https://immich.app&quot;&gt;Immich&lt;/a&gt; , the self-hosted photo and video manager. It connects to your own Immich server via an API key and lets you browse your timeline, search, explore people &amp;amp; places, upload/download, stream video, and keep browsing offline through a local thumbnail cache and queued uploads. It is not affiliated with, maintained by, or endorsed by the official Immich project.</source>
        <translation>immich desktop 是一款面向 &lt;a href=&quot;https://immich.app&quot;&gt;Immich&lt;/a&gt;（自托管照片与视频管理器）的非官方、爱好者制作桌面应用。它通过 API 密钥连接到你自己的 Immich 服务器，让你可以浏览时间线、搜索、探索人物与地点、上传/下载、流式播放视频，并通过本地缩略图缓存和排队上传保持离线浏览。它与官方 Immich 项目无关，不由其维护，也未获得其认可。</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Who makes it?</source>
        <translation>由谁开发？</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Built and maintained by the Immich Desktop community, led by &lt;a href=&quot;https://github.com/hdmain&quot;&gt;hdmain&lt;/a&gt;. Major contributor: &lt;a href=&quot;https://github.com/gardar&quot;&gt;gardar&lt;/a&gt; (viewer navigation, zoomable grids, collapsible sidebar, info panel, and more). Contributions, bug reports, and ideas are welcome on GitHub.</source>
        <translation>由 Immich Desktop 社区构建和维护，由 &lt;a href=&quot;https://github.com/hdmain&quot;&gt;hdmain&lt;/a&gt; 领导。主要贡献者：&lt;a href=&quot;https://github.com/gardar&quot;&gt;gardar&lt;/a&gt;（查看器导航、可缩放网格、可折叠侧边栏、信息面板等）。欢迎在 GitHub 上提交贡献、错误报告和想法。</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>&lt;a href=&quot;https://github.com/%1&quot;&gt;github.com/%1&lt;/a&gt; · &lt;a href=&quot;https://github.com/%1/issues&quot;&gt;Issues&lt;/a&gt; · &lt;a href=&quot;https://github.com/%1/releases&quot;&gt;Releases&lt;/a&gt;</source>
        <translation>&lt;a href=&quot;https://github.com/%1&quot;&gt;github.com/%1&lt;/a&gt; · &lt;a href=&quot;https://github.com/%1/issues&quot;&gt;问题&lt;/a&gt; · &lt;a href=&quot;https://github.com/%1/releases&quot;&gt;发行版&lt;/a&gt;</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Tech stack</source>
        <translation>技术栈</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Everything this build is made with , frameworks, packaging, and key libraries:</source>
        <translation>此版本用到的全部技术，框架、打包方式和关键库：</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Qt 6 - Core, Gui, Widgets, Network, Svg</source>
        <translation>Qt 6：Core、Gui、Widgets、Network、Svg</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Qt Multimedia (+ MultimediaWidgets) - video playback &amp; audio (QMediaPlayer, QVideoSink)</source>
        <translation>Qt Multimedia（+ MultimediaWidgets）：视频播放与音频（QMediaPlayer、QVideoSink）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>GStreamer / FFmpeg - media backends behind Qt Multimedia (selected at runtime)</source>
        <translation>GStreamer / FFmpeg：Qt Multimedia 背后的媒体后端（运行时选择）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>CMake + Ninja - build system</source>
        <translation>CMake + Ninja：构建系统</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>C++20 - language standard</source>
        <translation>C++20：语言标准</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>QSettings (INI) - AppSettings persistence (appearance, updates, server, playback)</source>
        <translation>QSettings (INI)：AppSettings 持久化（外观、更新、服务器、播放）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>QSystemTrayIcon - tray, notifications, close-to-tray</source>
        <translation>QSystemTrayIcon：托盘、通知、关闭到托盘</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>QLocalServer / QLocalSocket - SingleInstance (single-instance + raise)</source>
        <translation>QLocalServer / QLocalSocket：SingleInstance（单实例 + 唤起窗口）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Inter font family - bundled UI typeface (via FontLoader)</source>
        <translation>Inter 字体系列：内置界面字体（通过 FontLoader）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Lucide icons - SVG icon set</source>
        <translation>Lucide 图标：SVG 图标集</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>linuxdeploy + linuxdeploy-plugin-qt - AppImage bundling</source>
        <translation>linuxdeploy + linuxdeploy-plugin-qt：AppImage 打包</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>CPack - DEB / NSIS / WiX (MSI) installers</source>
        <translation>CPack：DEB / NSIS / WiX (MSI) 安装程序</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Snapcraft (core24 + gnome extension) - Snap package</source>
        <translation>Snapcraft（core24 + gnome 扩展）：Snap 软件包</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Flatpak (org.kde.Platform / Sdk) - Flatpak manifest</source>
        <translation>Flatpak（org.kde.Platform / Sdk）：Flatpak 清单</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>GitHub Actions - CI for Windows/Linux/Snap + tagged releases</source>
        <translation>GitHub Actions：适用于 Windows/Linux/Snap 的 CI + 带标签的发行版</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>NSIS &amp; WiX Toolset - Windows installers</source>
        <translation>NSIS 与 WiX Toolset：Windows 安装程序</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Get in touch &amp; collaborate</source>
        <translation>联系与协作</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Want to build this together, report a bug, or propose a feature? Reach out on GitHub or Discord , contributions of all sizes are welcome.</source>
        <translation>想一起开发、报告错误或提出功能建议吗？可通过 GitHub 或 Discord 联系我们，欢迎任何规模的贡献。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Discord: diegosanche3</source>
        <translation>Discord：diegosanche3</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Copy Discord handle</source>
        <translation>复制 Discord 用户名</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Open GitHub</source>
        <translation>打开 GitHub</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Discord handle: diegosanche3</source>
        <translation>Discord 用户名：diegosanche3</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Version</source>
        <translation>版本</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>v%1 (Snap %2)</source>
        <translation>v%1（Snap %2）</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Licensed under MIT. See LICENSE.txt. Immich is a trademark of its owners; this app is an independent project and does not redistribute official Immich branding.</source>
        <translation>依据 MIT 许可证授权。参见 LICENSE.txt。Immich 是其所有者的商标；本应用是独立项目，不重新分发官方 Immich 品牌资源。</translation>
    </message>
</context>
<context>
    <name>Aurora::AppearancePage</name>
    <message>
        <location filename="../src/ui/pages/AppearancePage.cpp" line="+90"/>
        <source>Close to system tray</source>
        <translation>关闭到系统托盘</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Start immich when I sign in</source>
        <translation>登录时启动 immich</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Choose a preset or build a palette that belongs to your product.</source>
        <translation>选择一个预设，或构建属于你的产品的调色板。</translation>
    </message>
    <message>
        <location line="+16"/>
        <location line="+5"/>
        <source>Language</source>
        <translation>语言</translation>
    </message>
    <message>
        <location line="-4"/>
        <source>Language changes apply immediately. Untranslated text appears in English.</source>
        <translation>语言更改立即生效。未翻译的文本将显示为英文。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>System default</source>
        <translation>系统默认</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Theme preset</source>
        <translation>主题预设</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Preset changes are applied immediately and saved automatically.</source>
        <translation>预设更改立即生效并自动保存。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Light</source>
        <translation>浅色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark</source>
        <translation>深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Custom</source>
        <translation>自定义</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Icons and interface</source>
        <translation>图标与界面</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Choose an icon pack and simplify action buttons. Changes are applied immediately and saved automatically.</source>
        <translation>选择图标包并简化操作按钮。更改立即生效并自动保存。</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+3"/>
        <source>Icon pack</source>
        <translation>图标包</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Lucide (default)</source>
        <translation>Lucide（默认）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Tabler</source>
        <translation>Tabler</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bootstrap Icons</source>
        <translation>Bootstrap Icons</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Preview: upload, library, settings</source>
        <translation>预览：上传、媒体库、设置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Minimalist UI (icons instead of button labels)</source>
        <translation>极简界面（用图标代替按钮文字）</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Navigation and action buttons use icons. Hover over an icon to see its action.</source>
        <translation>导航和操作按钮使用图标。悬停图标可查看其功能。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Custom palette</source>
        <translation>自定义调色板</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Choosing a color automatically activates the Custom theme.</source>
        <translation>选择颜色会自动激活自定义主题。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Background</source>
        <translation>背景</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Panels</source>
        <translation>面板</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Buttons</source>
        <translation>按钮</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Borders</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Muted text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Accent</source>
        <translation>强调色</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Reset custom colors</source>
        <translation>重置自定义颜色</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Desktop</source>
        <translation>桌面</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Control how immich behaves on your desktop and at sign-in.</source>
        <translation>控制 immich 在桌面和登录时的行为。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Closing the window keeps immich running in the tray. Press Ctrl+Q to quit.</source>
        <translation>关闭窗口后 immich 会继续在托盘中运行。按 Ctrl+Q 可退出。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>System tray is not available on this desktop.</source>
        <translation>此桌面环境不支持系统托盘。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Autostart is not supported on this platform.</source>
        <translation>此平台不支持自动启动。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Playback</source>
        <translation>播放</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Control preview and playback behavior.</source>
        <translation>控制预览和播放行为。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable video hover preview</source>
        <translation>启用视频悬停预览</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Plays a short muted clip when you hover a video tile. Disable this if hovering causes freezes or stutter.</source>
        <translation>悬停视频缩略图时播放一段短静音片段。如果悬停导致卡顿或掉帧，请禁用此功能。</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Note: the Snap build runs confined, so hover preview is disabled in the Snap. If video playback doesn&apos;t work from the Snap, install the GitHub release (&lt;a href=&quot;https://github.com/hdmain/immich-desktop/releases&quot;&gt;.deb / AppImage&lt;/a&gt;) , it uses your system&apos;s GStreamer/FFmpeg without confinement.</source>
        <translation>注意：Snap 版本运行在受限环境中，因此悬停预览在 Snap 中被禁用。如果 Snap 中无法播放视频，请安装 GitHub 发行版（&lt;a href=&quot;https://github.com/hdmain/immich-desktop/releases&quot;&gt;.deb / AppImage&lt;/a&gt;），它会在不受限制的情况下使用你系统的 GStreamer/FFmpeg。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Hover preview is unavailable in the Snap build.</source>
        <translation>Snap 版本不支持悬停预览。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Timeline</source>
        <translation>时间线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Choose how your library is laid out.</source>
        <translation>选择媒体库的布局方式。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Compact grid (no date headers)</source>
        <translation>紧凑网格（无日期标题）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Uniform square thumbnails packed edge to edge, like a phone&apos;s photo grid, instead of date-grouped rows of varying sizes.</source>
        <translation>统一的正方形缩略图边缘紧贴排列，类似手机的照片网格，而非按日期分组、大小不一的行。</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Cache</source>
        <translation>缓存</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Local storage used for thumbnails and offline library data.</source>
        <translation>用于缩略图和离线媒体库数据的本地存储。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Refresh</source>
        <translation>刷新</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Cache size: %1</source>
        <translation>缓存大小：%1</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Autostart</source>
        <translation>自动启动</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Could not update autostart settings.</source>
        <translation>无法更新自动启动设置。</translation>
    </message>
</context>
<context>
    <name>Aurora::AssetInfoPanel</name>
    <message>
        <location filename="../src/ui/widgets/AssetInfoPanel.cpp" line="+55"/>
        <source>Info</source>
        <translation>信息</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Hide info (I)</source>
        <translation>隐藏信息（I）</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>Details unavailable
%1</source>
        <translation>详细信息不可用
%1</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Albums need the album.read permission</source>
        <translation>查看相册需要 album.read 权限</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Albums unavailable</source>
        <translation>相册不可用</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Loading details…</source>
        <translation>正在加载详细信息…</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Show photos from this day</source>
        <translation>查看这一天的照片</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Date taken</source>
        <translation>拍摄日期</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>%1 MP</source>
        <translation>%1 MP</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>File</source>
        <translation>文件</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Camera</source>
        <translation>相机</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Show photos from %1</source>
        <translation>查看来自 %1 的照片</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Map coming soon</source>
        <translation>地图即将推出</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Location</source>
        <translation>位置</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Shared by</source>
        <translation>分享者</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Shared by %1</source>
        <translation>由 %1 分享</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+39"/>
        <source>People</source>
        <translation>人物</translation>
    </message>
    <message>
        <location line="-38"/>
        <source>No people recognized</source>
        <translation>未识别到人物</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Show photos of %1</source>
        <translation>查看 %1 的照片</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>this person</source>
        <translation>此人</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Unnamed</source>
        <translation>未命名</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Description</source>
        <translation>描述</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>No description</source>
        <translation>无描述</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Loading albums…</source>
        <translation>正在加载相册…</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Albums</source>
        <translation>相册</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Show photos tagged %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Tags</source>
        <translation>标签</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Show the album %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-47"/>
        <source>No tags</source>
        <translation>无标签</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Show the photos in this folder</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Folder</source>
        <translation>文件夹</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Not in any album</source>
        <translation>不在任何相册中</translation>
    </message>
</context>
<context>
    <name>Aurora::BrowseTree</name>
    <message>
        <location filename="../src/ui/widgets/BrowseTree.cpp" line="+50"/>
        <location line="+125"/>
        <source>Loading…</source>
        <translation type="unfinished">正在加载…</translation>
    </message>
    <message>
        <location line="-94"/>
        <source>No albums</source>
        <translation type="unfinished"></translation>
    </message>
    <message numerus="yes">
        <location line="+5"/>
        <source>%1 · %n item(s)</source>
        <translation type="unfinished">
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location line="+12"/>
        <source>No tags</source>
        <translation type="unfinished">无标签</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>No folders</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+42"/>
        <source>Needs the %1 permission · click to retry</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Couldn&apos;t be loaded · click to retry</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Aurora::ColorButton</name>
    <message>
        <location filename="../src/ui/widgets/ColorButton.cpp" line="+59"/>
        <source>Choose %1 color</source>
        <translation>选择 %1 颜色</translation>
    </message>
</context>
<context>
    <name>Aurora::ConnectionPage</name>
    <message>
        <location filename="../src/ui/pages/ConnectionPage.cpp" line="+34"/>
        <source>Enable custom HTTP headers</source>
        <translation>启用自定义 HTTP 标头</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable client certificate</source>
        <translation>启用客户端证书</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Client key</source>
        <translation>客户端密钥</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Remember password</source>
        <translation>记住密码</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Save &amp; test connection</source>
        <translation>保存并测试连接</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Immich server</source>
        <translation>Immich 服务器</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Connect this desktop client to your Immich instance. The API key needs user.read, asset.read, asset.view, asset.upload, asset.download, asset.delete, and person.read permissions; album.read, tag.read and folder.read are optional and enable the Albums, Tags and Folders sections (album.read also lists a photo&apos;s albums). An optional local URL is preferred automatically when reachable.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Optional - e.g. http://192.168.1.10:2283</source>
        <translation>可选，例如 http://192.168.1.10:2283</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Paste an Immich API key</source>
        <translation>粘贴 Immich API 密钥</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>PKCS#12 (.p12/.pfx)</source>
        <translation>PKCS#12（.p12/.pfx）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>PEM certificate + key</source>
        <translation>PEM 证书 + 密钥</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>PEM files (*.pem *.crt *.cer *.key)</source>
        <translation>PEM 文件（*.pem *.crt *.cer *.key）</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Browse…</source>
        <translation>浏览…</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Select client certificate</source>
        <translation>选择客户端证书</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>PKCS#12 bundles (*.p12 *.pfx)</source>
        <translation>PKCS#12 证书包（*.p12 *.pfx）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Select client key</source>
        <translation>选择客户端密钥</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Select CA certificate</source>
        <translation>选择 CA 证书</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Immich URL</source>
        <translation>Immich 地址</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Local URL</source>
        <translation>本地地址</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>API Key</source>
        <translation>API 密钥</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Sent with every request to your Immich server, including the local URL and video playback, but not to third-party CDN addresses. Values are stored locally in this app&apos;s plaintext settings file.</source>
        <translation>这些标头会随发往 Immich 服务器的每个请求一起发送，包括本地地址和视频播放请求，但不会发送到第三方 CDN 地址。值以明文保存在本应用的本地设置文件中。</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Add header</source>
        <translation>添加标头</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Format</source>
        <translation>格式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Client cert</source>
        <translation>客户端证书</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Server CA</source>
        <translation>服务器 CA</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Certificate that signed the server (or its reverse proxy). Needed when the server uses a private/internal CA. This is separate from the client certificate above and is not taken from the bundle.</source>
        <translation>签署服务器（或其反向代理）的证书。当服务器使用私有/内部 CA 时需要此项。它与上面的客户端证书相互独立，不会从捆绑包中提取。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Key password</source>
        <translation>密钥密码</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>The API key is stored locally in this app&apos;s settings and is never sent anywhere except your configured Immich server. When a local URL is set, the app probes it every few seconds and uses it while available.</source>
        <translation>API 密钥仅保存在本应用的本地设置中，除你配置的 Immich 服务器外，绝不会发送到任何地方。设置本地地址后，应用会每隔几秒探测一次，并在可用时使用它。</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Enter your server details to load the media library.</source>
        <translation>输入服务器信息以加载媒体库。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Save as new profile…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+68"/>
        <source>Delete profile…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-65"/>
        <location line="+42"/>
        <location line="+4"/>
        <source>New profile</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-46"/>
        <location line="+46"/>
        <location line="+14"/>
        <source>Profile name</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-41"/>
        <location line="+32"/>
        <source>User profiles</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-29"/>
        <source>Each profile has its own connection, library cache, upload queue and folder sync settings. Switching pauses the previous profile&apos;s uploads.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Active profile</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+23"/>
        <location line="+186"/>
        <source>Could not save the profile.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-180"/>
        <source>Rename</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Rename profile</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+16"/>
        <location line="+4"/>
        <location line="+3"/>
        <location line="+3"/>
        <source>Delete profile</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Switch to another profile before deleting this one.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Choose an inactive profile</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Delete %1 and its saved connection settings?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Could not delete the profile.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Use this profile on launch</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Header name</source>
        <translation>标头名称</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>value</source>
        <translation>值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Header value</source>
        <translation>标头值</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+1"/>
        <source>Remove header</source>
        <translation>移除标头</translation>
    </message>
    <message>
        <location line="+50"/>
        <source>Enter an HTTP(S) Immich URL without embedded credentials.</source>
        <translation>请输入不含内嵌凭据的 HTTP(S) Immich 地址。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Enter an API key.</source>
        <translation>请输入 API 密钥。</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Local URL must be HTTP(S) and must not contain embedded credentials.</source>
        <translation>本地地址必须为 HTTP(S) 且不得包含内嵌凭据。</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Testing connection…</source>
        <translation>正在测试连接…</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>✓ %1</source>
        <translation>✓ %1</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Could not connect: %1</source>
        <translation>无法连接：%1</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Using local endpoint: %1</source>
        <translation>正在使用本地端点：%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Using remote endpoint: %1</source>
        <translation>正在使用远程端点：%1</translation>
    </message>
</context>
<context>
    <name>Aurora::DashboardPage</name>
    <message>
        <location filename="../src/ui/pages/DashboardPage.cpp" line="+46"/>
        <source>A clean foundation for what comes next.</source>
        <translation>为后续发展打下干净的基础。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Add product modules without coupling them to navigation or appearance.</source>
        <translation>添加产品模块，同时避免它们与导航或外观耦合。</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>SYSTEM</source>
        <translation>系统</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Runtime themes</source>
        <translation>运行时主题</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The entire interface responds instantly to palette changes.</source>
        <translation>整个界面会即时响应调色板更改。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>ARCHITECTURE</source>
        <translation>架构</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Modular by design</source>
        <translation>模块化设计</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+3"/>
        <source>WORKSPACE</source>
        <translation>工作区</translation>
    </message>
    <message>
        <location line="-3"/>
        <location line="+3"/>
        <source>Future module</source>
        <translation>未来模块</translation>
    </message>
    <message>
        <location line="-2"/>
        <source>This card can become analytics, media, account, or workflow functionality.</source>
        <translation>这张卡片可以变成分析、媒体、账户或工作流功能。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Use the central theme manager to keep every new surface visually consistent.</source>
        <translation>使用中央主题管理器，让每个新增界面保持视觉一致。</translation>
    </message>
</context>
<context>
    <name>Aurora::ExplorePage</name>
    <message>
        <location filename="../src/ui/pages/ExplorePage.cpp" line="+189"/>
        <location line="+730"/>
        <source>Explore</source>
        <translation>探索</translation>
    </message>
    <message>
        <location line="-728"/>
        <source>People, places, and recent media</source>
        <translation>人物、地点和最近添加的媒体</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+3"/>
        <location line="+1"/>
        <location line="+177"/>
        <source>Refresh</source>
        <translation>刷新</translation>
    </message>
    <message>
        <location line="-141"/>
        <location line="+659"/>
        <location line="+182"/>
        <source>People</source>
        <translation>人物</translation>
    </message>
    <message>
        <location line="-840"/>
        <source>Places</source>
        <translation>地点</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Recently added</source>
        <translation>最近添加</translation>
    </message>
    <message>
        <location line="+51"/>
        <location line="+10"/>
        <source>Restore all</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+14"/>
        <source>Empty trash</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-5"/>
        <source>Restore everything in the trash to your library?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Permanently delete everything in the trash? This can&apos;t be undone.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+32"/>
        <location line="+1230"/>
        <source>Load more</source>
        <translation>加载更多</translation>
    </message>
    <message>
        <location line="-1074"/>
        <source>This folder couldn&apos;t be loaded.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+182"/>
        <source>Loading explore…</source>
        <translation>正在加载探索…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Explore could not be refreshed right now.</source>
        <translation>探索目前无法刷新。</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>Unknown</source>
        <translation>未知</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>%1 people · %2 places · %3 recent · offline</source>
        <translation>%1 位人物 · %2 个地点 · %3 项最近添加 · 离线</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>%1 people · %2 places · %3 recent</source>
        <translation>%1 位人物 · %2 个地点 · %3 项最近添加</translation>
    </message>
    <message>
        <location line="+74"/>
        <source>Load explore</source>
        <translation>加载探索</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Couldn&apos;t load more: %1</source>
        <translation>无法加载更多：%1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>%1 failed: %2</source>
        <translation>%1 失败：%2</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Couldn&apos;t load photos: %1</source>
        <translation>无法加载照片：%1</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>No people, places, or recent media yet.
Run face and location recognition on your Immich server, or add photos.</source>
        <translation>还没有人物、地点或最近添加的媒体。
请在 Immich 服务器上运行人脸和地点识别，或添加照片。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Connect to your Immich server in Settings → Immich Server
to explore people, places, and recent media.</source>
        <translation>在设置 → Immich 服务器中连接到你的 Immich 服务器
即可探索人物、地点和最近添加的媒体。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Unknown person</source>
        <translation>未知人物</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+24"/>
        <location line="+116"/>
        <source>Loading photos…</source>
        <translation>正在加载照片…</translation>
    </message>
    <message>
        <location line="-102"/>
        <location line="+131"/>
        <source>Albums</source>
        <translation type="unfinished">相册</translation>
    </message>
    <message>
        <location line="-107"/>
        <location line="+152"/>
        <source>Tags</source>
        <translation type="unfinished">标签</translation>
    </message>
    <message>
        <location line="-127"/>
        <location line="+138"/>
        <source>Folders</source>
        <translation type="unfinished">文件夹</translation>
    </message>
    <message>
        <location line="-110"/>
        <source>Favorites</source>
        <translation type="unfinished">收藏</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Archive</source>
        <translation type="unfinished">归档</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Trash</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+48"/>
        <source>No albums yet</source>
        <translation type="unfinished"></translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n album(s)</source>
        <translation type="unfinished">
            <numerusform></numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+3"/>
        <source>%1
%n item(s)</source>
        <translation type="unfinished">
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Unnamed</source>
        <translation type="unfinished">未命名</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 (hidden)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+8"/>
        <source>No people recognized yet</source>
        <translation type="unfinished"></translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n person(s)</source>
        <translation type="unfinished">
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Hide hidden</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show hidden (%1)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Hide people marked hidden in Immich</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show people marked hidden in Immich</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+11"/>
        <source>No tags yet</source>
        <translation type="unfinished"></translation>
    </message>
    <message numerus="yes">
        <location line="+0"/>
        <source>%n tag(s)</source>
        <translation type="unfinished">
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>No folders</source>
        <translation type="unfinished"></translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n folder(s)</source>
        <translation type="unfinished">
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location line="+111"/>
        <location line="+14"/>
        <source>Trash is empty</source>
        <translation type="unfinished"></translation>
    </message>
    <message numerus="yes">
        <location line="-8"/>
        <source> · deleted permanently after %n day(s)</source>
        <translation type="unfinished">
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location line="+92"/>
        <source>Unknown date</source>
        <translation>未知日期</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Today</source>
        <translation>今天</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Yesterday</source>
        <translation>昨天</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Photos</source>
        <translation>照片</translation>
    </message>
    <message>
        <location line="-122"/>
        <location line="+136"/>
        <source>Nothing here yet</source>
        <translation>这里还没有内容</translation>
    </message>
    <message numerus="yes">
        <location line="-133"/>
        <source>%n item(s)</source>
        <translation>
            <numerusform>%n 项</numerusform>
        </translation>
    </message>
    <message>
        <location line="-109"/>
        <location line="+453"/>
        <source>Loading…</source>
        <translation>正在加载…</translation>
    </message>
</context>
<context>
    <name>Aurora::FolderSyncService</name>
    <message>
        <location filename="../src/core/FolderSyncService.cpp" line="+505"/>
        <source>Folder sync is turned off.</source>
        <translation>文件夹同步已关闭。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Choose a folder to watch for photos and videos.</source>
        <translation>选择一个文件夹以监视照片和视频。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Sync folder does not exist: %1</source>
        <translation>同步文件夹不存在：%1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Connect to Immich in Settings → Immich Server first.</source>
        <translation>请先在设置 → Immich 服务器中连接到 Immich。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Waiting for Immich on the local network. Files already show in Library with an unsaved badge.</source>
        <translation>正在等待本地网络上的 Immich。文件已显示在媒体库中，并带有未保存标记。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Saving to Immich…</source>
        <translation>正在保存到 Immich…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Saving %1…</source>
        <translation>正在保存 %1…</translation>
    </message>
    <message numerus="yes">
        <location line="+5"/>
        <source>Waiting for %n file(s) to finish copying…</source>
        <translation>
            <numerusform>正在等待 %n 个文件完成复制…</numerusform>
        </translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Could not sync %1: %2</source>
        <translation>无法同步 %1：%2</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Offline - new files will upload when Immich is reachable.</source>
        <translation>离线：当 Immich 可访问时将上传新文件。</translation>
    </message>
    <message numerus="yes">
        <location line="+9"/>
        <source>Up to date with Immich (%n synced file(s)).</source>
        <translation>
            <numerusform>已与 Immich 同步（%n 个已同步文件）。</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>Aurora::ImmichClient</name>
    <message>
        <location filename="../src/core/ImmichClient.cpp" line="+441"/>
        <source>Connection</source>
        <translation>连接</translation>
    </message>
    <message>
        <location line="+153"/>
        <source>Video stream</source>
        <translation>视频流</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Could not start the local video stream proxy.</source>
        <translation>无法启动本地视频流代理。</translation>
    </message>
    <message>
        <location line="+20"/>
        <location line="+2"/>
        <source>Play video</source>
        <translation>播放视频</translation>
    </message>
    <message>
        <location line="+0"/>
        <location line="+1069"/>
        <location line="+85"/>
        <location line="+24"/>
        <location line="+76"/>
        <location line="+291"/>
        <source>The asset identifier is invalid.</source>
        <translation>媒体项标识符无效。</translation>
    </message>
    <message>
        <location line="-1514"/>
        <source>Set the Immich server URL and API key in Settings first.</source>
        <translation>请先在设置中填写 Immich 服务器地址和 API 密钥。</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>TLS handshake failed — check your client certificate and CA certificate.</source>
        <translation>TLS 握手失败，请检查客户端证书和 CA 证书。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Authentication failed. Check the API key and its permissions.</source>
        <translation>认证失败。请检查 API 密钥及其权限。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Connection test</source>
        <translation>连接测试</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Server URL and API key are required.</source>
        <translation>服务器地址和 API 密钥为必填项。</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>local endpoint</source>
        <translation>本地端点</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>remote endpoint</source>
        <translation>远程端点</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Connected via %1.</source>
        <translation>已通过 %1 连接。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Connected as %1 via %2.</source>
        <translation>以 %1 身份通过 %2 连接。</translation>
    </message>
    <message>
        <location line="+15"/>
        <location line="+3"/>
        <location line="+16"/>
        <location line="+23"/>
        <location line="+48"/>
        <location line="+18"/>
        <location line="+47"/>
        <source>Load library</source>
        <translation>加载媒体库</translation>
    </message>
    <message>
        <location line="-151"/>
        <source>You&apos;re offline - Years/Month browsing needs a connection.</source>
        <translation>当前离线：按年/月浏览需要网络连接。</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>You&apos;re offline.</source>
        <translation>当前离线。</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Check for new photos</source>
        <translation>检查新照片</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+17"/>
        <location line="+47"/>
        <source>Search</source>
        <translation>搜索</translation>
    </message>
    <message>
        <location line="-46"/>
        <source>You are offline and no cached library is available.</source>
        <translation>你处于离线状态，且没有可用的缓存媒体库。</translation>
    </message>
    <message>
        <location line="+89"/>
        <location line="+8"/>
        <location line="+115"/>
        <location line="+10"/>
        <location line="+173"/>
        <location line="+38"/>
        <source>Load explore</source>
        <translation>加载探索</translation>
    </message>
    <message>
        <location line="-335"/>
        <source>You are offline and no cached explore data is available.</source>
        <translation>你处于离线状态，且没有可用的缓存探索数据。</translation>
    </message>
    <message>
        <location line="+348"/>
        <location line="+90"/>
        <location line="+787"/>
        <location line="+52"/>
        <source>Load image</source>
        <translation>加载图片</translation>
    </message>
    <message>
        <location line="-925"/>
        <source>The person identifier is invalid.</source>
        <translation>人物标识符无效。</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Offline - person photo not cached.</source>
        <translation>离线：人物照片未缓存。</translation>
    </message>
    <message>
        <location line="+24"/>
        <location line="+907"/>
        <source>Unsupported image format.</source>
        <translation>不支持的图片格式。</translation>
    </message>
    <message>
        <location line="-806"/>
        <location line="+99"/>
        <location line="+9"/>
        <location line="+16"/>
        <location line="+6"/>
        <location line="+76"/>
        <location line="+11"/>
        <source>Upload</source>
        <translation>上传</translation>
    </message>
    <message>
        <location line="-120"/>
        <source>File not found: %1</source>
        <translation>找不到文件：%1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>File is empty: %1</source>
        <translation>文件为空：%1</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Could not open %1 after several tries: %2</source>
        <translation>多次尝试后仍无法打开 %1：%2</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Could not open %1: %2 (will retry)</source>
        <translation>无法打开 %1：%2（将重试）</translation>
    </message>
    <message>
        <location line="+76"/>
        <source>%1 interrupted - queued for retry when online.</source>
        <translation>%1 已中断，已排队，待联网后重试。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>%1: %2</source>
        <translation>%1：%2</translation>
    </message>
    <message>
        <location line="+36"/>
        <location line="+3"/>
        <location line="+8"/>
        <location line="+38"/>
        <location line="+11"/>
        <location line="+11"/>
        <source>Download</source>
        <translation>下载</translation>
    </message>
    <message>
        <location line="-59"/>
        <location line="+38"/>
        <location line="+22"/>
        <source>Could not write %1: %2</source>
        <translation>无法写入 %1：%2</translation>
    </message>
    <message>
        <location line="+113"/>
        <location line="+3"/>
        <location line="+39"/>
        <location line="+10"/>
        <location line="+8"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location line="-17"/>
        <location line="+10"/>
        <source>This original is larger than the 64 MB clipboard safety limit. Download it to disk instead.</source>
        <translation>此原图超过 64 MB 剪贴板安全限制。请改为下载到磁盘。</translation>
    </message>
    <message>
        <location line="+47"/>
        <location line="+9"/>
        <source>Trash</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+27"/>
        <location line="+12"/>
        <location line="+21"/>
        <source>Delete</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="-21"/>
        <source>No valid asset identifiers were provided.</source>
        <translation>未提供有效的媒体项标识符。</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Favorite</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Favorites need the asset.update permission on your API key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Connect to Immich and choose an uploaded photo or video.</source>
        <translation>连接到 Immich 并选择一张已上传的照片或视频。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Set a valid Immich URL in server settings first.</source>
        <translation>请先在服务器设置中填写有效的 Immich 地址。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>You are offline. Connect to Immich to create a share link.</source>
        <translation>你处于离线状态。请连接到 Immich 以创建分享链接。</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Immich returned an invalid share link.</source>
        <translation>Immich 返回了无效的分享链接。</translation>
    </message>
</context>
<context>
    <name>Aurora::LibraryPage</name>
    <message>
        <location filename="../src/ui/pages/LibraryPage.cpp" line="+114"/>
        <location line="+1637"/>
        <source>Upload</source>
        <translation>上传</translation>
    </message>
    <message>
        <location line="-1636"/>
        <source>Refresh</source>
        <translation>刷新</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+150"/>
        <location line="+29"/>
        <source>Years</source>
        <translation>年份</translation>
    </message>
    <message>
        <location line="-141"/>
        <source>Photos &amp; videos</source>
        <translation>照片和视频</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Your Immich library</source>
        <translation>你的 Immich 媒体库</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Search photos and videos…</source>
        <translation>搜索照片和视频…</translation>
    </message>
    <message>
        <location line="+14"/>
        <location line="+1"/>
        <source>Search settings</source>
        <translation>搜索设置</translation>
    </message>
    <message>
        <location line="+67"/>
        <source>Drop photos or videos to upload</source>
        <translation>拖放照片或视频以上传</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Library</source>
        <translation>媒体库</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>Refine your search</source>
        <translation>细化搜索</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Reset filters</source>
        <translation>重置筛选条件</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>All assets</source>
        <translation>全部媒体项</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Favorites</source>
        <translation>收藏</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Archive</source>
        <translation>归档</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Search in</source>
        <translation>搜索范围</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+161"/>
        <source>All people</source>
        <translation>全部人物</translation>
    </message>
    <message>
        <location line="-158"/>
        <source>People</source>
        <translation>人物</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>City</source>
        <translation>城市</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Places</source>
        <translation>地点</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Photos and videos</source>
        <translation>照片和视频</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Photos</source>
        <translation>照片</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Videos</source>
        <translation>视频</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Media</source>
        <translation>媒体</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Date range</source>
        <translation>日期范围</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>From date</source>
        <translation>开始日期</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>To date</source>
        <translation>结束日期</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>From</source>
        <translation>从</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>To</source>
        <translation>到</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Search by</source>
        <translation>搜索方式</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Context</source>
        <translation>上下文</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>File name</source>
        <translation>文件名</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Description</source>
        <translation>描述</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Full path or folder</source>
        <translation>完整路径或文件夹</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>OCR</source>
        <translation>OCR</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Search interpretation</source>
        <translation>搜索解析</translation>
    </message>
    <message>
        <location line="+111"/>
        <source>Unnamed person</source>
        <translation>未命名人物</translation>
    </message>
    <message>
        <location line="+191"/>
        <location line="+1316"/>
        <source>Loading library…</source>
        <translation>正在加载媒体库…</translation>
    </message>
    <message numerus="yes">
        <location line="-1234"/>
        <location line="+42"/>
        <location line="+283"/>
        <location line="+163"/>
        <location line="+779"/>
        <source>%n item(s)</source>
        <translation>
            <numerusform>%n 项</numerusform>
        </translation>
    </message>
    <message>
        <location line="-1264"/>
        <source>Loading library… %1/%2 months</source>
        <translation>正在加载媒体库… %1/%2 个月</translation>
    </message>
    <message>
        <location line="+57"/>
        <source>The start date must be on or before the end date.</source>
        <translation>开始日期必须早于或等于结束日期。</translation>
    </message>
    <message>
        <location line="+131"/>
        <source>Find scenes, objects, and ideas in the visual content of your photos.</source>
        <translation>在照片的视觉内容中查找场景、物体和概念。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Match the original name of a photo or video file.</source>
        <translation>匹配照片或视频文件的原始名称。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Search the descriptions saved with your photos and videos.</source>
        <translation>搜索照片和视频保存的描述。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Find media by its original full path or folder.</source>
        <translation>按媒体项的原始完整路径或文件夹查找。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Find words detected inside images, such as signs, receipts, and screenshots.</source>
        <translation>查找图像中识别出的文字，例如标识、收据和截图。</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>No filters</source>
        <translation>无筛选条件</translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n active filter(s)</source>
        <translation>
            <numerusform>%n 个生效的筛选条件</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+17"/>
        <source>%n new photo(s) found, refreshing…</source>
        <translation>
            <numerusform>%n 张新照片，正在刷新…</numerusform>
        </translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Loading more results…</source>
        <translation>正在加载更多结果…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Searching with filters…</source>
        <translation>正在使用筛选条件搜索…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Searching for “%1”…</source>
        <translation>正在搜索“%1”…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Loading more media…</source>
        <translation>正在加载更多媒体…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Loading your library…</source>
        <translation>正在加载你的媒体库…</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Unknown date</source>
        <translation>未知日期</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Today</source>
        <translation>今天</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Yesterday</source>
        <translation>昨天</translation>
    </message>
    <message numerus="yes">
        <location line="+24"/>
        <source>%n filtered result(s)</source>
        <translation>
            <numerusform>%n 个筛选结果</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n result(s) for “%1”</source>
        <translation>
            <numerusform>%n 条“%1”搜索结果</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+2"/>
        <source>%n cached item(s) · offline</source>
        <translation>
            <numerusform>%n 个缓存项 · 离线</numerusform>
        </translation>
    </message>
    <message>
        <location line="+229"/>
        <source>Couldn&apos;t load library: %1. Refresh to retry.</source>
        <translation>无法加载媒体库：%1。请刷新以重试。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Loading library… %1/%2 months (some months failed)</source>
        <translation>正在加载媒体库… %1/%2 个月（部分月份加载失败）</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>You&apos;re offline - Years needs a connection to browse by month.</source>
        <translation>当前离线——“年份”视图需要联网才能按月份浏览。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Loading…</source>
        <translation>正在加载…</translation>
    </message>
    <message>
        <location line="+104"/>
        <source>%1 - you&apos;re offline, can&apos;t load months.</source>
        <translation>%1：当前离线，无法加载月份。</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 - loading…</source>
        <translation>%1：正在加载…</translation>
    </message>
    <message>
        <location line="+246"/>
        <source>Load image</source>
        <translation>加载图片</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Copy failed: %1</source>
        <translation>复制失败：%1</translation>
    </message>
    <message>
        <location line="+15"/>
        <location line="+755"/>
        <source>Upload finished: %1 ok, %2 failed</source>
        <translation>上传完成：%1 成功，%2 失败</translation>
    </message>
    <message>
        <location line="-747"/>
        <source>%1 (%2 left)</source>
        <translation>%1（剩余 %2）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Upload failed: %1 (%2 left)</source>
        <translation>上传失败：%1（剩余 %2）</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>%1 failed: %2</source>
        <translation>%1 失败：%2</translation>
    </message>
    <message>
        <location line="+234"/>
        <source>Connect to your Immich server in Settings → Immich Server
to browse photos and videos.</source>
        <translation>在设置 → Immich 服务器中连接到你的 Immich 服务器
即可浏览照片和视频。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>No results for these filters.
Try different filters.</source>
        <translation>这些筛选条件没有结果。
请尝试其他筛选条件。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>No results for “%1”.
Try a different search or filters.</source>
        <translation>“%1”没有结果。
请尝试其他搜索词或筛选条件。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>No photos or videos yet.
Drop files here, paste with Ctrl+V, or use Upload to add media.</source>
        <translation>还没有照片或视频。
将文件拖放到此处、按 Ctrl+V 粘贴，或使用“上传”添加媒体。</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>%1 · local</source>
        <translation>%1 · 本地</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 · remote</source>
        <translation>%1 · 远程</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Back online , resuming uploads…</source>
        <translation>已恢复联网，正在继续上传…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Back online , refreshing…</source>
        <translation>已恢复联网，正在刷新…</translation>
    </message>
    <message numerus="yes">
        <location line="+5"/>
        <source>%n item(s) · offline</source>
        <translation>
            <numerusform>%n 项 · 离线</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+8"/>
        <source>%n file(s) waiting to upload</source>
        <translation>
            <numerusform>%n 个文件等待上传</numerusform>
        </translation>
    </message>
    <message>
        <location line="+73"/>
        <source>Preparing download…</source>
        <translation>正在准备下载…</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>Download media</source>
        <translation>下载媒体</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Downloading %1…</source>
        <translation>正在下载 %1…</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Connect to Immich in Settings before copying.</source>
        <translation>复制前请先在设置中连接到 Immich。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Copy works for photos , download videos instead.</source>
        <translation>复制适用于照片，视频请改为下载。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Copying %1…</source>
        <translation>正在复制 %1…</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+116"/>
        <source>photo</source>
        <translation>照片</translation>
    </message>
    <message>
        <location line="-107"/>
        <location line="+207"/>
        <source>Connect to Immich in Settings before uploading.</source>
        <translation>上传前请先在设置中连接到 Immich。</translation>
    </message>
    <message>
        <location line="-198"/>
        <source>Clipboard has no image or media files to upload.</source>
        <translation>剪贴板中没有可上传的图片或媒体文件。</translation>
    </message>
    <message>
        <location line="+54"/>
        <source>Could not save clipboard image for upload.</source>
        <translation>无法保存剪贴板图片以供上传。</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Could not decode image for clipboard.</source>
        <translation>无法解码剪贴板中的图片。</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Copied %1 to clipboard</source>
        <translation>已将 %1 复制到剪贴板</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>this item</source>
        <translation>此媒体项</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+10"/>
        <source>Delete permanently</source>
        <translation>永久删除</translation>
    </message>
    <message>
        <location line="-10"/>
        <location line="+10"/>
        <source>Move to trash</source>
        <translation>移至回收站</translation>
    </message>
    <message>
        <location line="-8"/>
        <source>Permanently delete %1? This cannot be undone.</source>
        <translation>永久删除 %1？此操作无法撤销。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Move %1 to trash? You can restore it later from Immich.</source>
        <translation>将 %1 移至回收站？你稍后可以从 Immich 恢复。</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Deleting…</source>
        <translation>正在删除…</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Moving to trash…</source>
        <translation>正在移至回收站…</translation>
    </message>
    <message numerus="yes">
        <location line="+7"/>
        <source>Deleted %n item(s).</source>
        <translation>
            <numerusform>已删除 %n 项。</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>Moved %n item(s) to trash.</source>
        <translation>
            <numerusform>已将 %n 项移至回收站。</numerusform>
        </translation>
    </message>
    <message>
        <location line="+59"/>
        <source>Upload media</source>
        <translation>上传媒体</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>No supported media files to upload.</source>
        <translation>没有可上传的受支持媒体文件。</translation>
    </message>
    <message numerus="yes">
        <location line="+7"/>
        <source>Queued %n file(s) for upload when online.</source>
        <translation>
            <numerusform>已排队 %n 个文件，待联网后上传。</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+2"/>
        <source>Uploading %n file(s)…</source>
        <translation>
            <numerusform>正在上传 %n 个文件…</numerusform>
        </translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Uploading %1… %2% (%3 left)</source>
        <translation>正在上传 %1… %2%（剩余 %3）</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Uploading %1…</source>
        <translation>正在上传 %1…</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Already on server: %1</source>
        <translation>已在服务器上：%1</translation>
    </message>
    <message numerus="yes">
        <location line="+2"/>
        <source>Uploaded %n file(s).</source>
        <translation>
            <numerusform>已上传 %n 个文件。</numerusform>
        </translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Duplicate skipped: %1 (%2 left)</source>
        <translation>已跳过重复项：%1（剩余 %2）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Uploaded %1 (%2 left)</source>
        <translation>已上传 %1（剩余 %2）</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Downloading… %1%</source>
        <translation>正在下载… %1%</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Saved to %1</source>
        <translation>已保存到 %1</translation>
    </message>
</context>
<context>
    <name>Aurora::MainWindow</name>
    <message>
        <location filename="../src/ui/MainWindow.cpp" line="+357"/>
        <location line="+224"/>
        <source>immich</source>
        <translation>immich</translation>
    </message>
    <message>
        <location line="-221"/>
        <source>Show immich</source>
        <translation>显示 immich</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Quit</source>
        <translation>退出</translation>
    </message>
    <message>
        <location line="+88"/>
        <source>Library</source>
        <translation>媒体库</translation>
    </message>
    <message>
        <location line="-303"/>
        <location line="+306"/>
        <source>Explore</source>
        <translation>探索</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Settings / Immich Server</source>
        <translation>设置 / Immich 服务器</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Settings / Folder Sync</source>
        <translation>设置 / 文件夹同步</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Settings / Appearance</source>
        <translation>设置 / 外观</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Settings / Update</source>
        <translation>设置 / 更新</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Settings / About</source>
        <translation>设置 / 关于</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Support immich desktop</source>
        <translation>支持 immich desktop</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>If immich desktop is useful, a GitHub star helps a lot.</source>
        <translation>如果 immich desktop 对你有帮助，在 GitHub 上点个星会很有帮助。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Stars make the project easier to discover. Thanks for considering it!</source>
        <translation>星标能让项目更容易被发现。感谢你的考虑！</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Star on GitHub</source>
        <translation>在 GitHub 上点星</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Maybe later</source>
        <translation>稍后再说</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Certificate password</source>
        <translation>证书密码</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enter the password for your client certificate:</source>
        <translation>输入客户端证书的密码：</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>immich - uploading</source>
        <translation>immich：正在上传</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>immich - downloading</source>
        <translation>immich：正在下载</translation>
    </message>
    <message>
        <location line="+16"/>
        <location line="+9"/>
        <source>Update available</source>
        <translation>有可用更新</translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+9"/>
        <source>immich desktop v%1 is available.</source>
        <translation>immich desktop v%1 已发布。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>What&apos;s new:</source>
        <translation>更新内容：</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Open the Updates page to download and install the package for this platform.</source>
        <translation>打开“更新”页面，下载并安装适用于此平台的安装包。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Open Updates</source>
        <translation>打开更新</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Later</source>
        <translation>稍后</translation>
    </message>
</context>
<context>
    <name>Aurora::MediaTile</name>
    <message>
        <location filename="../src/ui/widgets/MediaTile.cpp" line="+165"/>
        <source>…</source>
        <translation>…</translation>
    </message>
    <message>
        <location line="+155"/>
        <source>Open</source>
        <translation>打开</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+7"/>
        <source>Info</source>
        <translation>信息</translation>
    </message>
    <message>
        <location line="-4"/>
        <source>Restore</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+9"/>
        <source>Download</source>
        <translation>下载</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Share link…</source>
        <translation>分享链接…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove from favorites</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Add to favorites</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Move to trash</source>
        <translation>移至回收站</translation>
    </message>
    <message>
        <location line="-11"/>
        <location line="+12"/>
        <source>Delete permanently</source>
        <translation>永久删除</translation>
    </message>
</context>
<context>
    <name>Aurora::MediaViewerDialog</name>
    <message>
        <location filename="../src/ui/widgets/MediaViewerDialog.cpp" line="+48"/>
        <source>Favorite</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Download</source>
        <translation>下载</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Share link</source>
        <translation>分享链接</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Move to trash</source>
        <translation>移至回收站</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Restore</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Delete permanently</source>
        <translation>永久删除</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Info</source>
        <translation>信息</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Show info (I)</source>
        <translation>显示信息（I）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Close</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Previous (Left arrow)</source>
        <translation>上一个（左方向键）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Next (Right arrow)</source>
        <translation>下一个（右方向键）</translation>
    </message>
    <message>
        <location line="+104"/>
        <source>Preview unavailable
%1</source>
        <translation>预览不可用
%1</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Media preview</source>
        <translation>媒体预览</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Loading preview…</source>
        <translation>正在加载预览…</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Remove from favorites (F)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add to favorites (F)</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Aurora::SettingsPage</name>
    <message>
        <location filename="../src/ui/pages/SettingsPage.cpp" line="+33"/>
        <source>Back</source>
        <translation type="unfinished">返回</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Back (Esc)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Settings</source>
        <translation>设置</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Choose a settings section from the sidebar.</source>
        <translation>从侧边栏选择一个设置分类。</translation>
    </message>
</context>
<context>
    <name>Aurora::ShareLinkDialog</name>
    <message>
        <location filename="../src/ui/widgets/ShareLinkDialog.cpp" line="+19"/>
        <source>Create link</source>
        <translation>创建链接</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy link</source>
        <translation>复制链接</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Share media</source>
        <translation>分享媒体</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Share with a link</source>
        <translation>通过链接分享</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Photo or video</source>
        <translation>照片或视频</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>The link uses your Immich URL from server settings, even when the app is connected locally.</source>
        <translation>即使应用在本地连接，链接仍使用服务器设置中的 Immich 地址。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Password (optional)</source>
        <translation>密码（可选）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Share link password</source>
        <translation>分享链接密码</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Leave empty to share without a password</source>
        <translation>留空则无需密码即可分享</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Show password</source>
        <translation>显示密码</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Share link</source>
        <translation>分享链接</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Create a link to share this media…</source>
        <translation>创建链接以分享此媒体…</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Creating your share link…</source>
        <translation>正在创建分享链接…</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Link ready. A password is required to view this media.</source>
        <translation>链接已就绪。查看此媒体需要密码。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Link ready. Anyone with the link can view this media.</source>
        <translation>链接已就绪。任何持有链接的人都可以查看此媒体。</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Link copied. Share the password separately.</source>
        <translation>链接已复制。请单独分享密码。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Link copied to clipboard.</source>
        <translation>链接已复制到剪贴板。</translation>
    </message>
</context>
<context>
    <name>Aurora::Sidebar</name>
    <message>
        <location filename="../src/ui/widgets/Sidebar.cpp" line="+37"/>
        <location line="+133"/>
        <source>Collapse</source>
        <translation>折叠</translation>
    </message>
    <message>
        <location line="-77"/>
        <source>fan-made
immich desktop  ·  %1</source>
        <translation>爱好者制作
immich desktop  ·  %1</translation>
    </message>
    <message>
        <location line="+77"/>
        <source>Expand</source>
        <translation>展开</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Expand sidebar (F9)</source>
        <translation>展开侧边栏（F9）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Collapse sidebar (F9)</source>
        <translation>折叠侧边栏（F9）</translation>
    </message>
    <message>
        <location line="+138"/>
        <source>Main</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Library</source>
        <translation>媒体库</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Explore</source>
        <translation>探索</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>People</source>
        <translation type="unfinished">人物</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Favorites</source>
        <translation type="unfinished">收藏</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Collections</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Archive</source>
        <translation type="unfinished">归档</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Albums</source>
        <translation type="unfinished">相册</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tags</source>
        <translation type="unfinished">标签</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Folders</source>
        <translation type="unfinished">文件夹</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tools</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Trash</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Settings</source>
        <translation>设置</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>SETTINGS</source>
        <translation>设置</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Immich Server</source>
        <translation>Immich 服务器</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Folder Sync</source>
        <translation>文件夹同步</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Update</source>
        <translation>更新</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>About</source>
        <translation>关于</translation>
    </message>
</context>
<context>
    <name>Aurora::SummaryTile</name>
    <message numerus="yes">
        <location filename="../src/ui/widgets/SummaryTile.cpp" line="+89"/>
        <source>%n item(s)</source>
        <translation>
            <numerusform>%n 项</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>Aurora::SyncFolderPage</name>
    <message>
        <location filename="../src/ui/pages/SyncFolderPage.cpp" line="+27"/>
        <source>Watch folders and upload new media</source>
        <translation>监视文件夹并上传新增的媒体文件</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Upload only when Immich is available on the local network</source>
        <translation>仅当 Immich 在本地网络可用时上传</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Scan now</source>
        <translation>立即扫描</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+1"/>
        <source>Add another folder</source>
        <translation>添加另一个文件夹</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Folder sync</source>
        <translation>文件夹同步</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Choose folders on this computer. Photos and videos dropped into them (including subfolders) are uploaded to Immich automatically on Windows, macOS, and Linux. Files stay on disk after upload.</source>
        <translation>选择此计算机上的文件夹。放入其中（包括子文件夹）的照片和视频会在 Windows、macOS 和 Linux 上自动上传到 Immich。上传后文件仍保留在磁盘上。</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Uses your Local URL from Immich Server settings. Files still appear in Library with an unsaved badge until the local server is reachable.</source>
        <translation>使用 Immich 服务器设置中的本地地址。在本地服务器可达之前，文件仍会以未保存标记显示在媒体库中。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Folders</source>
        <translation>文件夹</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Status icons: saved · saving · unsaved · error saving</source>
        <translation>状态图标：已保存 · 保存中 · 未保存 · 保存出错</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Saved</source>
        <translation>已保存</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Saving</source>
        <translation>保存中</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Unsaved</source>
        <translation>未保存</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Choose sync folder</source>
        <translation>选择同步文件夹</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Add sync folder</source>
        <translation>添加同步文件夹</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Sync folder path</source>
        <translation>同步文件夹路径</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Choose a folder to sync…</source>
        <translation>选择要同步的文件夹…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Browse…</source>
        <translation>浏览…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open folder</source>
        <translation>打开文件夹</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+1"/>
        <source>Remove folder from sync</source>
        <translation>从同步中移除文件夹</translation>
    </message>
</context>
<context>
    <name>Aurora::Toast</name>
    <message>
        <location filename="../src/ui/widgets/Toast.cpp" line="+34"/>
        <source>Click to dismiss</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Aurora::TopBar</name>
    <message>
        <location filename="../src/ui/widgets/TopBar.cpp" line="+25"/>
        <source>Overview</source>
        <translation>概览</translation>
    </message>
    <message>
        <location line="+27"/>
        <location line="+1"/>
        <source>Switch profile</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+53"/>
        <source>immich — %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+16"/>
        <source>  Update available</source>
        <translation>  有可用更新</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>  Update to v%1</source>
        <translation>  更新到 v%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open the Updates page</source>
        <translation>打开“更新”页面</translation>
    </message>
</context>
<context>
    <name>Aurora::UpdateManager</name>
    <message>
        <location filename="../src/core/UpdateManager.cpp" line="+167"/>
        <source>No downloadable update package was found for this platform.</source>
        <translation>未找到适用于此平台的可下载更新包。</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>The update package has an unsafe file name.</source>
        <translation>更新包的文件名不安全。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Unable to create the update download directory.</source>
        <translation>无法创建更新下载目录。</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Unable to create the update package file.</source>
        <translation>无法创建更新包文件。</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Unable to refresh the Snap package. Try: snap refresh %1</source>
        <translation>无法刷新 Snap 包。请尝试：snap refresh %1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The update package has not been downloaded yet.</source>
        <translation>更新包尚未下载。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Unable to launch the update installer.</source>
        <translation>无法启动更新安装程序。</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Update check failed: %1</source>
        <translation>更新检查失败：%1</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>The update package exceeded its advertised size.</source>
        <translation>更新包超出其声明的大小。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Unable to write the downloaded package.</source>
        <translation>无法写入下载的安装包。</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Download failed: %1</source>
        <translation>下载失败：%1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>The update package file is unavailable.</source>
        <translation>更新包文件不可用。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>The update package size did not match the release metadata.</source>
        <translation>更新包大小与发行版元数据不一致。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Unable to finalize the downloaded package.</source>
        <translation>无法完成已下载安装包的收尾处理。</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>The update server returned an unexpected response.</source>
        <translation>更新服务器返回了意外的响应。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The latest release does not include a version tag.</source>
        <translation>最新发行版不包含版本标签。</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>No compatible installer was published for this platform.</source>
        <translation>未发布适用于此平台的兼容安装程序。</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>The selected update package has an unsafe file name.</source>
        <translation>所选更新包的文件名不安全。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The selected update package has an untrusted download URL.</source>
        <translation>所选更新包的下载 URL 不受信任。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>The selected update package has an invalid size.</source>
        <translation>所选更新包的大小无效。</translation>
    </message>
</context>
<context>
    <name>Aurora::UpdatesPage</name>
    <message>
        <location filename="../src/ui/pages/UpdatesPage.cpp" line="+30"/>
        <source>Check for updates automatically</source>
        <translation>自动检查更新</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refresh</source>
        <translation>刷新</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Update</source>
        <translation>更新</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Updates</source>
        <translation>更新</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Current version: %1</source>
        <translation>当前版本：%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+3"/>
        <source>Current version: v%1 (Snap)</source>
        <translation>当前版本：v%1（Snap）</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Enjoying immich desktop?</source>
        <translation>喜欢 immich desktop 吗？</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>A GitHub star helps others find this unofficial client and supports continued work.</source>
        <translation>在 GitHub 上点个星，能帮助其他人发现这个非官方客户端，并支持后续开发。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Star on GitHub</source>
        <translation>在 GitHub 上点星</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Maybe later</source>
        <translation>稍后再说</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>%p%  ·  %1 / %2</source>
        <translation>%p%  ·  %1 / %2</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>Updates for this installation are provided by your package manager.</source>
        <translation>此安装的更新由你的包管理器提供。</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Press Refresh to check the Snap Store for updates.</source>
        <translation>按“刷新”以检查 Snap Store 中的更新。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Press Refresh to check for updates.</source>
        <translation>按“刷新”以检查更新。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Checking the Snap Store for updates…</source>
        <translation>正在检查 Snap Store 中的更新…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Checking for updates…</source>
        <translation>正在检查更新…</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>You&apos;re up to date with the Snap Store.</source>
        <translation>你已与 Snap Store 保持同步。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>You&apos;re up to date.</source>
        <translation>你已是最新版本。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Snap Store update available: v%1</source>
        <translation>Snap Store 有可用更新：v%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Update available: v%1</source>
        <translation>有可用更新：v%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Downloading v%1…</source>
        <translation>正在下载 v%1…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Installing v%1…</source>
        <translation>正在安装 v%1…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Download complete</source>
        <translation>下载完成</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Installing update…</source>
        <translation>正在安装更新…</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Update failed: %1</source>
        <translation>更新失败：%1</translation>
    </message>
</context>
<context>
    <name>Aurora::VideoPlayerWidget</name>
    <message>
        <location filename="../src/ui/widgets/VideoPlayerWidget.cpp" line="+49"/>
        <location line="+170"/>
        <source>Play (Space)</source>
        <translation>播放（空格键）</translation>
    </message>
    <message>
        <location line="-169"/>
        <location line="+170"/>
        <source>Play</source>
        <translation>播放</translation>
    </message>
    <message>
        <location line="-159"/>
        <location line="+12"/>
        <source>Volume</source>
        <translation>音量</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>Buffering…</source>
        <translation>正在缓冲…</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Playback finished</source>
        <translation>播放完成</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Video playback failed.</source>
        <translation>视频播放失败。</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Could not start the video stream.</source>
        <translation>无法启动视频流。</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>Pause (Space)</source>
        <translation>暂停（空格键）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pause</source>
        <translation>暂停</translation>
    </message>
</context>
<context>
    <name>Aurora::ZoomSlider</name>
    <message>
        <location filename="../src/ui/widgets/ZoomSlider.cpp" line="+28"/>
        <source>Smaller thumbnails (Ctrl+−)</source>
        <translation>缩小缩略图（Ctrl+−）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Larger thumbnails (Ctrl++)</source>
        <translation>放大缩略图（Ctrl++）</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Thumbnail size (Ctrl+scroll)</source>
        <translation>缩略图大小（Ctrl+滚轮）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Thumbnail size</source>
        <translation>缩略图大小</translation>
    </message>
</context>
<context>
    <name>AutoStart</name>
    <message>
        <location filename="../src/core/AutoStart.cpp" line="+94"/>
        <source>Autostart is not supported on this platform.</source>
        <translation>此平台不支持自动启动。</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Could not update Windows startup settings.</source>
        <translation>无法更新 Windows 启动设置。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Could not remove autostart entry.</source>
        <translation>无法移除自动启动项。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Could not create autostart entry.</source>
        <translation>无法创建自动启动项。</translation>
    </message>
</context>
<context>
    <name>Profiles</name>
    <message>
        <location filename="../src/core/AppSettings.cpp" line="+48"/>
        <source>Personal</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../src/core/CustomHeaders.cpp" line="+45"/>
        <source>Row %1: header name is required.</source>
        <translation>第 %1 行：标头名称为必填项。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Row %1: &quot;%2&quot; is not a valid header name.</source>
        <translation>第 %1 行：“%2”不是有效的标头名称。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Row %1: &quot;%2&quot; is managed by the app and cannot be overridden.</source>
        <translation>第 %1 行：“%2”由应用管理，无法覆盖。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Row %1: header values cannot contain line breaks.</source>
        <translation>第 %1 行：标头值不能包含换行符。</translation>
    </message>
    <message>
        <location filename="../src/core/ImmichClient.cpp" line="+197"/>
        <source>The server returned an image larger than 32 MB.</source>
        <translation>服务器返回了大于 32 MB 的图片。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Authentication failed. Check the API key and its permissions.</source>
        <translation>认证失败。请检查 API 密钥及其权限。</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Unsupported image format.</source>
        <translation>不支持的图片格式。</translation>
    </message>
    <message>
        <location filename="../src/ui/pages/AboutPage.cpp" line="-36"/>
        <source>Copied!</source>
        <translation>已复制！</translation>
    </message>
    <message>
        <location filename="../src/ui/pages/LibraryPage.cpp" line="-2462"/>
        <source>Media files (*.jpg *.jpeg *.png *.gif *.webp *.heic *.heif *.tif *.tiff *.bmp *.raw *.dng *.cr2 *.nef *.arw *.mp4 *.mov *.m4v *.avi *.mkv *.webm *.3gp);;All files (*.*)</source>
        <translation>媒体文件 (*.jpg *.jpeg *.png *.gif *.webp *.heic *.heif *.tif *.tiff *.bmp *.raw *.dng *.cr2 *.nef *.arw *.mp4 *.mov *.m4v *.avi *.mkv *.webm *.3gp);;所有文件 (*.*)</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="+70"/>
        <source>Updating translations…</source>
        <translation>正在更新翻译…</translation>
    </message>
</context>
<context>
    <name>TlsConfig</name>
    <message>
        <location filename="../src/core/TlsConfig.cpp" line="+25"/>
        <location line="+18"/>
        <source>Could not open %1: %2</source>
        <translation>无法打开 %1：%2</translation>
    </message>
    <message>
        <location line="-6"/>
        <source>A password is required to open the PKCS#12 bundle.</source>
        <translation>打开 PKCS#12 证书包需要密码。</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Could not read the PKCS#12 bundle. The password may be wrong, or the file is not a valid bundle.</source>
        <translation>无法读取 PKCS#12 证书包。密码可能不正确，或文件不是有效的证书包。</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Could not read a client certificate from %1.</source>
        <translation>无法从 %1 读取客户端证书。</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Could not read the private key from %1. The key may be encrypted and need a password, or the password is wrong.</source>
        <translation>无法从 %1 读取私钥。密钥可能已加密且需要密码，或密码不正确。</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Could not read a CA certificate from %1 (tried PEM and DER).</source>
        <translation>无法从 %1 读取 CA 证书（已尝试 PEM 和 DER 格式）。</translation>
    </message>
</context>
</TS>
