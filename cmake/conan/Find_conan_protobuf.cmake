set(PROTOBUF_COMPONETS libprotobuf
					   libprotobuf-lite
					   libprotoc
					   )

__conan_import(protobuf lib COMPONENT ${PROTOBUF_COMPONETS})
set(protobuf ${PROTOBUF_COMPONETS})

if(CC_BC_WIN)
	set(protoc ${CONAN_BIN_DIRS_PROTOBUF_RELEASE}/protoc.exe)
else(CC_BC_LINUX)
	set(protoc ${CONAN_BIN_DIRS_PROTOBUF_RELEASE}/protoc)
endif()