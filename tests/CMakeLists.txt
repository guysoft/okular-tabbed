set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

kde4_add_unit_test( shelltest shelltest.cpp ../shell/shellutils.cpp )
target_link_libraries( shelltest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} )

kde4_add_unit_test( parttest parttest.cpp )
target_link_libraries( parttest ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} okularpart okularcore )

kde4_add_unit_test( searchtest searchtest.cpp )
target_link_libraries( searchtest ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} okularcore )

kde4_add_unit_test( urldetecttest urldetecttest.cpp )
target_link_libraries( urldetecttest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} )
