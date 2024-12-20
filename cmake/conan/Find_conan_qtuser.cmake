# qtuser target
__cc_find(quazip)

set(QTUSER_COMPONETS qtuser_core
					 qtuser_3d
					 qtuser_quick
					 qtuser_qml
					)

__enable_qt5()
__conan_import(qtuser dll COMPONENT ${QTUSER_COMPONETS})

