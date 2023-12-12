include(Global)
include(log)

include(CCBuildSystem)

include(lib)
include(git)
include(conan)

include(ConfigureTarget)

include(ConfigureQt)

include(FileUtil)

include(ConfigureProperty)

include(FindUtil)
include(PackageUtil)

include(Boost)
include(openmp)

include(CXX)
include(pch)
include(Warning)

include(cc)
include(InstallUtil)

include(collect/CollectEntry)
include(qt/QtEntry)
include(render/RenderEntry)

if(CC_BC_EMCC)
	include(emcc)
endif()

include(unitTesting)

