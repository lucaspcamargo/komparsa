add_executable(komparsa)

ecm_add_qml_module(komparsa
    URI
    br.eng.camargo.komparsa
)

target_sources(komparsa
    PRIVATE
    main.cpp
)

ecm_target_qml_sources(komparsa
    SOURCES
    Main.qml
)

target_link_libraries(komparsa
    PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    Qt6::QuickControls2
    Qt6::Widgets
    KF6::I18n
    KF6::CoreAddons
    KF6::IconThemes
)

install(TARGETS komparsa ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
