# This sets the following variables:
# sensors_analytics_sdk target

__check_target_return(sensors_analytics_sdk)

if(NOT TARGET sensors_analytics_sdk)
	__search_target_components(sensors_analytics_sdk
							INC sensors_analytics_sdk.h
							DLIB sensors_analytics_sdk
							LIB sensors_analytics_sdk
							PRE sensors_analytics_sdk
							)
	
	__test_import(sensors_analytics_sdk lib)
endif()
