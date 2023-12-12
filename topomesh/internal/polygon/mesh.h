//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef CX_MESH_H
#define CX_MESH_H
#include <unordered_map>
#include "vertex.h"

namespace topomesh
{
    /*!
    A Mesh is the most basic representation of a 3D model. It contains all the faces as MeshFaces.

    See MeshFace for the specifics of how/when faces are connected.
    */
    class Mesh
    {
        //! The vertex_hash_map stores a index reference of each vertex for the hash of that location. Allows for quick retrieval of points with the same location.
        std::unordered_map<uint32_t, std::vector<uint32_t> > vertex_hash_map;
        AABB3D aabb;
    public:
        std::vector<MeshVertex> vertices;//!< list of all vertices in the mesh
        std::vector<MeshFace> faces; //!< list of all faces in the mesh

        std::string mesh_name;

        Mesh();
        ~Mesh();

        void addFace(Point3& v0, Point3& v1, Point3& v2); //!< add a face to the mesh without settings it's connected_faces.
        void clear(); //!< clears all data
        void finish(); //!< complete the model : set the connected_face_index fields of the faces.

        Point3 min() const; //!< min (in x,y and z) vertex of the bounding box
        Point3 max() const; //!< max (in x,y and z) vertex of the bounding box
        AABB3D getAABB() const; //!< Get the axis aligned bounding box
		void calculateBox();

        void expandXY(int64_t offset); //!< Register applied horizontal expansion in the AABB

        /*!
         * Offset the whole mesh (all vertices and the bounding box).
         * \param offset The offset byu which to offset the whole mesh.
         */
        void offset(Point3 offset)
        {
            if (offset == Point3(0, 0, 0)) { return; }
            for (MeshVertex& v : vertices)
                v.p += offset;
            aabb.offset(offset);
        }

    private:
        mutable bool has_disconnected_faces; //!< Whether it has been logged that this mesh contains disconnected faces
        mutable bool has_overlapping_faces; //!< Whether it has been logged that this mesh contains overlapping faces
        int findIndexOfVertex(const Point3& v); //!< find index of vertex close to the given point, or create a new vertex and return its index.

        /*!
         * Get the index of the face connected to the face with index \p notFaceIdx, via vertices \p idx0 and \p idx1.
         *
         * In case multiple faces connect with the same edge, return the next counter-clockwise face when viewing from \p idx1 to \p idx0.
         *
         * \param idx0 the first vertex index
         * \param idx1 the second vertex index
         * \param notFaceIdx the index of a face which shouldn't be returned
         * \param notFaceVertexIdx should be the third vertex of face \p notFaceIdx.
         * \return the face index of a face sharing the edge from \p idx0 to \p idx1
        */
        int getFaceIdxWithPoints(int idx0, int idx1, int notFaceIdx, int notFaceVertexIdx) const;
    };

}//namespace cxutil
#endif//CX_MESH_H

