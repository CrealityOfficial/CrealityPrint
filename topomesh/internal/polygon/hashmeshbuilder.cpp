#include "hashmeshbuilder.h"
#include "floatpoint.h"

namespace topomesh
{
    const int vertex_meld_distance = MM2INT(0.03);
    /*!
     * returns a hash for the location, but first divides by the vertex_meld_distance,
     * so that any point within a box of vertex_meld_distance by vertex_meld_distance would get mapped to the same hash.
     */
    static inline uint32_t pointHash(const Point3& p)
    {
        return ((p.x + vertex_meld_distance / 2) / vertex_meld_distance) ^ (((p.y + vertex_meld_distance / 2) / vertex_meld_distance) << 10) ^ (((p.z + vertex_meld_distance / 2) / vertex_meld_distance) << 20);
    }

    HashMeshBuilder::HashMeshBuilder()
        : has_disconnected_faces(false)
        , has_overlapping_faces(false)
    {

    }

    HashMeshBuilder::~HashMeshBuilder()
    {

    }

    MeshObject* HashMeshBuilder::build(bool swap)
    {
        for (unsigned int i = 0; i < faces.size(); i++)
        {
            MeshFace& face = faces[i];
            // faces are connected via the outside
            face.connected_face_index[0] = getFaceIdxWithPoints(face.vertex_index[0], face.vertex_index[1], i, face.vertex_index[2]);
            face.connected_face_index[1] = getFaceIdxWithPoints(face.vertex_index[1], face.vertex_index[2], i, face.vertex_index[0]);
            face.connected_face_index[2] = getFaceIdxWithPoints(face.vertex_index[2], face.vertex_index[0], i, face.vertex_index[1]);
        }

        if (vertices.size() > 0 && faces.size() > 0)
        {
            MeshObject* mesh = new MeshObject();
            if (swap)
            {
                std::swap(mesh->vertices, vertices);
                std::swap(mesh->faces, faces);
            }
            else
            {
                mesh->vertices = vertices;
                mesh->faces = faces;
            }

            return mesh;
        }
        return nullptr;
    }

	Mesh* HashMeshBuilder::buildMesh(bool swap)
	{
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			MeshFace& face = faces[i];
			// faces are connected via the outside
			face.connected_face_index[0] = getFaceIdxWithPoints(face.vertex_index[0], face.vertex_index[1], i, face.vertex_index[2]);
			face.connected_face_index[1] = getFaceIdxWithPoints(face.vertex_index[1], face.vertex_index[2], i, face.vertex_index[0]);
			face.connected_face_index[2] = getFaceIdxWithPoints(face.vertex_index[2], face.vertex_index[0], i, face.vertex_index[1]);
		}

		if (vertices.size() > 0 && faces.size() > 0)
		{
			Mesh* mesh = new Mesh();
			if (swap)
			{
				std::swap(mesh->vertices, vertices);
				std::swap(mesh->faces, faces);
			}
			else
			{
				mesh->vertices = vertices;
				mesh->faces = faces;
			}

			return mesh;
		}
		return nullptr;
	}

    void HashMeshBuilder::addFace(const Point3& v0, const Point3& v1, const Point3& v2)
    {
        int vi0 = findIndexOfVertex(v0);
        int vi1 = findIndexOfVertex(v1);
        int vi2 = findIndexOfVertex(v2);
        if (vi0 == vi1 || vi1 == vi2 || vi0 == vi2) return; // the face has two vertices which get assigned the same location. Don't add the face.

        int idx = faces.size(); // index of face to be added
        faces.emplace_back();
        MeshFace& face = faces[idx];
        face.vertex_index[0] = vi0;
        face.vertex_index[1] = vi1;
        face.vertex_index[2] = vi2;
        vertices[face.vertex_index[0]].connected_faces.push_back(idx);
        vertices[face.vertex_index[1]].connected_faces.push_back(idx);
        vertices[face.vertex_index[2]].connected_faces.push_back(idx);
    }

    int HashMeshBuilder::findIndexOfVertex(const Point3& v)
    {
        uint32_t hash = pointHash(v);

        for (unsigned int idx = 0; idx < vertex_hash_map[hash].size(); idx++)
        {
            if ((vertices[vertex_hash_map[hash][idx]].p - v).testLength(vertex_meld_distance))
            {
                return vertex_hash_map[hash][idx];
            }
        }
        vertex_hash_map[hash].push_back(vertices.size());
        vertices.emplace_back(v);

        return (int)vertices.size() - 1;
    }

    int HashMeshBuilder::getFaceIdxWithPoints(int idx0, int idx1, int notFaceIdx, int notFaceVertexIdx) const
    {
        std::vector<int> candidateFaces;
        for (int f : vertices[idx0].connected_faces)
        {
            if (f == notFaceIdx)
            {
                continue;
            }
            if (faces[f].vertex_index[0] == idx1
                || faces[f].vertex_index[1] == idx1
                || faces[f].vertex_index[2] == idx1
                )  candidateFaces.push_back(f);

        }

        if (candidateFaces.size() == 0)
        {
            if (!has_disconnected_faces)
            {
                //cura::LOGW("Mesh has disconnected faces!\n");
            }
            has_disconnected_faces = true;
            return -1;
        }
        if (candidateFaces.size() == 1) { return candidateFaces[0]; }


        if (candidateFaces.size() % 2 == 0)
        {
            //cura::LOGD("Warning! Edge with uneven number of faces connecting it!(%i)\n", candidateFaces.size() + 1);
            if (!has_disconnected_faces)
            {
                //cura::LOGW("Mesh has disconnected faces!\n");
            }
            has_disconnected_faces = true;
        }

        FPoint3 vn = vertices[idx1].p - vertices[idx0].p;
        FPoint3 n = vn / vn.vSize(); // the normal of the plane in which all normals of faces connected to the edge lie => the normalized normal
        FPoint3 v0 = vertices[idx1].p - vertices[idx0].p;

        // the normals below are abnormally directed! : these normals all point counterclockwise (viewed from idx1 to idx0) from the face, irrespective of the direction of the face.
        FPoint3 n0 = FPoint3(vertices[notFaceVertexIdx].p - vertices[idx0].p).cross(v0);

        if (n0.vSize() <= 0)
        {
            //cura::LOGD("Face %i has zero area!", notFaceIdx);
        }

        double smallestAngle = 1000; // more then 2 PI (impossible angle)
        int bestIdx = -1;

        for (int candidateFace : candidateFaces)
        {
            int candidateVertex;
            {// find third vertex belonging to the face (besides idx0 and idx1)
                for (candidateVertex = 0; candidateVertex < 3; candidateVertex++)
                    if (faces[candidateFace].vertex_index[candidateVertex] != idx0 && faces[candidateFace].vertex_index[candidateVertex] != idx1)
                        break;
            }

            FPoint3 v1 = vertices[faces[candidateFace].vertex_index[candidateVertex]].p - vertices[idx0].p;
            FPoint3 n1 = v0.cross(v1);

            double dot = n0 * n1;
            double det = n * n0.cross(n1);
            double angle = std::atan2(det, dot);
            if (angle < 0) angle += 2 * M_PI; // 0 <= angle < 2* M_PI

            if (angle == 0)
            {
                //cura::LOGD("Overlapping faces: face %i and face %i.\n", notFaceIdx, candidateFace);
                if (!has_overlapping_faces)
                {
                    //cura::LOGW("Mesh has overlapping faces!\n");
                }
                has_overlapping_faces = true;
            }
            if (angle < smallestAngle)
            {
                smallestAngle = angle;
                bestIdx = candidateFace;
            }
        }
        if (bestIdx < 0)
        {
            //cura::LOGD("Couldn't find face connected to face %i.\n", notFaceIdx);
            if (!has_disconnected_faces)
            {
                //cura::LOGW("Mesh has disconnected faces!\n");
            }
            has_disconnected_faces = true;
        }
        return bestIdx;
    }
}