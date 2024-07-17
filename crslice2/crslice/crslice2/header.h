#ifndef CRSLICE_HEADER_INTERFACE_2
#define CRSLICE_HEADER_INTERFACE_2
#include "crslice2/interface.h"
#include <memory>

#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "trimesh2/TriMesh_algo.h"

#include <fstream>
#include <unordered_map>
#include "ccglobal/tracer.h"

typedef std::shared_ptr<trimesh::TriMesh> TriMeshPtr;

#include "crslice2/settings.h"
namespace crslice2
{
    typedef std::shared_ptr<crslice2::Settings> SettingsPtr;
}

#endif // CRSLICE_HEADER_INTERFACE_2