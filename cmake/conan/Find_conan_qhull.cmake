# This sets the following variables:
# qhullcpp target
# qhullstatic_r target

__conan_import(qhull lib COMPONENT qhullcpp qhullstatic_r)

set(qhull qhullstatic_r qhullcpp)