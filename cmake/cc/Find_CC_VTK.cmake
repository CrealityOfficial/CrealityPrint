# This sets the following variables:
# stringutil target

set(VTK_LIBRARIES vtkCommonCore
				  vtkCommonMath
				  vtkCommonSystem
				  vtkCommonDataModel
				  vtkCommonTransforms
				  vtkFiltersCore
				  vtkFiltersGeometry
				  vtkFiltersGeneral
				  vtkFiltersVerdict
				  )

foreach(vtk_library ${VTK_LIBRARIES})
	if(NOT TARGET ${vtk_library})
		__search_target_components(${vtk_library}
						INC vtkActor.h
						DLIB ${vtk_library}-8.2_d
						LIB ${vtk_library}-8.2
						PRE vtk
						)
	
		__test_import(${vtk_library} dll)
	endif()
endforeach()