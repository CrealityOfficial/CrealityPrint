macro(__enable_pch target headers)
	target_precompile_headers(${target} PUBLIC ${headers})
	target_compile_definitions(${target} PRIVATE ${target}_USE_PCH)
endmacro()