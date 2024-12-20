set(AppDir ${CMAKE_INSTALL_PREFIX})

#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/package/linux/default.desktop DESTINATION ${AppDir})
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/package/linux/default.png DESTINATION ${AppDir})
#install openssl
install(FILES /lib/x86_64-linux-gnu/libssl.so.1.1 DESTINATION ${AppDir}/lib)
install(FILES  /lib/x86_64-linux-gnu/libcrypto.so.1.1 DESTINATION ${AppDir}/lib)
