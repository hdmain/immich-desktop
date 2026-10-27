<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en" sourcelanguage="en">
<context>
    <name>Aurora::AboutPage</name>
    <message>
        <location filename="../src/ui/pages/AboutPage.cpp" line="+73"/>
        <source>About</source>
        <translation>About</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Unofficial, fan-made desktop client for your self-hosted Immich library.</source>
        <translation>Unofficial, fan-made desktop client for your self-hosted Immich library.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>What is this?</source>
        <translation>What is this?</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>immich desktop is an unofficial, fan-made desktop app for &lt;a href=&quot;https://immich.app&quot;&gt;Immich&lt;/a&gt; , the self-hosted photo and video manager. It connects to your own Immich server via an API key and lets you browse your timeline, search, explore people &amp;amp; places, upload/download, stream video, and keep browsing offline through a local thumbnail cache and queued uploads. It is not affiliated with, maintained by, or endorsed by the official Immich project.</source>
        <translation>immich desktop is an unofficial, fan-made desktop app for &lt;a href=&quot;https://immich.app&quot;&gt;Immich&lt;/a&gt; , the self-hosted photo and video manager. It connects to your own Immich server via an API key and lets you browse your timeline, search, explore people &amp;amp; places, upload/download, stream video, and keep browsing offline through a local thumbnail cache and queued uploads. It is not affiliated with, maintained by, or endorsed by the official Immich project.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Who makes it?</source>
        <translation>Who makes it?</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Built and maintained by the Immich Desktop community, led by &lt;a href=&quot;https://github.com/hdmain&quot;&gt;hdmain&lt;/a&gt;. Major contributor: &lt;a href=&quot;https://github.com/gardar&quot;&gt;gardar&lt;/a&gt; (viewer navigation, zoomable grids, collapsible sidebar, info panel, and more). Contributions, bug reports, and ideas are welcome on GitHub.</source>
        <translation>Built and maintained by the Immich Desktop community, led by &lt;a href=&quot;https://github.com/hdmain&quot;&gt;hdmain&lt;/a&gt;. Major contributor: &lt;a href=&quot;https://github.com/gardar&quot;&gt;gardar&lt;/a&gt; (viewer navigation, zoomable grids, collapsible sidebar, info panel, and more). Contributions, bug reports, and ideas are welcome on GitHub.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>&lt;a href=&quot;https://github.com/%1&quot;&gt;github.com/%1&lt;/a&gt; · &lt;a href=&quot;https://github.com/%1/issues&quot;&gt;Issues&lt;/a&gt; · &lt;a href=&quot;https://github.com/%1/releases&quot;&gt;Releases&lt;/a&gt;</source>
        <translation>&lt;a href=&quot;https://github.com/%1&quot;&gt;github.com/%1&lt;/a&gt; · &lt;a href=&quot;https://github.com/%1/issues&quot;&gt;Issues&lt;/a&gt; · &lt;a href=&quot;https://github.com/%1/releases&quot;&gt;Releases&lt;/a&gt;</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Tech stack</source>
        <translation>Tech stack</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Everything this build is made with , frameworks, packaging, and key libraries:</source>
        <translation>Everything this build is made with , frameworks, packaging, and key libraries:</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Qt 6 - Core, Gui, Widgets, Network, Svg</source>
        <translation>Qt 6 - Core, Gui, Widgets, Network, Svg</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Qt Multimedia (+ MultimediaWidgets) - video playback &amp; audio (QMediaPlayer, QVideoSink)</source>
        <translation>Qt Multimedia (+ MultimediaWidgets) - video playback &amp; audio (QMediaPlayer, QVideoSink)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>GStreamer / FFmpeg - media backends behind Qt Multimedia (selected at runtime)</source>
        <translation>GStreamer / FFmpeg - media backends behind Qt Multimedia (selected at runtime)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>CMake + Ninja - build system</source>
        <translation>CMake + Ninja - build system</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>C++20 - language standard</source>
        <translation>C++20 - language standard</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>QSettings (INI) - AppSettings persistence (appearance, updates, server, playback)</source>
        <translation>QSettings (INI) - AppSettings persistence (appearance, updates, server, playback)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>QSystemTrayIcon - tray, notifications, close-to-tray</source>
        <translation>QSystemTrayIcon - tray, notifications, close-to-tray</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>QLocalServer / QLocalSocket - SingleInstance (single-instance + raise)</source>
        <translation>QLocalServer / QLocalSocket - SingleInstance (single-instance + raise)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Inter font family - bundled UI typeface (via FontLoader)</source>
        <translation>Inter font family - bundled UI typeface (via FontLoader)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Lucide icons - SVG icon set</source>
        <translation>Lucide icons - SVG icon set</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>linuxdeploy + linuxdeploy-plugin-qt - AppImage bundling</source>
        <translation>linuxdeploy + linuxdeploy-plugin-qt - AppImage bundling</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>CPack - DEB / NSIS / WiX (MSI) installers</source>
        <translation>CPack - DEB / NSIS / WiX (MSI) installers</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Snapcraft (core24 + gnome extension) - Snap package</source>
        <translation>Snapcraft (core24 + gnome extension) - Snap package</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Flatpak (org.kde.Platform / Sdk) - Flatpak manifest</source>
        <translation>Flatpak (org.kde.Platform / Sdk) - Flatpak manifest</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>GitHub Actions - CI for Windows/Linux/Snap + tagged releases</source>
        <translation>GitHub Actions - CI for Windows/Linux/Snap + tagged releases</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>NSIS &amp; WiX Toolset - Windows installers</source>
        <translation>NSIS &amp; WiX Toolset - Windows installers</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Get in touch &amp; collaborate</source>
        <translation>Get in touch &amp; collaborate</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Want to build this together, report a bug, or propose a feature? Reach out on GitHub or Discord , contributions of all sizes are welcome.</source>
        <translation>Want to build this together, report a bug, or propose a feature? Reach out on GitHub or Discord , contributions of all sizes are welcome.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Discord: diegosanche3</source>
        <translation>Discord: diegosanche3</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Copy Discord handle</source>
        <translation>Copy Discord handle</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Open GitHub</source>
        <translation>Open GitHub</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Discord handle: diegosanche3</source>
        <translation>Discord handle: diegosanche3</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Version</source>
        <translation>Version</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>v%1 (Snap %2)</source>
        <translation>v%1 (Snap %2)</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Licensed under MIT. See LICENSE.txt. Immich is a trademark of its owners; this app is an independent project and does not redistribute official Immich branding.</source>
        <translation>Licensed under MIT. See LICENSE.txt. Immich is a trademark of its owners; this app is an independent project and does not redistribute official Immich branding.</translation>
    </message>
</context>
<context>
    <name>Aurora::AppearancePage</name>
    <message>
        <location filename="../src/ui/pages/AppearancePage.cpp" line="+90"/>
        <source>Close to system tray</source>
        <translation>Close to system tray</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Start immich when I sign in</source>
        <translation>Start immich when I sign in</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Appearance</source>
        <translation>Appearance</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Choose a preset or build a palette that belongs to your product.</source>
        <translation>Choose a preset or build a palette that belongs to your product.</translation>
    </message>
    <message>
        <location line="+16"/>
        <location line="+5"/>
        <source>Language</source>
        <translation>Language</translation>
    </message>
    <message>
        <location line="-4"/>
        <source>Language changes apply immediately. Untranslated text appears in English.</source>
        <translation>Language changes apply immediately. Untranslated text appears in English.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>System default</source>
        <translation>System default</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Theme preset</source>
        <translation>Theme preset</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Preset changes are applied immediately and saved automatically.</source>
        <translation>Preset changes are applied immediately and saved automatically.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Light</source>
        <translation>Light</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark</source>
        <translation>Dark</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Custom</source>
        <translation>Custom</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Icons and interface</source>
        <translation>Icons and interface</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Choose an icon pack and simplify action buttons. Changes are applied immediately and saved automatically.</source>
        <translation>Choose an icon pack and simplify action buttons. Changes are applied immediately and saved automatically.</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+3"/>
        <source>Icon pack</source>
        <translation>Icon pack</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Lucide (default)</source>
        <translation>Lucide (default)</translation>
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
        <translation>Preview: upload, library, settings</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Minimalist UI (icons instead of button labels)</source>
        <translation>Minimalist UI (icons instead of button labels)</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Navigation and action buttons use icons. Hover over an icon to see its action.</source>
        <translation>Navigation and action buttons use icons. Hover over an icon to see its action.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Custom palette</source>
        <translation>Custom palette</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Choosing a color automatically activates the Custom theme.</source>
        <translation>Choosing a color automatically activates the Custom theme.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Background</source>
        <translation>Background</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Panels</source>
        <translation>Panels</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Buttons</source>
        <translation>Buttons</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Borders</source>
        <translation>Borders</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Text</source>
        <translation>Text</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Muted text</source>
        <translation>Muted text</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Accent</source>
        <translation>Accent</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Reset custom colors</source>
        <translation>Reset custom colors</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Desktop</source>
        <translation>Desktop</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Control how immich behaves on your desktop and at sign-in.</source>
        <translation>Control how immich behaves on your desktop and at sign-in.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Closing the window keeps immich running in the tray. Press Ctrl+Q to quit.</source>
        <translation>Closing the window keeps immich running in the tray. Press Ctrl+Q to quit.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>System tray is not available on this desktop.</source>
        <translation>System tray is not available on this desktop.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Autostart is not supported on this platform.</source>
        <translation>Autostart is not supported on this platform.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Playback</source>
        <translation>Playback</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Control preview and playback behavior.</source>
        <translation>Control preview and playback behavior.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable video hover preview</source>
        <translation>Enable video hover preview</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Plays a short muted clip when you hover a video tile. Disable this if hovering causes freezes or stutter.</source>
        <translation>Plays a short muted clip when you hover a video tile. Disable this if hovering causes freezes or stutter.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Note: the Snap build runs confined, so hover preview is disabled in the Snap. If video playback doesn&apos;t work from the Snap, install the GitHub release (&lt;a href=&quot;https://github.com/hdmain/immich-desktop/releases&quot;&gt;.deb / AppImage&lt;/a&gt;) , it uses your system&apos;s GStreamer/FFmpeg without confinement.</source>
        <translation>Note: the Snap build runs confined, so hover preview is disabled in the Snap. If video playback doesn&apos;t work from the Snap, install the GitHub release (&lt;a href=&quot;https://github.com/hdmain/immich-desktop/releases&quot;&gt;.deb / AppImage&lt;/a&gt;) , it uses your system&apos;s GStreamer/FFmpeg without confinement.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Hover preview is unavailable in the Snap build.</source>
        <translation>Hover preview is unavailable in the Snap build.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Timeline</source>
        <translation>Timeline</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Choose how your library is laid out.</source>
        <translation>Choose how your library is laid out.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Compact grid (no date headers)</source>
        <translation>Compact grid (no date headers)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Uniform square thumbnails packed edge to edge, like a phone&apos;s photo grid, instead of date-grouped rows of varying sizes.</source>
        <translation>Uniform square thumbnails packed edge to edge, like a phone&apos;s photo grid, instead of date-grouped rows of varying sizes.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Cache</source>
        <translation>Cache</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Local storage used for thumbnails and offline library data.</source>
        <translation>Local storage used for thumbnails and offline library data.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Refresh</source>
        <translation>Refresh</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Cache size: %1</source>
        <translation>Cache size: %1</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Autostart</source>
        <translation>Autostart</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Could not update autostart settings.</source>
        <translation>Could not update autostart settings.</translation>
    </message>
</context>
<context>
    <name>Aurora::AssetInfoPanel</name>
    <message>
        <location filename="../src/ui/widgets/AssetInfoPanel.cpp" line="+55"/>
        <source>Info</source>
        <translation>Info</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Hide info (I)</source>
        <translation>Hide info (I)</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>Details unavailable
%1</source>
        <translation>Details unavailable
%1</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Albums need the album.read permission</source>
        <translation>Albums need the album.read permission</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Albums unavailable</source>
        <translation>Albums unavailable</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Loading details…</source>
        <translation>Loading details…</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Show photos from this day</source>
        <translation>Show photos from this day</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Date taken</source>
        <translation>Date taken</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>%1 MP</source>
        <translation>%1 MP</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>File</source>
        <translation>File</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Camera</source>
        <translation>Camera</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Show photos from %1</source>
        <translation>Show photos from %1</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Map coming soon</source>
        <translation>Map coming soon</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Location</source>
        <translation>Location</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Shared by</source>
        <translation>Shared by</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Shared by %1</source>
        <translation>Shared by %1</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+39"/>
        <source>People</source>
        <translation>People</translation>
    </message>
    <message>
        <location line="-38"/>
        <source>No people recognized</source>
        <translation>No people recognized</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Show photos of %1</source>
        <translation>Show photos of %1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>this person</source>
        <translation>this person</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Unnamed</source>
        <translation>Unnamed</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Description</source>
        <translation>Description</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>No description</source>
        <translation>No description</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Loading albums…</source>
        <translation>Loading albums…</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Albums</source>
        <translation>Albums</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Show photos tagged %1</source>
        <translation>Show photos tagged %1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Tags</source>
        <translation>Tags</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Show the album %1</source>
        <translation>Show the album %1</translation>
    </message>
    <message>
        <location line="-47"/>
        <source>No tags</source>
        <translation>No tags</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Show the photos in this folder</source>
        <translation>Show the photos in this folder</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Folder</source>
        <translation>Folder</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Not in any album</source>
        <translation>Not in any album</translation>
    </message>
</context>
<context>
    <name>Aurora::BrowseTree</name>
    <message>
        <location filename="../src/ui/widgets/BrowseTree.cpp" line="+50"/>
        <location line="+125"/>
        <source>Loading…</source>
        <translation>Loading…</translation>
    </message>
    <message>
        <location line="-94"/>
        <source>No albums</source>
        <translation>No albums</translation>
    </message>
    <message numerus="yes">
        <location line="+5"/>
        <source>%1 · %n item(s)</source>
        <translation>
            <numerusform>%1 · %n item(s)</numerusform>
            <numerusform>%1 · %n item(s)</numerusform>
        </translation>
    </message>
    <message>
        <location line="+12"/>
        <source>No tags</source>
        <translation>No tags</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>No folders</source>
        <translation>No folders</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>Needs the %1 permission · click to retry</source>
        <translation>Needs the %1 permission · click to retry</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Couldn&apos;t be loaded · click to retry</source>
        <translation>Couldn&apos;t be loaded · click to retry</translation>
    </message>
</context>
<context>
    <name>Aurora::ColorButton</name>
    <message>
        <location filename="../src/ui/widgets/ColorButton.cpp" line="+59"/>
        <source>Choose %1 color</source>
        <translation>Choose %1 color</translation>
    </message>
</context>
<context>
    <name>Aurora::ConnectionPage</name>
    <message>
        <location filename="../src/ui/pages/ConnectionPage.cpp" line="+34"/>
        <source>Enable custom HTTP headers</source>
        <translation>Enable custom HTTP headers</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable client certificate</source>
        <translation>Enable client certificate</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Client key</source>
        <translation>Client key</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Remember password</source>
        <translation>Remember password</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Save &amp; test connection</source>
        <translation>Save &amp; test connection</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Immich server</source>
        <translation>Immich server</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Connect this desktop client to your Immich instance. The API key needs user.read, asset.read, asset.view, asset.upload, asset.download, asset.delete, and person.read permissions; album.read, tag.read and folder.read are optional and enable the Albums, Tags and Folders sections (album.read also lists a photo&apos;s albums). An optional local URL is preferred automatically when reachable.</source>
        <translation>Connect this desktop client to your Immich instance. The API key needs user.read, asset.read, asset.view, asset.upload, asset.download, asset.delete, and person.read permissions; album.read, tag.read and folder.read are optional and enable the Albums, Tags and Folders sections (album.read also lists a photo&apos;s albums). An optional local URL is preferred automatically when reachable.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Optional - e.g. http://192.168.1.10:2283</source>
        <translation>Optional - e.g. http://192.168.1.10:2283</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Paste an Immich API key</source>
        <translation>Paste an Immich API key</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>PKCS#12 (.p12/.pfx)</source>
        <translation>PKCS#12 (.p12/.pfx)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>PEM certificate + key</source>
        <translation>PEM certificate + key</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>PEM files (*.pem *.crt *.cer *.key)</source>
        <translation>PEM files (*.pem *.crt *.cer *.key)</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Browse…</source>
        <translation>Browse…</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Select client certificate</source>
        <translation>Select client certificate</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>PKCS#12 bundles (*.p12 *.pfx)</source>
        <translation>PKCS#12 bundles (*.p12 *.pfx)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Select client key</source>
        <translation>Select client key</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Select CA certificate</source>
        <translation>Select CA certificate</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Immich URL</source>
        <translation>Immich URL</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Local URL</source>
        <translation>Local URL</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>API Key</source>
        <translation>API Key</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Sent with every request to your Immich server, including the local URL and video playback, but not to third-party CDN addresses. Values are stored locally in this app&apos;s plaintext settings file.</source>
        <translation>Sent with every request to your Immich server, including the local URL and video playback, but not to third-party CDN addresses. Values are stored locally in this app&apos;s plaintext settings file.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Add header</source>
        <translation>Add header</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Format</source>
        <translation>Format</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Client cert</source>
        <translation>Client cert</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Server CA</source>
        <translation>Server CA</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Certificate that signed the server (or its reverse proxy). Needed when the server uses a private/internal CA. This is separate from the client certificate above and is not taken from the bundle.</source>
        <translation>Certificate that signed the server (or its reverse proxy). Needed when the server uses a private/internal CA. This is separate from the client certificate above and is not taken from the bundle.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Key password</source>
        <translation>Key password</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>The API key is stored locally in this app&apos;s settings and is never sent anywhere except your configured Immich server. When a local URL is set, the app probes it every few seconds and uses it while available.</source>
        <translation>The API key is stored locally in this app&apos;s settings and is never sent anywhere except your configured Immich server. When a local URL is set, the app probes it every few seconds and uses it while available.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Enter your server details to load the media library.</source>
        <translation>Enter your server details to load the media library.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Save as new profile…</source>
        <translation>Save as new profile…</translation>
    </message>
    <message>
        <location line="+68"/>
        <source>Delete profile…</source>
        <translation>Delete profile…</translation>
    </message>
    <message>
        <location line="-65"/>
        <location line="+42"/>
        <location line="+4"/>
        <source>New profile</source>
        <translation>New profile</translation>
    </message>
    <message>
        <location line="-46"/>
        <location line="+46"/>
        <location line="+14"/>
        <source>Profile name</source>
        <translation>Profile name</translation>
    </message>
    <message>
        <location line="-41"/>
        <location line="+32"/>
        <source>User profiles</source>
        <translation>User profiles</translation>
    </message>
    <message>
        <location line="-29"/>
        <source>Each profile has its own connection, library cache, upload queue and folder sync settings. Switching pauses the previous profile&apos;s uploads.</source>
        <translation>Each profile has its own connection, library cache, upload queue and folder sync settings. Switching pauses the previous profile&apos;s uploads.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Active profile</source>
        <translation>Active profile</translation>
    </message>
    <message>
        <location line="+23"/>
        <location line="+186"/>
        <source>Could not save the profile.</source>
        <translation>Could not save the profile.</translation>
    </message>
    <message>
        <location line="-180"/>
        <source>Rename</source>
        <translation>Rename</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Rename profile</source>
        <translation>Rename profile</translation>
    </message>
    <message>
        <location line="+16"/>
        <location line="+4"/>
        <location line="+3"/>
        <location line="+3"/>
        <source>Delete profile</source>
        <translation>Delete profile</translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Switch to another profile before deleting this one.</source>
        <translation>Switch to another profile before deleting this one.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Choose an inactive profile</source>
        <translation>Choose an inactive profile</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Delete %1 and its saved connection settings?</source>
        <translation>Delete %1 and its saved connection settings?</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Could not delete the profile.</source>
        <translation>Could not delete the profile.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Use this profile on launch</source>
        <translation>Use this profile on launch</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Header name</source>
        <translation>Header name</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>value</source>
        <translation>value</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Header value</source>
        <translation>Header value</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+1"/>
        <source>Remove header</source>
        <translation>Remove header</translation>
    </message>
    <message>
        <location line="+50"/>
        <source>Enter an HTTP(S) Immich URL without embedded credentials.</source>
        <translation>Enter an HTTP(S) Immich URL without embedded credentials.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Enter an API key.</source>
        <translation>Enter an API key.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Local URL must be HTTP(S) and must not contain embedded credentials.</source>
        <translation>Local URL must be HTTP(S) and must not contain embedded credentials.</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Testing connection…</source>
        <translation>Testing connection…</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>✓ %1</source>
        <translation>✓ %1</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Could not connect: %1</source>
        <translation>Could not connect: %1</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Using local endpoint: %1</source>
        <translation>Using local endpoint: %1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Using remote endpoint: %1</source>
        <translation>Using remote endpoint: %1</translation>
    </message>
</context>
<context>
    <name>Aurora::DashboardPage</name>
    <message>
        <location filename="../src/ui/pages/DashboardPage.cpp" line="+46"/>
        <source>A clean foundation for what comes next.</source>
        <translation>A clean foundation for what comes next.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Add product modules without coupling them to navigation or appearance.</source>
        <translation>Add product modules without coupling them to navigation or appearance.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>SYSTEM</source>
        <translation>SYSTEM</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Runtime themes</source>
        <translation>Runtime themes</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The entire interface responds instantly to palette changes.</source>
        <translation>The entire interface responds instantly to palette changes.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>ARCHITECTURE</source>
        <translation>ARCHITECTURE</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Modular by design</source>
        <translation>Modular by design</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+3"/>
        <source>WORKSPACE</source>
        <translation>WORKSPACE</translation>
    </message>
    <message>
        <location line="-3"/>
        <location line="+3"/>
        <source>Future module</source>
        <translation>Future module</translation>
    </message>
    <message>
        <location line="-2"/>
        <source>This card can become analytics, media, account, or workflow functionality.</source>
        <translation>This card can become analytics, media, account, or workflow functionality.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Use the central theme manager to keep every new surface visually consistent.</source>
        <translation>Use the central theme manager to keep every new surface visually consistent.</translation>
    </message>
</context>
<context>
    <name>Aurora::ExplorePage</name>
    <message>
        <location filename="../src/ui/pages/ExplorePage.cpp" line="+189"/>
        <location line="+730"/>
        <source>Explore</source>
        <translation>Explore</translation>
    </message>
    <message>
        <location line="-728"/>
        <source>People, places, and recent media</source>
        <translation>People, places, and recent media</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+3"/>
        <location line="+1"/>
        <location line="+177"/>
        <source>Refresh</source>
        <translation>Refresh</translation>
    </message>
    <message>
        <location line="-141"/>
        <location line="+659"/>
        <location line="+182"/>
        <source>People</source>
        <translation>People</translation>
    </message>
    <message>
        <location line="-840"/>
        <source>Places</source>
        <translation>Places</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Recently added</source>
        <translation>Recently added</translation>
    </message>
    <message>
        <location line="+51"/>
        <location line="+10"/>
        <source>Restore all</source>
        <translation>Restore all</translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+14"/>
        <source>Empty trash</source>
        <translation>Empty trash</translation>
    </message>
    <message>
        <location line="-5"/>
        <source>Restore everything in the trash to your library?</source>
        <translation>Restore everything in the trash to your library?</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Permanently delete everything in the trash? This can&apos;t be undone.</source>
        <translation>Permanently delete everything in the trash? This can&apos;t be undone.</translation>
    </message>
    <message>
        <location line="+32"/>
        <location line="+1230"/>
        <source>Load more</source>
        <translation>Load more</translation>
    </message>
    <message>
        <location line="-1074"/>
        <source>This folder couldn&apos;t be loaded.</source>
        <translation>This folder couldn&apos;t be loaded.</translation>
    </message>
    <message>
        <location line="+182"/>
        <source>Loading explore…</source>
        <translation>Loading explore…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Explore could not be refreshed right now.</source>
        <translation>Explore could not be refreshed right now.</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>Unknown</source>
        <translation>Unknown</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>%1 people · %2 places · %3 recent · offline</source>
        <translation>%1 people · %2 places · %3 recent · offline</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>%1 people · %2 places · %3 recent</source>
        <translation>%1 people · %2 places · %3 recent</translation>
    </message>
    <message>
        <location line="+74"/>
        <source>Load explore</source>
        <translation>Load explore</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Couldn&apos;t load more: %1</source>
        <translation>Couldn&apos;t load more: %1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>%1 failed: %2</source>
        <translation>%1 failed: %2</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Couldn&apos;t load photos: %1</source>
        <translation>Couldn&apos;t load photos: %1</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>No people, places, or recent media yet.
Run face and location recognition on your Immich server, or add photos.</source>
        <translation>No people, places, or recent media yet.
Run face and location recognition on your Immich server, or add photos.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Connect to your Immich server in Settings → Immich Server
to explore people, places, and recent media.</source>
        <translation>Connect to your Immich server in Settings → Immich Server
to explore people, places, and recent media.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Unknown person</source>
        <translation>Unknown person</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+24"/>
        <location line="+116"/>
        <source>Loading photos…</source>
        <translation>Loading photos…</translation>
    </message>
    <message>
        <location line="-102"/>
        <location line="+131"/>
        <source>Albums</source>
        <translation>Albums</translation>
    </message>
    <message>
        <location line="-107"/>
        <location line="+152"/>
        <source>Tags</source>
        <translation>Tags</translation>
    </message>
    <message>
        <location line="-127"/>
        <location line="+138"/>
        <source>Folders</source>
        <translation>Folders</translation>
    </message>
    <message>
        <location line="-110"/>
        <source>Favorites</source>
        <translation>Favorites</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Archive</source>
        <translation>Archive</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Trash</source>
        <translation>Trash</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>No albums yet</source>
        <translation>No albums yet</translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n album(s)</source>
        <translation>
            <numerusform>%n album(s)</numerusform>
            <numerusform>%n album(s)</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+3"/>
        <source>%1
%n item(s)</source>
        <translation>
            <numerusform>%1
%n item(s)</numerusform>
            <numerusform>%1
%n item(s)</numerusform>
        </translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Unnamed</source>
        <translation>Unnamed</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 (hidden)</source>
        <translation>%1 (hidden)</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>No people recognized yet</source>
        <translation>No people recognized yet</translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n person(s)</source>
        <translation>
            <numerusform>%n person(s)</numerusform>
            <numerusform>%n person(s)</numerusform>
        </translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Hide hidden</source>
        <translation>Hide hidden</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show hidden (%1)</source>
        <translation>Show hidden (%1)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Hide people marked hidden in Immich</source>
        <translation>Hide people marked hidden in Immich</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show people marked hidden in Immich</source>
        <translation>Show people marked hidden in Immich</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>No tags yet</source>
        <translation>No tags yet</translation>
    </message>
    <message numerus="yes">
        <location line="+0"/>
        <source>%n tag(s)</source>
        <translation>
            <numerusform>%n tag(s)</numerusform>
            <numerusform>%n tag(s)</numerusform>
        </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>No folders</source>
        <translation>No folders</translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n folder(s)</source>
        <translation>
            <numerusform>%n folder(s)</numerusform>
            <numerusform>%n folder(s)</numerusform>
        </translation>
    </message>
    <message>
        <location line="+111"/>
        <location line="+14"/>
        <source>Trash is empty</source>
        <translation>Trash is empty</translation>
    </message>
    <message numerus="yes">
        <location line="-8"/>
        <source> · deleted permanently after %n day(s)</source>
        <translation>
            <numerusform> · deleted permanently after %n day(s)</numerusform>
            <numerusform> · deleted permanently after %n day(s)</numerusform>
        </translation>
    </message>
    <message>
        <location line="+92"/>
        <source>Unknown date</source>
        <translation>Unknown date</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Today</source>
        <translation>Today</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Yesterday</source>
        <translation>Yesterday</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Photos</source>
        <translation>Photos</translation>
    </message>
    <message>
        <location line="-122"/>
        <location line="+136"/>
        <source>Nothing here yet</source>
        <translation>Nothing here yet</translation>
    </message>
    <message numerus="yes">
        <location line="-133"/>
        <source>%n item(s)</source>
        <translation>
            <numerusform>%n item</numerusform>
            <numerusform>%n items</numerusform>
        </translation>
    </message>
    <message>
        <location line="-109"/>
        <location line="+453"/>
        <source>Loading…</source>
        <translation>Loading…</translation>
    </message>
</context>
<context>
    <name>Aurora::FolderSyncService</name>
    <message>
        <location filename="../src/core/FolderSyncService.cpp" line="+505"/>
        <source>Folder sync is turned off.</source>
        <translation>Folder sync is turned off.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Choose a folder to watch for photos and videos.</source>
        <translation>Choose a folder to watch for photos and videos.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Sync folder does not exist: %1</source>
        <translation>Sync folder does not exist: %1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Connect to Immich in Settings → Immich Server first.</source>
        <translation>Connect to Immich in Settings → Immich Server first.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Waiting for Immich on the local network. Files already show in Library with an unsaved badge.</source>
        <translation>Waiting for Immich on the local network. Files already show in Library with an unsaved badge.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Saving to Immich…</source>
        <translation>Saving to Immich…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Saving %1…</source>
        <translation>Saving %1…</translation>
    </message>
    <message numerus="yes">
        <location line="+5"/>
        <source>Waiting for %n file(s) to finish copying…</source>
        <translation>
            <numerusform>Waiting for %n file to finish copying…</numerusform>
            <numerusform>Waiting for %n files to finish copying…</numerusform>
        </translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Could not sync %1: %2</source>
        <translation>Could not sync %1: %2</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Offline - new files will upload when Immich is reachable.</source>
        <translation>Offline - new files will upload when Immich is reachable.</translation>
    </message>
    <message numerus="yes">
        <location line="+9"/>
        <source>Up to date with Immich (%n synced file(s)).</source>
        <translation>
            <numerusform>Up to date with Immich (%n synced file).</numerusform>
            <numerusform>Up to date with Immich (%n synced files).</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>Aurora::ImmichClient</name>
    <message>
        <location filename="../src/core/ImmichClient.cpp" line="+441"/>
        <source>Connection</source>
        <translation>Connection</translation>
    </message>
    <message>
        <location line="+153"/>
        <source>Video stream</source>
        <translation>Video stream</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Could not start the local video stream proxy.</source>
        <translation>Could not start the local video stream proxy.</translation>
    </message>
    <message>
        <location line="+20"/>
        <location line="+2"/>
        <source>Play video</source>
        <translation>Play video</translation>
    </message>
    <message>
        <location line="+0"/>
        <location line="+1069"/>
        <location line="+85"/>
        <location line="+24"/>
        <location line="+76"/>
        <location line="+291"/>
        <source>The asset identifier is invalid.</source>
        <translation>The asset identifier is invalid.</translation>
    </message>
    <message>
        <location line="-1514"/>
        <source>Set the Immich server URL and API key in Settings first.</source>
        <translation>Set the Immich server URL and API key in Settings first.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>TLS handshake failed — check your client certificate and CA certificate.</source>
        <translation>TLS handshake failed — check your client certificate and CA certificate.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Authentication failed. Check the API key and its permissions.</source>
        <translation>Authentication failed. Check the API key and its permissions.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Connection test</source>
        <translation>Connection test</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Server URL and API key are required.</source>
        <translation>Server URL and API key are required.</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>local endpoint</source>
        <translation>local endpoint</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>remote endpoint</source>
        <translation>remote endpoint</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Connected via %1.</source>
        <translation>Connected via %1.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Connected as %1 via %2.</source>
        <translation>Connected as %1 via %2.</translation>
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
        <translation>Load library</translation>
    </message>
    <message>
        <location line="-151"/>
        <source>You&apos;re offline - Years/Month browsing needs a connection.</source>
        <translation>You&apos;re offline - Years/Month browsing needs a connection.</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>You&apos;re offline.</source>
        <translation>You&apos;re offline.</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Check for new photos</source>
        <translation>Check for new photos</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+17"/>
        <location line="+47"/>
        <source>Search</source>
        <translation>Search</translation>
    </message>
    <message>
        <location line="-46"/>
        <source>You are offline and no cached library is available.</source>
        <translation>You are offline and no cached library is available.</translation>
    </message>
    <message>
        <location line="+89"/>
        <location line="+8"/>
        <location line="+115"/>
        <location line="+10"/>
        <location line="+173"/>
        <location line="+38"/>
        <source>Load explore</source>
        <translation>Load explore</translation>
    </message>
    <message>
        <location line="-335"/>
        <source>You are offline and no cached explore data is available.</source>
        <translation>You are offline and no cached explore data is available.</translation>
    </message>
    <message>
        <location line="+348"/>
        <location line="+90"/>
        <location line="+787"/>
        <location line="+52"/>
        <source>Load image</source>
        <translation>Load image</translation>
    </message>
    <message>
        <location line="-925"/>
        <source>The person identifier is invalid.</source>
        <translation>The person identifier is invalid.</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Offline - person photo not cached.</source>
        <translation>Offline - person photo not cached.</translation>
    </message>
    <message>
        <location line="+24"/>
        <location line="+907"/>
        <source>Unsupported image format.</source>
        <translation>Unsupported image format.</translation>
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
        <translation>Upload</translation>
    </message>
    <message>
        <location line="-120"/>
        <source>File not found: %1</source>
        <translation>File not found: %1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>File is empty: %1</source>
        <translation>File is empty: %1</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Could not open %1 after several tries: %2</source>
        <translation>Could not open %1 after several tries: %2</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Could not open %1: %2 (will retry)</source>
        <translation>Could not open %1: %2 (will retry)</translation>
    </message>
    <message>
        <location line="+76"/>
        <source>%1 interrupted - queued for retry when online.</source>
        <translation>%1 interrupted - queued for retry when online.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>%1: %2</source>
        <translation>%1: %2</translation>
    </message>
    <message>
        <location line="+36"/>
        <location line="+3"/>
        <location line="+8"/>
        <location line="+38"/>
        <location line="+11"/>
        <location line="+11"/>
        <source>Download</source>
        <translation>Download</translation>
    </message>
    <message>
        <location line="-59"/>
        <location line="+38"/>
        <location line="+22"/>
        <source>Could not write %1: %2</source>
        <translation>Could not write %1: %2</translation>
    </message>
    <message>
        <location line="+113"/>
        <location line="+3"/>
        <location line="+39"/>
        <location line="+10"/>
        <location line="+8"/>
        <source>Copy</source>
        <translation>Copy</translation>
    </message>
    <message>
        <location line="-17"/>
        <location line="+10"/>
        <source>This original is larger than the 64 MB clipboard safety limit. Download it to disk instead.</source>
        <translation>This original is larger than the 64 MB clipboard safety limit. Download it to disk instead.</translation>
    </message>
    <message>
        <location line="+47"/>
        <location line="+9"/>
        <source>Trash</source>
        <translation>Trash</translation>
    </message>
    <message>
        <location line="+27"/>
        <location line="+12"/>
        <location line="+21"/>
        <source>Delete</source>
        <translation>Delete</translation>
    </message>
    <message>
        <location line="-21"/>
        <source>No valid asset identifiers were provided.</source>
        <translation>No valid asset identifiers were provided.</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Favorite</source>
        <translation>Favorite</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Favorites need the asset.update permission on your API key.</source>
        <translation>Favorites need the asset.update permission on your API key.</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Connect to Immich and choose an uploaded photo or video.</source>
        <translation>Connect to Immich and choose an uploaded photo or video.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Set a valid Immich URL in server settings first.</source>
        <translation>Set a valid Immich URL in server settings first.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>You are offline. Connect to Immich to create a share link.</source>
        <translation>You are offline. Connect to Immich to create a share link.</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Immich returned an invalid share link.</source>
        <translation>Immich returned an invalid share link.</translation>
    </message>
</context>
<context>
    <name>Aurora::LibraryPage</name>
    <message>
        <location filename="../src/ui/pages/LibraryPage.cpp" line="+114"/>
        <location line="+1637"/>
        <source>Upload</source>
        <translation>Upload</translation>
    </message>
    <message>
        <location line="-1636"/>
        <source>Refresh</source>
        <translation>Refresh</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+150"/>
        <location line="+29"/>
        <source>Years</source>
        <translation>Years</translation>
    </message>
    <message>
        <location line="-141"/>
        <source>Photos &amp; videos</source>
        <translation>Photos &amp; videos</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Your Immich library</source>
        <translation>Your Immich library</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Search photos and videos…</source>
        <translation>Search photos and videos…</translation>
    </message>
    <message>
        <location line="+14"/>
        <location line="+1"/>
        <source>Search settings</source>
        <translation>Search settings</translation>
    </message>
    <message>
        <location line="+67"/>
        <source>Drop photos or videos to upload</source>
        <translation>Drop photos or videos to upload</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Library</source>
        <translation>Library</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>Refine your search</source>
        <translation>Refine your search</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Reset filters</source>
        <translation>Reset filters</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>All assets</source>
        <translation>All assets</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Favorites</source>
        <translation>Favorites</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Archive</source>
        <translation>Archive</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Search in</source>
        <translation>Search in</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+161"/>
        <source>All people</source>
        <translation>All people</translation>
    </message>
    <message>
        <location line="-158"/>
        <source>People</source>
        <translation>People</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>City</source>
        <translation>City</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Places</source>
        <translation>Places</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Photos and videos</source>
        <translation>Photos and videos</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Photos</source>
        <translation>Photos</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Videos</source>
        <translation>Videos</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Media</source>
        <translation>Media</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Date range</source>
        <translation>Date range</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>From date</source>
        <translation>From date</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>To date</source>
        <translation>To date</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>From</source>
        <translation>From</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>To</source>
        <translation>To</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Search by</source>
        <translation>Search by</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Context</source>
        <translation>Context</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>File name</source>
        <translation>File name</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Description</source>
        <translation>Description</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Full path or folder</source>
        <translation>Full path or folder</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>OCR</source>
        <translation>OCR</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Search interpretation</source>
        <translation>Search interpretation</translation>
    </message>
    <message>
        <location line="+111"/>
        <source>Unnamed person</source>
        <translation>Unnamed person</translation>
    </message>
    <message>
        <location line="+191"/>
        <location line="+1316"/>
        <source>Loading library…</source>
        <translation>Loading library…</translation>
    </message>
    <message numerus="yes">
        <location line="-1234"/>
        <location line="+42"/>
        <location line="+283"/>
        <location line="+163"/>
        <location line="+779"/>
        <source>%n item(s)</source>
        <translation>
            <numerusform>%n item</numerusform>
            <numerusform>%n items</numerusform>
        </translation>
    </message>
    <message>
        <location line="-1264"/>
        <source>Loading library… %1/%2 months</source>
        <translation>Loading library… %1/%2 months</translation>
    </message>
    <message>
        <location line="+57"/>
        <source>The start date must be on or before the end date.</source>
        <translation>The start date must be on or before the end date.</translation>
    </message>
    <message>
        <location line="+131"/>
        <source>Find scenes, objects, and ideas in the visual content of your photos.</source>
        <translation>Find scenes, objects, and ideas in the visual content of your photos.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Match the original name of a photo or video file.</source>
        <translation>Match the original name of a photo or video file.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Search the descriptions saved with your photos and videos.</source>
        <translation>Search the descriptions saved with your photos and videos.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Find media by its original full path or folder.</source>
        <translation>Find media by its original full path or folder.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Find words detected inside images, such as signs, receipts, and screenshots.</source>
        <translation>Find words detected inside images, such as signs, receipts, and screenshots.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>No filters</source>
        <translation>No filters</translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n active filter(s)</source>
        <translation>
            <numerusform>%n active filter</numerusform>
            <numerusform>%n active filters</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+17"/>
        <source>%n new photo(s) found, refreshing…</source>
        <translation>
            <numerusform>%n new photo found, refreshing…</numerusform>
            <numerusform>%n new photos found, refreshing…</numerusform>
        </translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Loading more results…</source>
        <translation>Loading more results…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Searching with filters…</source>
        <translation>Searching with filters…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Searching for “%1”…</source>
        <translation>Searching for “%1”…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Loading more media…</source>
        <translation>Loading more media…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Loading your library…</source>
        <translation>Loading your library…</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Unknown date</source>
        <translation>Unknown date</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Today</source>
        <translation>Today</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Yesterday</source>
        <translation>Yesterday</translation>
    </message>
    <message numerus="yes">
        <location line="+24"/>
        <source>%n filtered result(s)</source>
        <translation>
            <numerusform>%n filtered result</numerusform>
            <numerusform>%n filtered results</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>%n result(s) for “%1”</source>
        <translation>
            <numerusform>%n result for “%1”</numerusform>
            <numerusform>%n results for “%1”</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+2"/>
        <source>%n cached item(s) · offline</source>
        <translation>
            <numerusform>%n cached item · offline</numerusform>
            <numerusform>%n cached items · offline</numerusform>
        </translation>
    </message>
    <message>
        <location line="+229"/>
        <source>Couldn&apos;t load library: %1. Refresh to retry.</source>
        <translation>Couldn&apos;t load library: %1. Refresh to retry.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Loading library… %1/%2 months (some months failed)</source>
        <translation>Loading library… %1/%2 months (some months failed)</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>You&apos;re offline - Years needs a connection to browse by month.</source>
        <translation>You&apos;re offline - Years needs a connection to browse by month.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Loading…</source>
        <translation>Loading…</translation>
    </message>
    <message>
        <location line="+104"/>
        <source>%1 - you&apos;re offline, can&apos;t load months.</source>
        <translation>%1 - you&apos;re offline, can&apos;t load months.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 - loading…</source>
        <translation>%1 - loading…</translation>
    </message>
    <message>
        <location line="+246"/>
        <source>Load image</source>
        <translation>Load image</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Copy</source>
        <translation>Copy</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Copy failed: %1</source>
        <translation>Copy failed: %1</translation>
    </message>
    <message>
        <location line="+15"/>
        <location line="+755"/>
        <source>Upload finished: %1 ok, %2 failed</source>
        <translation>Upload finished: %1 ok, %2 failed</translation>
    </message>
    <message>
        <location line="-747"/>
        <source>%1 (%2 left)</source>
        <translation>%1 (%2 left)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Upload failed: %1 (%2 left)</source>
        <translation>Upload failed: %1 (%2 left)</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>%1 failed: %2</source>
        <translation>%1 failed: %2</translation>
    </message>
    <message>
        <location line="+234"/>
        <source>Connect to your Immich server in Settings → Immich Server
to browse photos and videos.</source>
        <translation>Connect to your Immich server in Settings → Immich Server
to browse photos and videos.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>No results for these filters.
Try different filters.</source>
        <translation>No results for these filters.
Try different filters.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>No results for “%1”.
Try a different search or filters.</source>
        <translation>No results for “%1”.
Try a different search or filters.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>No photos or videos yet.
Drop files here, paste with Ctrl+V, or use Upload to add media.</source>
        <translation>No photos or videos yet.
Drop files here, paste with Ctrl+V, or use Upload to add media.</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>%1 · local</source>
        <translation>%1 · local</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 · remote</source>
        <translation>%1 · remote</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Back online , resuming uploads…</source>
        <translation>Back online , resuming uploads…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Back online , refreshing…</source>
        <translation>Back online , refreshing…</translation>
    </message>
    <message numerus="yes">
        <location line="+5"/>
        <source>%n item(s) · offline</source>
        <translation>
            <numerusform>%n item · offline</numerusform>
            <numerusform>%n items · offline</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+8"/>
        <source>%n file(s) waiting to upload</source>
        <translation>
            <numerusform>%n file waiting to upload</numerusform>
            <numerusform>%n files waiting to upload</numerusform>
        </translation>
    </message>
    <message>
        <location line="+73"/>
        <source>Preparing download…</source>
        <translation>Preparing download…</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>Download media</source>
        <translation>Download media</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Downloading %1…</source>
        <translation>Downloading %1…</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Connect to Immich in Settings before copying.</source>
        <translation>Connect to Immich in Settings before copying.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Copy works for photos , download videos instead.</source>
        <translation>Copy works for photos , download videos instead.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Copying %1…</source>
        <translation>Copying %1…</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+116"/>
        <source>photo</source>
        <translation>photo</translation>
    </message>
    <message>
        <location line="-107"/>
        <location line="+207"/>
        <source>Connect to Immich in Settings before uploading.</source>
        <translation>Connect to Immich in Settings before uploading.</translation>
    </message>
    <message>
        <location line="-198"/>
        <source>Clipboard has no image or media files to upload.</source>
        <translation>Clipboard has no image or media files to upload.</translation>
    </message>
    <message>
        <location line="+54"/>
        <source>Could not save clipboard image for upload.</source>
        <translation>Could not save clipboard image for upload.</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Could not decode image for clipboard.</source>
        <translation>Could not decode image for clipboard.</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Copied %1 to clipboard</source>
        <translation>Copied %1 to clipboard</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>this item</source>
        <translation>this item</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+10"/>
        <source>Delete permanently</source>
        <translation>Delete permanently</translation>
    </message>
    <message>
        <location line="-10"/>
        <location line="+10"/>
        <source>Move to trash</source>
        <translation>Move to trash</translation>
    </message>
    <message>
        <location line="-8"/>
        <source>Permanently delete %1? This cannot be undone.</source>
        <translation>Permanently delete %1? This cannot be undone.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Move %1 to trash? You can restore it later from Immich.</source>
        <translation>Move %1 to trash? You can restore it later from Immich.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Deleting…</source>
        <translation>Deleting…</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Moving to trash…</source>
        <translation>Moving to trash…</translation>
    </message>
    <message numerus="yes">
        <location line="+7"/>
        <source>Deleted %n item(s).</source>
        <translation>
            <numerusform>Deleted %n item.</numerusform>
            <numerusform>Deleted %n items.</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+1"/>
        <source>Moved %n item(s) to trash.</source>
        <translation>
            <numerusform>Moved %n item to trash.</numerusform>
            <numerusform>Moved %n items to trash.</numerusform>
        </translation>
    </message>
    <message>
        <location line="+59"/>
        <source>Upload media</source>
        <translation>Upload media</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>No supported media files to upload.</source>
        <translation>No supported media files to upload.</translation>
    </message>
    <message numerus="yes">
        <location line="+7"/>
        <source>Queued %n file(s) for upload when online.</source>
        <translation>
            <numerusform>Queued %n file for upload when online.</numerusform>
            <numerusform>Queued %n files for upload when online.</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location line="+2"/>
        <source>Uploading %n file(s)…</source>
        <translation>
            <numerusform>Uploading %n file…</numerusform>
            <numerusform>Uploading %n files…</numerusform>
        </translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Uploading %1… %2% (%3 left)</source>
        <translation>Uploading %1… %2% (%3 left)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Uploading %1…</source>
        <translation>Uploading %1…</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Already on server: %1</source>
        <translation>Already on server: %1</translation>
    </message>
    <message numerus="yes">
        <location line="+2"/>
        <source>Uploaded %n file(s).</source>
        <translation>
            <numerusform>Uploaded %n file.</numerusform>
            <numerusform>Uploaded %n files.</numerusform>
        </translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Duplicate skipped: %1 (%2 left)</source>
        <translation>Duplicate skipped: %1 (%2 left)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Uploaded %1 (%2 left)</source>
        <translation>Uploaded %1 (%2 left)</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Downloading… %1%</source>
        <translation>Downloading… %1%</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Saved to %1</source>
        <translation>Saved to %1</translation>
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
        <translation>Show immich</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Quit</source>
        <translation>Quit</translation>
    </message>
    <message>
        <location line="+88"/>
        <source>Library</source>
        <translation>Library</translation>
    </message>
    <message>
        <location line="-303"/>
        <location line="+306"/>
        <source>Explore</source>
        <translation>Explore</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Settings / Immich Server</source>
        <translation>Settings / Immich Server</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Settings / Folder Sync</source>
        <translation>Settings / Folder Sync</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Settings / Appearance</source>
        <translation>Settings / Appearance</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Settings / Update</source>
        <translation>Settings / Update</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Settings / About</source>
        <translation>Settings / About</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Support immich desktop</source>
        <translation>Support immich desktop</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>If immich desktop is useful, a GitHub star helps a lot.</source>
        <translation>If immich desktop is useful, a GitHub star helps a lot.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Stars make the project easier to discover. Thanks for considering it!</source>
        <translation>Stars make the project easier to discover. Thanks for considering it!</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Star on GitHub</source>
        <translation>Star on GitHub</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Maybe later</source>
        <translation>Maybe later</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Certificate password</source>
        <translation>Certificate password</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enter the password for your client certificate:</source>
        <translation>Enter the password for your client certificate:</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>immich - uploading</source>
        <translation>immich - uploading</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>immich - downloading</source>
        <translation>immich - downloading</translation>
    </message>
    <message>
        <location line="+16"/>
        <location line="+9"/>
        <source>Update available</source>
        <translation>Update available</translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+9"/>
        <source>immich desktop v%1 is available.</source>
        <translation>immich desktop v%1 is available.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>What&apos;s new:</source>
        <translation>What&apos;s new:</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Open the Updates page to download and install the package for this platform.</source>
        <translation>Open the Updates page to download and install the package for this platform.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Open Updates</source>
        <translation>Open Updates</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Later</source>
        <translation>Later</translation>
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
        <translation>Open</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+7"/>
        <source>Info</source>
        <translation>Info</translation>
    </message>
    <message>
        <location line="-4"/>
        <source>Restore</source>
        <translation>Restore</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Copy</source>
        <translation>Copy</translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+9"/>
        <source>Download</source>
        <translation>Download</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Share link…</source>
        <translation>Share link…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove from favorites</source>
        <translation>Remove from favorites</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Add to favorites</source>
        <translation>Add to favorites</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Move to trash</source>
        <translation>Move to trash</translation>
    </message>
    <message>
        <location line="-11"/>
        <location line="+12"/>
        <source>Delete permanently</source>
        <translation>Delete permanently</translation>
    </message>
</context>
<context>
    <name>Aurora::MediaViewerDialog</name>
    <message>
        <location filename="../src/ui/widgets/MediaViewerDialog.cpp" line="+48"/>
        <source>Favorite</source>
        <translation>Favorite</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy</source>
        <translation>Copy</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Download</source>
        <translation>Download</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Share link</source>
        <translation>Share link</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Move to trash</source>
        <translation>Move to trash</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Restore</source>
        <translation>Restore</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Delete permanently</source>
        <translation>Delete permanently</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Info</source>
        <translation>Info</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Show info (I)</source>
        <translation>Show info (I)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Close</source>
        <translation>Close</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Previous (Left arrow)</source>
        <translation>Previous (Left arrow)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Next (Right arrow)</source>
        <translation>Next (Right arrow)</translation>
    </message>
    <message>
        <location line="+104"/>
        <source>Preview unavailable
%1</source>
        <translation>Preview unavailable
%1</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Media preview</source>
        <translation>Media preview</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Loading preview…</source>
        <translation>Loading preview…</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Remove from favorites (F)</source>
        <translation>Remove from favorites (F)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add to favorites (F)</source>
        <translation>Add to favorites (F)</translation>
    </message>
</context>
<context>
    <name>Aurora::SettingsPage</name>
    <message>
        <location filename="../src/ui/pages/SettingsPage.cpp" line="+33"/>
        <source>Back</source>
        <translation>Back</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Back (Esc)</source>
        <translation>Back (Esc)</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Settings</source>
        <translation>Settings</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Choose a settings section from the sidebar.</source>
        <translation>Choose a settings section from the sidebar.</translation>
    </message>
</context>
<context>
    <name>Aurora::ShareLinkDialog</name>
    <message>
        <location filename="../src/ui/widgets/ShareLinkDialog.cpp" line="+19"/>
        <source>Create link</source>
        <translation>Create link</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy link</source>
        <translation>Copy link</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Share media</source>
        <translation>Share media</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Share with a link</source>
        <translation>Share with a link</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Photo or video</source>
        <translation>Photo or video</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>The link uses your Immich URL from server settings, even when the app is connected locally.</source>
        <translation>The link uses your Immich URL from server settings, even when the app is connected locally.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Password (optional)</source>
        <translation>Password (optional)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Share link password</source>
        <translation>Share link password</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Leave empty to share without a password</source>
        <translation>Leave empty to share without a password</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Show password</source>
        <translation>Show password</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Share link</source>
        <translation>Share link</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Create a link to share this media…</source>
        <translation>Create a link to share this media…</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Creating your share link…</source>
        <translation>Creating your share link…</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Link ready. A password is required to view this media.</source>
        <translation>Link ready. A password is required to view this media.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Link ready. Anyone with the link can view this media.</source>
        <translation>Link ready. Anyone with the link can view this media.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Link copied. Share the password separately.</source>
        <translation>Link copied. Share the password separately.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Link copied to clipboard.</source>
        <translation>Link copied to clipboard.</translation>
    </message>
</context>
<context>
    <name>Aurora::Sidebar</name>
    <message>
        <location filename="../src/ui/widgets/Sidebar.cpp" line="+37"/>
        <location line="+133"/>
        <source>Collapse</source>
        <translation>Collapse</translation>
    </message>
    <message>
        <location line="-77"/>
        <source>fan-made
immich desktop  ·  %1</source>
        <translation>fan-made
immich desktop  ·  %1</translation>
    </message>
    <message>
        <location line="+77"/>
        <source>Expand</source>
        <translation>Expand</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Expand sidebar (F9)</source>
        <translation>Expand sidebar (F9)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Collapse sidebar (F9)</source>
        <translation>Collapse sidebar (F9)</translation>
    </message>
    <message>
        <location line="+138"/>
        <source>Main</source>
        <translation>Main</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Library</source>
        <translation>Library</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Explore</source>
        <translation>Explore</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>People</source>
        <translation>People</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Favorites</source>
        <translation>Favorites</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Collections</source>
        <translation>Collections</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Archive</source>
        <translation>Archive</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Albums</source>
        <translation>Albums</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tags</source>
        <translation>Tags</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Folders</source>
        <translation>Folders</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tools</source>
        <translation>Tools</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Trash</source>
        <translation>Trash</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Settings</source>
        <translation>Settings</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>SETTINGS</source>
        <translation>SETTINGS</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Immich Server</source>
        <translation>Immich Server</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Folder Sync</source>
        <translation>Folder Sync</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Appearance</source>
        <translation>Appearance</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Update</source>
        <translation>Update</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>About</source>
        <translation>About</translation>
    </message>
</context>
<context>
    <name>Aurora::SummaryTile</name>
    <message numerus="yes">
        <location filename="../src/ui/widgets/SummaryTile.cpp" line="+89"/>
        <source>%n item(s)</source>
        <translation>
            <numerusform>%n item</numerusform>
            <numerusform>%n items</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>Aurora::SyncFolderPage</name>
    <message>
        <location filename="../src/ui/pages/SyncFolderPage.cpp" line="+27"/>
        <source>Watch folders and upload new media</source>
        <translation>Watch folders and upload new media</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Upload only when Immich is available on the local network</source>
        <translation>Upload only when Immich is available on the local network</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Scan now</source>
        <translation>Scan now</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+1"/>
        <source>Add another folder</source>
        <translation>Add another folder</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Folder sync</source>
        <translation>Folder sync</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Choose folders on this computer. Photos and videos dropped into them (including subfolders) are uploaded to Immich automatically on Windows, macOS, and Linux. Files stay on disk after upload.</source>
        <translation>Choose folders on this computer. Photos and videos dropped into them (including subfolders) are uploaded to Immich automatically on Windows, macOS, and Linux. Files stay on disk after upload.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Uses your Local URL from Immich Server settings. Files still appear in Library with an unsaved badge until the local server is reachable.</source>
        <translation>Uses your Local URL from Immich Server settings. Files still appear in Library with an unsaved badge until the local server is reachable.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Folders</source>
        <translation>Folders</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Status icons: saved · saving · unsaved · error saving</source>
        <translation>Status icons: saved · saving · unsaved · error saving</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Saved</source>
        <translation>Saved</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Saving</source>
        <translation>Saving</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Unsaved</source>
        <translation>Unsaved</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Error</source>
        <translation>Error</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Choose sync folder</source>
        <translation>Choose sync folder</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Add sync folder</source>
        <translation>Add sync folder</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Sync folder path</source>
        <translation>Sync folder path</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Choose a folder to sync…</source>
        <translation>Choose a folder to sync…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Browse…</source>
        <translation>Browse…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open folder</source>
        <translation>Open folder</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+1"/>
        <source>Remove folder from sync</source>
        <translation>Remove folder from sync</translation>
    </message>
</context>
<context>
    <name>Aurora::Toast</name>
    <message>
        <location filename="../src/ui/widgets/Toast.cpp" line="+34"/>
        <source>Click to dismiss</source>
        <translation>Click to dismiss</translation>
    </message>
</context>
<context>
    <name>Aurora::TopBar</name>
    <message>
        <location filename="../src/ui/widgets/TopBar.cpp" line="+25"/>
        <source>Overview</source>
        <translation>Overview</translation>
    </message>
    <message>
        <location line="+27"/>
        <location line="+1"/>
        <source>Switch profile</source>
        <translation>Switch profile</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>immich — %1</source>
        <translation>immich — %1</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>  Update available</source>
        <translation>  Update available</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>  Update to v%1</source>
        <translation>  Update to v%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open the Updates page</source>
        <translation>Open the Updates page</translation>
    </message>
</context>
<context>
    <name>Aurora::UpdateManager</name>
    <message>
        <location filename="../src/core/UpdateManager.cpp" line="+167"/>
        <source>No downloadable update package was found for this platform.</source>
        <translation>No downloadable update package was found for this platform.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>The update package has an unsafe file name.</source>
        <translation>The update package has an unsafe file name.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Unable to create the update download directory.</source>
        <translation>Unable to create the update download directory.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Unable to create the update package file.</source>
        <translation>Unable to create the update package file.</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Unable to refresh the Snap package. Try: snap refresh %1</source>
        <translation>Unable to refresh the Snap package. Try: snap refresh %1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The update package has not been downloaded yet.</source>
        <translation>The update package has not been downloaded yet.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Unable to launch the update installer.</source>
        <translation>Unable to launch the update installer.</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Update check failed: %1</source>
        <translation>Update check failed: %1</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>The update package exceeded its advertised size.</source>
        <translation>The update package exceeded its advertised size.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Unable to write the downloaded package.</source>
        <translation>Unable to write the downloaded package.</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Download failed: %1</source>
        <translation>Download failed: %1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>The update package file is unavailable.</source>
        <translation>The update package file is unavailable.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>The update package size did not match the release metadata.</source>
        <translation>The update package size did not match the release metadata.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Unable to finalize the downloaded package.</source>
        <translation>Unable to finalize the downloaded package.</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>The update server returned an unexpected response.</source>
        <translation>The update server returned an unexpected response.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The latest release does not include a version tag.</source>
        <translation>The latest release does not include a version tag.</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>No compatible installer was published for this platform.</source>
        <translation>No compatible installer was published for this platform.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>The selected update package has an unsafe file name.</source>
        <translation>The selected update package has an unsafe file name.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The selected update package has an untrusted download URL.</source>
        <translation>The selected update package has an untrusted download URL.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>The selected update package has an invalid size.</source>
        <translation>The selected update package has an invalid size.</translation>
    </message>
</context>
<context>
    <name>Aurora::UpdatesPage</name>
    <message>
        <location filename="../src/ui/pages/UpdatesPage.cpp" line="+30"/>
        <source>Check for updates automatically</source>
        <translation>Check for updates automatically</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refresh</source>
        <translation>Refresh</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Update</source>
        <translation>Update</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Updates</source>
        <translation>Updates</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Current version: %1</source>
        <translation>Current version: %1</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+3"/>
        <source>Current version: v%1 (Snap)</source>
        <translation>Current version: v%1 (Snap)</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Enjoying immich desktop?</source>
        <translation>Enjoying immich desktop?</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>A GitHub star helps others find this unofficial client and supports continued work.</source>
        <translation>A GitHub star helps others find this unofficial client and supports continued work.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Star on GitHub</source>
        <translation>Star on GitHub</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Maybe later</source>
        <translation>Maybe later</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>%p%  ·  %1 / %2</source>
        <translation>%p%  ·  %1 / %2</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>Updates for this installation are provided by your package manager.</source>
        <translation>Updates for this installation are provided by your package manager.</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Press Refresh to check the Snap Store for updates.</source>
        <translation>Press Refresh to check the Snap Store for updates.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Press Refresh to check for updates.</source>
        <translation>Press Refresh to check for updates.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Checking the Snap Store for updates…</source>
        <translation>Checking the Snap Store for updates…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Checking for updates…</source>
        <translation>Checking for updates…</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>You&apos;re up to date with the Snap Store.</source>
        <translation>You&apos;re up to date with the Snap Store.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>You&apos;re up to date.</source>
        <translation>You&apos;re up to date.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Snap Store update available: v%1</source>
        <translation>Snap Store update available: v%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Update available: v%1</source>
        <translation>Update available: v%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Downloading v%1…</source>
        <translation>Downloading v%1…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Installing v%1…</source>
        <translation>Installing v%1…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Download complete</source>
        <translation>Download complete</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Installing update…</source>
        <translation>Installing update…</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Update failed: %1</source>
        <translation>Update failed: %1</translation>
    </message>
</context>
<context>
    <name>Aurora::VideoPlayerWidget</name>
    <message>
        <location filename="../src/ui/widgets/VideoPlayerWidget.cpp" line="+49"/>
        <location line="+170"/>
        <source>Play (Space)</source>
        <translation>Play (Space)</translation>
    </message>
    <message>
        <location line="-169"/>
        <location line="+170"/>
        <source>Play</source>
        <translation>Play</translation>
    </message>
    <message>
        <location line="-159"/>
        <location line="+12"/>
        <source>Volume</source>
        <translation>Volume</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>Buffering…</source>
        <translation>Buffering…</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Playback finished</source>
        <translation>Playback finished</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Video playback failed.</source>
        <translation>Video playback failed.</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Could not start the video stream.</source>
        <translation>Could not start the video stream.</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>Pause (Space)</source>
        <translation>Pause (Space)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pause</source>
        <translation>Pause</translation>
    </message>
</context>
<context>
    <name>Aurora::ZoomSlider</name>
    <message>
        <location filename="../src/ui/widgets/ZoomSlider.cpp" line="+28"/>
        <source>Smaller thumbnails (Ctrl+−)</source>
        <translation>Smaller thumbnails (Ctrl+−)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Larger thumbnails (Ctrl++)</source>
        <translation>Larger thumbnails (Ctrl++)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Thumbnail size (Ctrl+scroll)</source>
        <translation>Thumbnail size (Ctrl+scroll)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Thumbnail size</source>
        <translation>Thumbnail size</translation>
    </message>
</context>
<context>
    <name>AutoStart</name>
    <message>
        <location filename="../src/core/AutoStart.cpp" line="+94"/>
        <source>Autostart is not supported on this platform.</source>
        <translation>Autostart is not supported on this platform.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Could not update Windows startup settings.</source>
        <translation>Could not update Windows startup settings.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Could not remove autostart entry.</source>
        <translation>Could not remove autostart entry.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Could not create autostart entry.</source>
        <translation>Could not create autostart entry.</translation>
    </message>
</context>
<context>
    <name>Profiles</name>
    <message>
        <location filename="../src/core/AppSettings.cpp" line="+48"/>
        <source>Personal</source>
        <translation>Personal</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../src/core/CustomHeaders.cpp" line="+45"/>
        <source>Row %1: header name is required.</source>
        <translation>Row %1: header name is required.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Row %1: &quot;%2&quot; is not a valid header name.</source>
        <translation>Row %1: &quot;%2&quot; is not a valid header name.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Row %1: &quot;%2&quot; is managed by the app and cannot be overridden.</source>
        <translation>Row %1: &quot;%2&quot; is managed by the app and cannot be overridden.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Row %1: header values cannot contain line breaks.</source>
        <translation>Row %1: header values cannot contain line breaks.</translation>
    </message>
    <message>
        <location filename="../src/core/ImmichClient.cpp" line="+197"/>
        <source>The server returned an image larger than 32 MB.</source>
        <translation>The server returned an image larger than 32 MB.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Authentication failed. Check the API key and its permissions.</source>
        <translation>Authentication failed. Check the API key and its permissions.</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Unsupported image format.</source>
        <translation>Unsupported image format.</translation>
    </message>
    <message>
        <location filename="../src/ui/pages/AboutPage.cpp" line="-36"/>
        <source>Copied!</source>
        <translation>Copied!</translation>
    </message>
    <message>
        <location filename="../src/ui/pages/LibraryPage.cpp" line="-2462"/>
        <source>Media files (*.jpg *.jpeg *.png *.gif *.webp *.heic *.heif *.tif *.tiff *.bmp *.raw *.dng *.cr2 *.nef *.arw *.mp4 *.mov *.m4v *.avi *.mkv *.webm *.3gp);;All files (*.*)</source>
        <translation>Media files (*.jpg *.jpeg *.png *.gif *.webp *.heic *.heif *.tif *.tiff *.bmp *.raw *.dng *.cr2 *.nef *.arw *.mp4 *.mov *.m4v *.avi *.mkv *.webm *.3gp);;All files (*.*)</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="+70"/>
        <source>Updating translations…</source>
        <translation>Updating translations…</translation>
    </message>
</context>
<context>
    <name>TlsConfig</name>
    <message>
        <location filename="../src/core/TlsConfig.cpp" line="+25"/>
        <location line="+18"/>
        <source>Could not open %1: %2</source>
        <translation>Could not open %1: %2</translation>
    </message>
    <message>
        <location line="-6"/>
        <source>A password is required to open the PKCS#12 bundle.</source>
        <translation>A password is required to open the PKCS#12 bundle.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Could not read the PKCS#12 bundle. The password may be wrong, or the file is not a valid bundle.</source>
        <translation>Could not read the PKCS#12 bundle. The password may be wrong, or the file is not a valid bundle.</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Could not read a client certificate from %1.</source>
        <translation>Could not read a client certificate from %1.</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Could not read the private key from %1. The key may be encrypted and need a password, or the password is wrong.</source>
        <translation>Could not read the private key from %1. The key may be encrypted and need a password, or the password is wrong.</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Could not read a CA certificate from %1 (tried PEM and DER).</source>
        <translation>Could not read a CA certificate from %1 (tried PEM and DER).</translation>
    </message>
</context>
</TS>
