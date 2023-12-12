

set(OPENCASCADE_COMPONETS TKernel
							TKMath
							TKG2d
							TKG3d
							TKGeomBase
							TKBRep
							TKGeomAlgo
							TKTopAlgo
							TKPrim
							TKBO
							TKShHealing
							TKBool
							TKHLR
							TKMesh
							TKService
							TKV3d
							TKCDF
							TKLCAF
							TKCAF
							TKBinL
							TKXmlL
							TKBin
							TKXml
							TKVCAF
							TKXSBase
							TKSTEPBase
							TKSTEPAttr
							TKSTEP209
							TKSTEP
							TKIGES
							TKXCAF
							TKXDE
							TKXDEIGES
							TKXDESTEP
							TKSTL
							TKVRML
							TKRWMesh
							TKXmlXCAF
							TKBinXCAF   
					)
					
__conan_import(opencascade_static lib COMPONENT ${OPENCASCADE_COMPONETS})