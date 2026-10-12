cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
cmake_policy(SET CMP0092 NEW) # don't add /W3 as default
cmake_policy(SET CMP0115 OLD) # Source file extensions must be explicit.

# set(VCPKG_OVERLAY_PORTS    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg-overlays/ports)
set(VCPKG_OVERLAY_TRIPLETS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg-overlays/triplets)
set(CMAKE_TOOLCHAIN_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project("NeTorrent")

set(CMAKE_CXX_STANDARD 17)

# --------- antlr4 options
option(WITH_STATIC_CRT "" Off)
# -----------------------

# --------- wx options
# wxWidgets now comes from vcpkg (see vcpkg.json) rather than the vendored
# submodule, so the submodule-only build knobs (wxBUILD_*, wxUSE_*) are gone.
# --------------------

# vendor libs
add_subdirectory(vendor/sentry-crashpad)

# components
add_subdirectory(src/pql)

# vcpkg packages
find_package(wxWidgets              CONFIG REQUIRED)
find_package(Boost                      REQUIRED COMPONENTS log)
find_package(fmt                 CONFIG REQUIRED)
find_package(LibtorrentRasterbar CONFIG REQUIRED)
find_package(nlohmann_json       CONFIG REQUIRED)
find_package(unofficial-sqlite3  CONFIG REQUIRED)

# Generate buildinfo file
configure_file("${CMAKE_SOURCE_DIR}/src/netorrent/buildinfo.cpp.in" "${CMAKE_SOURCE_DIR}/src/netorrent/buildinfo.cpp" @ONLY)

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")

# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG /INCREMENTAL:NO /MAP /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/DEBUG /INCREMENTAL:NO /MAP /OPT:REF /OPT:ICF")

add_executable(
    NeTorrent-coredb
    src/l10n/main
    lang/ca-ES.json
    lang/de-DE.json
    lang/el-GR.json
    lang/en-US.json
    lang/es-ES.json
    lang/fi-FI.json
    lang/fr-FR.json
    lang/hu-HU.json
    lang/hy-AM.json
    lang/id-ID.json
    lang/it-IT.json
    lang/ka-GE.json
    lang/ko-KR.json
    lang/lt-LT.json
    lang/lv-LV.json
    lang/nl-NL.json
    lang/pl-PL.json
    lang/pt-BR.json
    lang/ro-RO.json
    lang/ru-RU.json
    lang/sv-SE.json
    lang/tr-TR.json
    lang/uk-UA.json
    lang/zh-CN.json
)

target_link_libraries(
    NeTorrent-coredb
    PRIVATE
    nlohmann_json::nlohmann_json
    unofficial::sqlite3::sqlite3
)

add_executable(
    NeTorrent
    WIN32

    src/netorrent/application
    src/netorrent/buildinfo
    src/netorrent/crashpadinitializer
    src/netorrent/main
    src/netorrent/persistencemanager
    src/netorrent/resources.rc

    # API
    src/netorrent/api/libnetorrent

    # BitTorrent
    src/netorrent/bittorrent/session
    src/netorrent/bittorrent/torrenthandle

    # Core
    src/netorrent/core/configuration
    src/netorrent/core/database
    src/netorrent/core/environment
    src/netorrent/core/utils

    # Http
    src/netorrent/http/httpclient

    # IPC
    src/netorrent/ipc/applicationoptionsconnection
    src/netorrent/ipc/server

    # Dialogs
    src/netorrent/ui/dialogs/aboutdialog
    src/netorrent/ui/dialogs/addmagnetlinkdialog
    src/netorrent/ui/dialogs/addtrackerdialog
    src/netorrent/ui/dialogs/addtorrentdialog
    src/netorrent/ui/dialogs/createtorrentdialog
    src/netorrent/ui/dialogs/listeninterfacedialog
    src/netorrent/ui/dialogs/preferencesadvancedpage
    src/netorrent/ui/dialogs/preferencesconnectionpage
    src/netorrent/ui/dialogs/preferencesdialog
    src/netorrent/ui/dialogs/preferencesdownloadspage
    src/netorrent/ui/dialogs/preferencesgeneralpage
    src/netorrent/ui/dialogs/preferenceslabelspage
    src/netorrent/ui/dialogs/preferencesproxypage
    src/netorrent/ui/dialogs/textoutputdialog

    # Filters
    src/netorrent/ui/filters/pqltorrentfilter

    # Models
    src/netorrent/ui/models/filestoragemodel
    src/netorrent/ui/models/peerlistmodel
    src/netorrent/ui/models/torrentlistmodel
    src/netorrent/ui/models/trackerlistmodel

    # Persistence
    src/netorrent/ui/persistence/persistenttorrentlistview

    # UI
    src/netorrent/ui/console
    src/netorrent/ui/mainframe
    src/netorrent/ui/statusbar
    src/netorrent/ui/taskbaricon
    src/netorrent/ui/torrentcontextmenu
    src/netorrent/ui/torrentdetailsfilespanel
    src/netorrent/ui/torrentdetailsoverviewpanel
    src/netorrent/ui/torrentdetailspeerspanel
    src/netorrent/ui/torrentdetailstrackerspanel
    src/netorrent/ui/torrentdetailsview
    src/netorrent/ui/torrentfilelistview
    src/netorrent/ui/torrentlistview
    src/netorrent/ui/translator

    # Widgets
    src/netorrent/ui/widgets/pieceprogressbar

    # Win32 specific stuff
    src/netorrent/ui/win32/openfiledialog
)

target_compile_definitions(
    NeTorrent
    PRIVATE
    -D_UNICODE
    -D_WIN32
    # Boost.Log derives its ABI namespace from _WIN32_WINNT (>= Win8 => v2s_mt_nt62).
    # Our dependencies are built against the current SDK, so target Windows 10 to
    # match; 0x0600 (Vista) produced a different namespace and broke linking.
    -D_WIN32_WINNT=0x0A00
    -DNTDDI_VERSION=0x0A000000
    -DNOMINMAX
    -DPICOJSON_USE_INT64
    -DUNICODE
    -DWIN32
    -DWIN32_LEAN_AND_MEAN

    -DLIBNETORRENT_API_EXPORT

    -DNETORRENT_GIT_COMMITISH=${GITVERSION_VAR_SHORTSHA}
    -DNETORRENT_VERSION_MAJOR=${GITVERSION_VAR_VERSION_MAJOR}
    -DNETORRENT_VERSION_MINOR=${GITVERSION_VAR_VERSION_MINOR}
    -DNETORRENT_VERSION_PATCH=${GITVERSION_VAR_VERSION_PATCH}

    -DNETORRENT_WX_GIT_COMMITISH=${NETORRENT_WX_GIT_COMMITISH}
)

target_compile_options(
    NeTorrent
    PRIVATE
    /guard:cf /W4 /WX
    # We build libtorrent with its deprecated API enabled (vcpkg 'deprfun' feature)
    # on purpose for now, which makes every deprecated symbol emit C4996. /WX would
    # turn those into build errors, so silence C4996 until the 2.1 port lands.
    /wd4996
)

target_include_directories(
    NeTorrent
    PRIVATE

    include
)

target_link_libraries(
    NeTorrent

    PRIVATE

    # boost
    Boost::boost
    Boost::log

    # wxWidgets
    wx::core wx::base wx::propgrid

    # Windows
    Comctl32
    crypt32
    iphlpapi
    legacy_stdio_definitions
    propsys
    shlwapi
    wininet
    winhttp

    # Crashpad
    crashpad_client
    crashpad_util
    mini_chromium

    # fmt
    fmt::fmt

    # nlohmann-json
    nlohmann_json::nlohmann_json

    # Rasterbar-libtorrent
    LibtorrentRasterbar::torrent-rasterbar

    # PQL
    NeTorrentPQL

    unofficial::sqlite3::sqlite3
)

set_property(TARGET NeTorrent PROPERTY ENABLE_EXPORTS 1)

# Plugins
add_library(
    Plugin_Updater
    SHARED
    src/plugins/updater/updater
)

target_compile_definitions(Plugin_Updater PRIVATE -D_WIN32 -DUNICODE -DWIN32_LEAN_AND_MEAN)
target_include_directories(Plugin_Updater PRIVATE include)
target_link_libraries(Plugin_Updater PRIVATE NeTorrent Comctl32)

# Copy Crashpad handler
add_custom_command(
    TARGET NeTorrent POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:crashpad_handler>
        $<TARGET_FILE_DIR:NeTorrent>
)

# Generate coredb
add_custom_command(
    TARGET NeTorrent POST_BUILD
    COMMAND NeTorrent-coredb "${CMAKE_SOURCE_DIR}/lang" $<TARGET_FILE_DIR:NeTorrent>
)
