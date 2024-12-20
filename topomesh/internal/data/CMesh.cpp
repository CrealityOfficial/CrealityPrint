#include "CMesh.h"
#include <numeric>
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unordered_set>
#include <map>
#include <math.h>

#ifndef EPS
#define EPS 1E-8F
#endif // !EPS
namespace topomesh {
    CMesh::CMesh()
    {
        mbox.clear();
        ::std::vector<PPoint>().swap(mpoints);
        ::std::vector<FFace>().swap(mfaces);
        ::std::vector<EEdge>().swap(medges);
        ::std::vector<PPoint>().swap(mnorms);
        ::std::vector<float>().swap(mareas);
        ::std::vector<float>().swap(medgeLengths);
        ::std::vector<std::vector<int>>().swap(mfaceEdges);
        ::std::vector<std::vector<int>>().swap(medgeFaces);
    }
    CMesh::~CMesh()
    {
        Clear();
    }
    void CMesh::Clear()
    {
        mbox.clear();
        std::vector<PPoint>().swap(mpoints);
        std::vector<FFace>().swap(mfaces);
        std::vector<PPoint>().swap(mnorms);
        std::vector<std::vector<int>>().swap(mfaceEdges);
        std::vector<std::vector<int>>().swap(medgeFaces);
        std::vector<float>().swap(medgeLengths);
        std::vector<EEdge>().swap(medges);
        std::vector<float>().swap(mareas);
    }
    bool CMesh::ReadFromSTL(const char* filename)
    {
        // only support ASCII or BINARY format
        // https://zhuanlan.zhihu.com/p/398208443?utm_id=0
        Clear();
        std::ifstream fid;
        std::string format, header, tail;
        char buffer[80];
        fid.open(filename, std::ios::in | std::ios::binary);
        fid.seekg(0, std::ios::end);
        size_t fidsize = fid.tellg(); // Check the size of the file
        if ((fidsize - 84) % 50 > 0)
            format = "ascii";
        else {
            fid.seekg(0, std::ios::beg);//go to the beginning of the file
            fid.read(buffer, 80);
            header = buffer;
            bool isSolid = header.find("solid") + 1;
            fid.seekg(-80, std::ios::end);          // go to the end of the file minus 80 characters
            fid.read(buffer, 80);
            tail = buffer;
            bool isEndSolid = tail.find("endsolid") + 1;
            if (isSolid && isEndSolid)
                format = "ascii";
            else format = "binary";
        }
        fid.close();
        size_t facenums = 0;
        if (format == "ascii")
        {
            // ReadASCII
            fid.open(filename, std::ios::in);
            // Read the STL header
            std::string header;
            std::getline(fid, header);
            std::string linestr;
            size_t np1, np2;
            float x, y, z;
            mfaces.reserve(facenums);
            mnorms.reserve(facenums);
            mpoints.reserve(3 * facenums);
            while (std::getline(fid, linestr)) {
                std::string alpha = linestr.substr(0, linestr.find_first_of(' '));
                if (alpha == "facet") {
                    if (linestr.back() == '\r') {
                        linestr.pop_back();
                    }
                    // skip "facet normal"
                    linestr = linestr.substr(13);
                    np1 = linestr.find_first_of(' ');
                    np2 = linestr.find_last_of(' ');
                    x = std::stof(linestr.substr(0, np1));
                    y = std::stof(linestr.substr(np1, np2));
                    z = std::stof(linestr.substr(np2));
                    //sscanf_s(linestr.c_str(), "facet normal %f %f %f", &x, &y, &z);
                    mnorms.emplace_back(x, y, z);
                    mfaces.emplace_back();
                    const auto& index = 3 * facenums;
                    mfaces.back().x = index;
                    mfaces.back().y = index + 1;
                    mfaces.back().z = index + 2;
                    ++facenums;
                } else if (alpha == "vertex") {
                    if (linestr.back() == '\r') {
                        linestr.pop_back();
                    }
                    // skip "vertex "
                    linestr = linestr.substr(7);
                    np1 = linestr.find_first_of(' ');
                    np2 = linestr.find_last_of(' ');
                    x = std::stof(linestr.substr(0, np1));
                    y = std::stof(linestr.substr(np1, np2));
                    z = std::stof(linestr.substr(np2));
                    //sscanf_s(linestr.c_str(), "vertex %f %f %f", &x, &y, &z);
                    mpoints.emplace_back(std::move(PPoint(x, y, z)));
                }
            }
            fid.close();
        } else if (format == "binary") {
            //ReadBinary;
            char objectname[80];
            fid.open(filename, std::ios::in | std::ios::binary);
            fid.read(reinterpret_cast<char*>(&objectname), sizeof(objectname));
            //fid.seekg(80, std::ios::beg);
            fid.read(reinterpret_cast<char*>(&facenums), sizeof(int));
            mpoints.resize(3 * facenums);
            mnorms.resize(facenums);
            mfaces.resize(facenums);
            for (size_t i = 0; i < facenums; ++i) {
                float pdata[12];
                fid.read(reinterpret_cast<char*>(&pdata), sizeof(float) * 12);
                //
                float* pbuffer = pdata;
                for (size_t j = 0; j < 3; ++j) {
                    mnorms[i][j] = *pbuffer;
                    ++pbuffer;
                }
                //
                const int& i0 = 3 * i;
                for (size_t j = 0; j < 3; ++j) {
                    for (size_t k = 0; k < 3; ++k) {
                        mpoints[3 * i + j][k] = *pbuffer;
                        ++pbuffer;
                    }
                    mfaces[i][j] = i0 + j;
                }

                char tailbuffer[2];
                fid.read(reinterpret_cast<char*>(&tailbuffer), sizeof(tailbuffer));
            }
            fid.close();
        } else {
            std::cout << "format error!" << std::endl;
            return false;
        }
        return true;
    }
    void CMesh::DuplicateSTL(double ratio)
    {
        const size_t pointnums = mpoints.size();
        const size_t facenums = mfaces.size();
        std::vector<PPoint> originPts;
        std::vector<FFace> originFs;
        originPts.swap(mpoints);
        originFs.swap(mfaces);
        Clear();
        struct PointHash {
            int operator()(const PPoint& p) const
            {
                return (int(p.x * 99971)) ^ (int(p.y * 99989) << 2) ^ (int(p.z * 99991) << 3);
            }
        };
        struct PointEqual {
            bool operator()(const PPoint& p1, const PPoint& p2) const
            {
                auto isEqual = [&](double a, double b, double eps = EPS) {
                    return std::fabs(a - b) < eps;
                };
                return isEqual(p1.x, p2.x) && isEqual(p1.y, p2.y) && isEqual(p1.z, p2.z);
            }
        };
        std::unordered_map<PPoint, int, PointHash, PointEqual> pointIndexMap;
        pointIndexMap.rehash(pointnums * ratio);
        std::vector<PPoint> points;
        points.reserve(pointnums);
        std::vector<int> indexs;
        indexs.reserve(pointnums);
        for (int i = 0; i < pointnums; ++i) {
            const auto& p = originPts[i];
            const auto& iter = pointIndexMap.find(p);
            if (iter != pointIndexMap.end()) {
                indexs.emplace_back(iter->second);
            } else {
                const int index = points.size();
                pointIndexMap.emplace(p, index);
                indexs.emplace_back(index);
                points.emplace_back(p);
            }
        }
        std::vector<FFace> faces;
        faces.resize(facenums);
        for (int i = 0; i < facenums; ++i) {
            for (int j = 0; j < 3; ++j) {
                faces[i][j] = indexs[originFs[i][j]];
            }
        }
        mpoints.swap(points);
        mfaces.swap(faces);
        return;
    }
    bool CMesh::WriteSTLFile(const char* filename, bool bBinary)
    {
        int face_num = mfaces.size();
        if (mnorms.empty()) {
            GenerateFaceNormals();
        }
        int normal_num = mnorms.size();
        if (face_num != normal_num) {
            std::cout << "data error!" << std::endl;
            return false;
        }
        //https://blog.csdn.net/kxh123456/article/details/105814498/
        if (bBinary) {
            std::ofstream fs(std::string(filename) + "_bin.stl", std::ios::binary | std::ios::trunc);
            if (!fs) { fs.close(); return false; }
            int intSize = sizeof(int);
            int floatSize = sizeof(float);
            //
            char fileHead[3];
            for (int i = 0; i < 3; ++i) {
                fileHead[i] = ' ';
            }
            fs.write(fileHead, sizeof(char) * 3);
            //
            char fileInfo[77];
            for (int i = 0; i < 77; ++i)
                fileInfo[i] = ' ';
            fs.write(fileInfo, sizeof(char) * 77);
            // 
            fs.write((char*)(&face_num), intSize);
            // 
            char a[2];
            int a_size = sizeof(char) * 2;
            for (int i = 0; i < face_num; ++i) {
                int pIndex0 = mfaces[i][0];
                int pIndex1 = mfaces[i][1];
                int pIndex2 = mfaces[i][2];
                float nx = mnorms[i].x;
                float ny = mnorms[i].y;
                float nz = mnorms[i].z;
                //
                fs.write((char*)(&nx), floatSize);
                fs.write((char*)(&ny), floatSize);
                fs.write((char*)(&nz), floatSize);
                // 
                float p0x = mpoints[pIndex0].x;
                float p0y = mpoints[pIndex0].y;
                float p0z = mpoints[pIndex0].z;
                fs.write((char*)(&p0x), floatSize);
                fs.write((char*)(&p0y), floatSize);
                fs.write((char*)(&p0z), floatSize);

                float p1x = mpoints[pIndex1].x;
                float p1y = mpoints[pIndex1].y;
                float p1z = mpoints[pIndex1].z;
                fs.write((char*)(&p1x), floatSize);
                fs.write((char*)(&p1y), floatSize);
                fs.write((char*)(&p1z), floatSize);

                float p2x = mpoints[pIndex2].x;
                float p2y = mpoints[pIndex2].y;
                float p2z = mpoints[pIndex2].z;
                fs.write((char*)(&p2x), floatSize);
                fs.write((char*)(&p2y), floatSize);
                fs.write((char*)(&p2z), floatSize);
                fs.write(a, a_size);
            }
            fs.close();
            return true;
        } else {
            //mpoints:
            //mfaces:
            //mnorms:
            if (face_num <= 1e6) {
                std::ofstream fs(std::string(filename) + "_ast.stl");
                if (!fs) { fs.close(); return false; }
                fs << "solid WRAP" << std::endl;
                for (int f = 0; f < face_num; ++f) {
                    fs << "facet normal ";
                    const auto& n = mnorms[f];
                    fs << (float)n.x << " " << (float)n.y << " " << (float)n.z << std::endl;
                    fs << "outer loop" << std::endl;
                    const auto& vertexs = mfaces[f];
                    for (const auto& inx : vertexs) {
                        fs << "vertex ";
                        const auto& p = mpoints[inx];
                        fs << (float)p.x << " " << (float)p.y << " " << (float)p.z << std::endl;
                    }
                    fs << "end loop" << std::endl;
                    fs << "end facet" << std::endl;
                }
                fs << "endsolid WRAP";
                fs.close();
                return true;
            }
            //
            const int block_size = 10000; // 
            const int num_threads = 8;     // 
            const int num_blocks = (face_num + block_size - 1) / block_size;
            // 
            std::ofstream of(std::string(filename) + "_ast.stl", std::ios::trunc);
            of << "solid ASCII_STL\n";
            of.close();
            // 
            std::vector<std::thread> threads(num_threads);
            std::mutex mutex;
            std::condition_variable cv;
            std::atomic<int> block_counter(0);
            const auto & normals = mnorms;
            const auto & points = mpoints;
            const auto & indexs = mfaces;
            std::ofstream out(std::string(filename) + "_ast.stl", std::ios::app);
            for (int i = 0; i < num_threads; ++i) {
                threads[i] = std::thread([&]() {
                    while (true) {
                        int block_index = block_counter.fetch_add(1);
                        if (block_index >= num_blocks) {
                            break;
                        }
                        int start = block_index * block_size;
                        int end = std::min(start + block_size, face_num);
                        std::string buffer;
                        for (int j = start; j < end; ++j) {
                            const auto& n = normals[j];
                            buffer += "facet normal " + std::to_string(n.x) + " " + std::to_string(n.y) + " " + std::to_string(n.z) + "\n";
                            buffer += "outer loop\n";
                            const auto& neighbors = indexs[j];
                            for (const auto& inx : neighbors) {
                                const auto& p = points[inx];
                                buffer += "vertex " + std::to_string(p.x) + " " + std::to_string(p.y) + " " + std::to_string(p.z) + "\n";
                            }
                            buffer += "endloop\n";
                            buffer += "endfacet\n";
                            if (j % 1000 == 0) {
                                std::unique_lock<std::mutex> lock(mutex);
                                out << buffer;
                                buffer.clear();
                                cv.notify_all();
                            }
                        }
                        std::unique_lock<std::mutex> lock(mutex);
                        out << buffer;
                        cv.notify_all();
                    }
                });
            }
            // 
            for (auto& thread : threads) {
                thread.join();
            }
            // 
            out << "endsolid ASCII_STL\n";
            out.close();
            return true;
        }
    }
    CMesh::CMesh(const trimesh::TriMesh* mesh)
    {
        trimesh::TriMesh trimesh = *mesh;
        mfaces.swap(trimesh.faces);
        mpoints.swap(trimesh.vertices);
        std::swap(mbox, trimesh.bbox);
    }
    trimesh::TriMesh CMesh::GetTriMesh() const
    {
        trimesh::TriMesh mesh;
        mesh.vertices = mpoints;
        mesh.faces = mfaces;
        mesh.bbox = mbox;
        return mesh;
    }
    trimesh::TriMesh CMesh::ConvertToTriMesh()
    {
        trimesh::TriMesh mesh;
        mesh.faces.swap(mfaces);
        mesh.vertices.swap(mpoints);
        std::swap(mesh.bbox, mbox);
        return mesh;
    }
    void CMesh::Merge(const CMesh& mesh, bool bDuplicate)
    {
        int pn = mpoints.size();
        int fn = mfaces.size();
        const auto& points = mesh.mpoints;
        const auto& faces = mesh.mfaces;
        mfaces.reserve(fn + faces.size());
        mpoints.reserve(pn + points.size());
        mpoints.insert(mpoints.end(), points.begin(), points.end());
        for (const auto& f : faces) {
            mfaces.emplace_back(trimesh::ivec3(pn + f[0], pn + f[1], pn + f[2]));
        }
        if (bDuplicate) {
            DuplicateSTL();
        }
        mbox.valid = false;
    }
    void CMesh::JoinTogether(const std::vector<CMesh>& meshes, bool bDuplicate)
    {
        int pn = mpoints.size();
        int fn = mfaces.size();
        for (int i = 0; i < meshes.size(); ++i) {
            const auto& mesh = meshes[i];
            const auto& points = mesh.mpoints;
            const auto& faces = mesh.mfaces;
            mpoints.insert(mpoints.end(), points.begin(), points.end());
            for (const auto& f : faces) {
                mfaces.emplace_back(trimesh::ivec3(pn + f[0], pn + f[1], pn + f[2]));
            }
            fn += faces.size();
            pn += points.size();
        }
        if (bDuplicate) {
            DuplicateSTL();
        }
        mbox.valid = false;
    }
    void CMesh::Clone(const CMesh& mesh)
    {
        *this = mesh;
    }
    void CMesh::MiniCopy(const CMesh& mesh)
    {
        mbox = std::move(mesh.mbox);
        mfaces = std::move(mesh.mfaces);
        mpoints = std::move(mesh.mpoints);
    }
    void CMesh::Translate(const PPoint& trans)
    {
        for (auto& pt : mpoints) {
            pt += trans;
        }
        mbox.min += trans;
        mbox.max += trans;
    }
    void CMesh::Rotate(const trimesh::fxform& mat)
    {
        for (auto& pt : mpoints) {
            pt = mat * pt;
        }
        mbox.valid = false;
    }
    void CMesh::Rotate(const PPoint& axis, const double& angle)
    {
        trimesh::fxform&& mat = trimesh::fxform::rot(angle, axis);
        Rotate(mat);
    }
    void CMesh::Rotate(const PPoint& dir1, const PPoint& dir2)
    {
        trimesh::fxform&& mat = trimesh::fxform::rot_into(dir1, dir2);
        Rotate(mat);
    }
    void CMesh::BuildFromBox(const BBox& box)
    {
        const trimesh::point& min = box.min;
        const trimesh::point& max = box.max;
        const auto& diag = max - min;
        AddPoint(min + trimesh::point(diag.x, 0, 0));
        AddPoint(min + trimesh::point(diag.x, diag.y, 0));
        AddPoint(min + trimesh::point(diag.x, diag.y, diag.z));
        AddPoint(min + trimesh::point(diag.x, 0, diag.z));
        AddPoint(min + trimesh::point(0, 0, 0));
        AddPoint(min + trimesh::point(0, diag.y, 0));
        AddPoint(min + trimesh::point(0, diag.y, diag.z));
        AddPoint(min + trimesh::point(0, 0, diag.z));
        AddFace(0, 1, 2);
        AddFace(0, 2, 3);
        AddFace(1, 5, 6);
        AddFace(1, 6, 2);
        AddFace(3, 2, 6);
        AddFace(3, 6, 7);
        AddFace(4, 0, 3);
        AddFace(4, 3, 7);
        AddFace(4, 5, 1);
        AddFace(4, 1, 0);
        AddFace(5, 4, 7);
        AddFace(5, 7, 6);
    }
    void CMesh::GenerateBoundBox()
    {
        if (!mbox.valid) {
            constexpr float max = std::numeric_limits<float>::max();
            constexpr float min = std::numeric_limits<float>::lowest();
            trimesh::vec3 a(max, max, max), b(min, min, min);
            for (const auto& p : mpoints) {
                if (a.x > p.x) a.x = p.x;
                if (a.y > p.y) a.y = p.y;
                if (a.z > p.z) a.z = p.z;
                if (b.x < p.x) b.x = p.x;
                if (b.y < p.y) b.y = p.y;
                if (b.z < p.z) b.z = p.z;
            }
            mbox = trimesh::box3(a, b);
            mbox.valid = true;
        }
    }
    int CMesh::AddPoint(const PPoint& p)
    {
        mbox += p;
        mpoints.emplace_back(std::move(p));
        return mpoints.size() - 1;
    }
    int CMesh::AddFace(int i0, int i1, int i2)
    {
        FFace f(i0, i1, i2);
        mfaces.emplace_back(std::move(f));
        return mfaces.size() - 1;
    }

    int CMesh::EdgeOppositePoint(int e, int f) const
    {
        const auto& neighbor = mfaces[f];
        const auto& edge = medges[e];
        for (int i = 0; i < 3; ++i) {
            if ((neighbor[i] != edge.a) && (neighbor[i] != edge.b)) {
                return neighbor[i];
            }
        }
        return -1;
    }

    int CMesh::PointOppositeEdge(int v, int f) const
    {
        const auto& neighbor = mfaceEdges[f];
        for (int i = 0; i < neighbor.size(); ++i) {
            if ((medges[neighbor[i]].a != v) && (medges[neighbor[i]].b != v)) {
                return neighbor[i];
            }
        }
        return -1;
    }

    float CMesh::GetVolume()const
    {
        float volume = 0.0f;
        for (const auto& f : mfaces) {
            const auto& a = mpoints[f[0]];
            const auto& b = mpoints[f[1]];
            const auto& c = mpoints[f[2]];
            volume += ((a TRICROSS b) DOT c);
        }
        return volume / 6.0f;
    }

    void CMesh::GenerateFaceAreas(bool calculateAgain)
    {
        if (mareas.empty() || calculateAgain) {
            const int facenums = mfaces.size();
            mareas.resize(facenums);
            std::transform(mfaces.begin(), mfaces.end(), mareas.begin(), [&](const FFace & f) {
                const auto& p0 = mpoints[f[0]];
                const auto& p1 = mpoints[f[1]];
                const auto& p2 = mpoints[f[2]];
                return trimesh::len((p2 - p1).cross(p0 - p1)) / 2.0;
            });
        }
    }

    void CMesh::GenerateFaceNormals(bool Normalized, bool calculateArea)
    {
        const int facenums = mfaces.size();
        ::std::vector<PPoint>().swap(mnorms);
        mnorms.reserve(facenums);
        for (const auto& f : mfaces) {
            const auto& p0 = mpoints[f[0]];
            const auto& p1 = mpoints[f[1]];
            const auto& p2 = mpoints[f[2]];
            const auto& n = (p2 - p1).cross(p0 - p1);
            mnorms.emplace_back(std::move(n));
        }
        //std::transform
        if (calculateArea) {
            mareas.reserve(facenums);
            for (const trimesh::vec3& n : mnorms) {
                mareas.emplace_back(trimesh::len(n) / 2.0);
            }
        }
        if (Normalized) {
            for (auto& n : mnorms) {
                trimesh::normalize(n);
            }
        }
    }

    std::vector<std::vector<int>> CMesh::GenerateFaceNeighborFaces()
    {
        const int facenums = mfaces.size();
        std::vector<std::vector<int>> vertexFaceMap;
        vertexFaceMap.resize(mpoints.size());
        for (int f = 0; f < facenums; ++f) {
            const auto& fs = std::move(mfaces[f]);
            vertexFaceMap[fs[0]].emplace_back(f);
            vertexFaceMap[fs[1]].emplace_back(f);
            vertexFaceMap[fs[2]].emplace_back(f);
        }
        std::vector<std::vector<int>>neighbors;
        neighbors.resize(facenums);
        //#pragma omp parallel for
        for (int f = 0; f < facenums; ++f) {
            std::vector<int> neighbor;
            neighbor.reserve(20);
            const auto& vertexs = std::move(mfaces[f]);
            for (const auto& v : vertexs) {
                const auto& fs = std::move(vertexFaceMap[v]);
                //neighbor.reserve(neighbor.size() + std::distance(fs.begin(), fs.end()));
                for (const auto& fa : fs) {
                    if (fa != f) {
                        neighbor.emplace_back(fa);
                    }
                }
            }
            std::sort(neighbor.begin(), neighbor.end());
            auto last = std::unique(neighbor.begin(), neighbor.end());
            neighbor.erase(last, neighbor.end());
            neighbors[f] = std::move(neighbor);
        }
        return neighbors;
    }

    void CMesh::GenerateFaceEdgeAdjacency(bool bGenerateEdgeFaceAdjacency, bool bGenerateEgdeLength)
    {
        const size_t facenums = mfaces.size();
        medges.reserve(3 * facenums);
        std::vector<EdgeToFace> edgesmap;
        edgesmap.reserve(3 * facenums);
        for (size_t facet_idx = 0; facet_idx < facenums; ++facet_idx) {
            for (size_t j = 0; j < 3; ++j) {
                edgesmap.push_back({});
                EdgeToFace& e2f = edgesmap.back();
                e2f.vertex_low = mfaces[facet_idx][j];
                e2f.vertex_high = mfaces[facet_idx][(j + 1) % 3];
                e2f.at_face = facet_idx;
                e2f.which_edge = j + 1;
                if (e2f.vertex_low > e2f.vertex_high) {
                    std::swap(e2f.vertex_low, e2f.vertex_high);
                    e2f.which_edge = -e2f.which_edge;
                }
            }
        }
        std::sort(edgesmap.begin(), edgesmap.end());
        std::vector<std::vector<int>>fedges(facenums, std::vector<int>(3, -1));
        int num_edges = 0;
        for (size_t i = 0; i < edgesmap.size(); ++i) {
            EdgeToFace& edge_i = edgesmap[i];
            if (edge_i.at_face == -1)
                continue;
            for (size_t j = i + 1; j < edgesmap.size() && edge_i == edgesmap[j]; ++j) {
                EdgeToFace& edge_j = edgesmap[j];
                fedges[edge_j.at_face][std::fabs(edge_j.which_edge) - 1] = num_edges;
                // Mark the edge as connected.
                edge_j.at_face = -1;
            }
            fedges[edge_i.at_face][std::fabs(edge_i.which_edge) - 1] = num_edges;
            medges.emplace_back(EEdge(edge_i.vertex_low, edge_i.vertex_high));
            ++num_edges;
        }
        if (bGenerateEdgeFaceAdjacency) {
            std::vector<std::vector<int>> efaces(num_edges);
            for (int i = 0; i < num_edges; ++i) {
                efaces[i].reserve(2);
            }
            for (size_t i = 0; i < facenums; ++i) {
                const auto& es = fedges[i];
                for (const auto& e : es) {
                    efaces[e].emplace_back(i);
                }
            }
            medgeFaces.swap(efaces);
        }
        mfaceEdges.swap(fedges);
        if (bGenerateEgdeLength) {
            medgeLengths.reserve(num_edges);
            for (const auto& e : medges) {
                const auto& d = trimesh::len(mpoints[e.a] - mpoints[e.b]);
                medgeLengths.emplace_back(d);
            }
        }
    }

    void CMesh::GenerateFaceEdgeAdjacency2(bool bGenerateEdgeFaceAdjacency, bool bGenerateEgdeLength)
    {
        //
        std::vector<FFace> indexs(mfaces.begin(), mfaces.end());
        const size_t facenums = indexs.size();
        struct EEdgeHash {
            size_t operator()(const EEdge& e) const
            {
                return int((e.a * 99989)) ^ (int(e.b * 99991) << 2);
            }
        };
        struct EEdgeEqual {
            bool operator()(const EEdge& e1, const EEdge& e2) const
            {
                return e1.a == e2.a && e1.b == e2.b;
            }
        };
        std::unordered_map<EEdge, size_t, EEdgeHash, EEdgeEqual> edgeIndexMap;
        medges.reserve(3 * facenums);
        edgeIndexMap.reserve(3 * facenums);
        mfaceEdges.resize(facenums);
        for (size_t i = 0; i < facenums; ++i) {
            std::vector<int>elist;
            elist.reserve(3);
            auto& vertexs = indexs[i];
            std::sort(vertexs.begin(), vertexs.end());
            for (size_t j = 0; j < 2; ++j) {
                EEdge e(vertexs[j], vertexs[j + 1]);
                const auto & itr = edgeIndexMap.find(e);
                if (itr != edgeIndexMap.end()) {
                    elist.emplace_back(itr->second);
                }
                if (itr == edgeIndexMap.end()) {
                    const size_t edgeIndex = edgeIndexMap.size();
                    edgeIndexMap.emplace(std::move(e), edgeIndex);
                    medges.emplace_back(std::move(e));
                    elist.emplace_back(edgeIndex);
                }
            }
            // 
            EEdge e(vertexs[0], vertexs[2]);
            const auto& itr = edgeIndexMap.find(e);
            if (itr != edgeIndexMap.end()) {
                elist.emplace_back(itr->second);
            }
            if (itr == edgeIndexMap.end()) {
                const size_t edgeIndex = edgeIndexMap.size();
                edgeIndexMap.emplace(std::move(e), edgeIndex);
                medges.emplace_back(std::move(e));
                elist.emplace_back(edgeIndex);
            }
            mfaceEdges[i].swap(elist);
        }
        if (bGenerateEdgeFaceAdjacency) {
            const size_t edgenums = medges.size();
            medgeFaces.resize(edgenums);
            for (size_t i = 0; i < edgenums; ++i) {
                medgeFaces[i].reserve(2);
            }
            for (size_t i = 0; i < facenums; ++i) {
                const auto& es = std::move(mfaceEdges[i]);
                for (int j = 0; j < 3; ++j) {
                    medgeFaces[es[j]].emplace_back(i);
                }
            }
        }
        if (bGenerateEgdeLength) {
            const size_t edgenums = medges.size();
            medgeLengths.reserve(edgenums);
            for (const auto& e : medges) {
                const auto& d = trimesh::len(mpoints[e.a] - mpoints[e.b]);
                medgeLengths.emplace_back(d);
            }
        }
    }

    std::vector<int> CMesh::SelectLargetPlanar(float threshold)
    {
        GenerateFaceNormals(true, true);
        const size_t facenums = mfaces.size();
        std::vector<int> sequence(facenums);
        for (int i = 0; i < facenums; ++i) {
            sequence[i] = i;
        }
        trimesh::TriMesh trimesh = GetTriMesh();
        trimesh.need_across_edge();
        mffaces = trimesh.across_edge;
        std::vector<int> masks(facenums, 1);
        std::vector<std::vector<int>> selectFaces;
        selectFaces.reserve(facenums);
        for (const auto& f : sequence) {
            if (masks[f]) {
                const auto& nf = mnorms[f];
                std::vector<int>currentFaces;
                std::queue<int>currentQueue;
                currentQueue.emplace(f);
                currentFaces.emplace_back(f);
                masks[f] = false;
                while (!currentQueue.empty()) {
                    auto& fr = currentQueue.front();
                    currentQueue.pop();
                    const FFace& neighbor = mffaces[fr];
                    for (const auto& fa : neighbor) {
                        if (fa < 0) continue;
                        if (masks[fa]) {
                            const auto& na = mnorms[fa];
                            const auto& nr = mnorms[fr];
                            if ((nr DOT na) > threshold && (nf DOT na) > threshold) {
                                currentQueue.emplace(fa);
                                currentFaces.emplace_back(fa);
                                masks[fa] = 0;
                            }
                        }
                    }
                }
                selectFaces.emplace_back(std::move(currentFaces));
            }
        }

        //
        double maxArea = 0.0f;
        std::vector<int> resultFaces;
        for (auto& fs : selectFaces) {
            double currentArea = 0.0;
            for (const auto& f : fs) {
                currentArea += mareas[f];
            }
            if (currentArea > maxArea) {
                maxArea = currentArea;
                resultFaces.swap(fs);
            }
        }
        return resultFaces;
    }

    void CMesh::FlatBottomSurface(std::vector<int>* bottomfaces)
    {
        GenerateBoundBox();
        const auto& minz = mbox.min.z;
        for (const auto& f : *bottomfaces) {
            const auto& neighbor = mfaces[f];
            for (int j = 0; j < 3; ++j) {
                mpoints[neighbor[j]].z = minz;
            }
        }
    }

    CMesh::PPoint CMesh::FindBottomDirection(std::vector<int>* bottomfaces, float threshold)
    {
        *bottomfaces = SelectLargetPlanar(threshold);
        PPoint normal(0, 0, 0);
        for (const auto& f : *bottomfaces) {
            normal += mnorms[f] * mareas[f];
        }
        return trimesh::normalized(normal);
    }

    void CMesh::DeleteFaces(std::vector<int>& faceIndexs, bool bKeepPoints)
    {
        std::sort(faceIndexs.begin(), faceIndexs.end());
        std::vector<int> sequence(mfaces.size());
        for (int i = 0; i < mfaces.size(); ++i) {
            sequence[i] = i;
        }
        std::vector<int> result;
        std::set_difference(sequence.begin(), sequence.end(), faceIndexs.begin(), faceIndexs.end(), std::back_inserter(result));
        const size_t facenums = result.size();
        std::vector<FFace> otherFaces;
        otherFaces.resize(facenums);
        for (int i = 0; i < facenums; ++i) {
            otherFaces[i] = std::move(mfaces[result[i]]);
        }
        if (bKeepPoints) {
            mfaces = std::move(otherFaces);
            GenerateFaceNormals();
            return;
        }
        struct PPointHash {
            size_t operator()(const PPoint& p) const
            {
                return (int(p.x * 99971)) ^ (int(p.y * 99989) << 2) ^ (int(p.z * 99991) << 3);
            }
        };
        struct PPointEqual {
            bool operator()(const PPoint& p1, const PPoint& p2) const
            {
                auto isEqual = [&](float a, float b, float eps = EPS) {
                    return std::fabs(a - b) < eps;
                };
                return isEqual(p1.x, p2.x) && isEqual(p1.y, p2.y) && isEqual(p1.z, p2.z);
            }
        };
        std::unordered_map<PPoint, int, PPointHash, PPointEqual> pointMap;
        pointMap.rehash(2 * facenums);
        std::vector<PPoint> newPoints;
        std::vector<FFace>newFaces;
        newPoints.reserve(3 * facenums);
        newFaces.reserve(facenums);
        for (const auto& vs : otherFaces) {
            std::vector<int> f;
            f.reserve(3);
            for (const auto& v : vs) {
                const auto& p = mpoints[v];
                const auto& iter = pointMap.find(p);
                if (iter != pointMap.end()) {
                    f.emplace_back(iter->second);
                } else {
                    const int newIndex = pointMap.size();
                    newPoints.emplace_back(std::move(p));
                    pointMap.emplace(std::move(p), newIndex);
                    f.emplace_back(newIndex);
                }
            }
            newFaces.emplace_back(std::move(f));
        }
        mpoints.swap(newPoints);
        mfaces.swap(newFaces);
        GenerateFaceNormals();
        mbox.valid = false;
        return;
    }

    void CMesh::SelectIndividualEdges(std::vector<int>& edgeIndexs, bool bCounterClockWise)
    {
        std::vector<int>().swap(edgeIndexs);
        if (medgeFaces.empty()) {
            GenerateFaceEdgeAdjacency2(true);
        }
        const size_t edgenums = medgeFaces.size();
        for (int i = 0; i < edgenums; ++i) {
            const auto& neighbor = medgeFaces[i];
            if (neighbor.size() == 1) {
                edgeIndexs.emplace_back(i);
            }
        }
        if (bCounterClockWise) {
            for (auto& e : edgeIndexs) {
                const auto& neighbor = medgeFaces[e];
                const int f = neighbor.front();
                std::queue<int> vertexs;
                for (const auto& v : mfaces[f]) {
                    vertexs.emplace(v);
                }
                int v = EdgeOppositePoint(e, f);
                for (int i = 0; i < 3; ++i) {
                    int fr = vertexs.front();
                    if (fr == v) break;
                    vertexs.emplace(fr);
                    vertexs.pop();
                }
                vertexs.pop();
                const int a = medges[e].a;
                const int vt = vertexs.front();
                if (vt == a) {
                    medges[e].a = medges[e].a + medges[e].b;
                    medges[e].b = medges[e].a - medges[e].b;
                    medges[e].a = medges[e].a - medges[e].b;
                }
            }
        }
    }

    bool CMesh::GetSequentialPoints(std::vector<int>& edgeIndexs, std::vector<std::vector<int>>& sequentials)
    {
        bool result = true;
        const size_t nums = edgeIndexs.size();
        std::vector<bool> masks(medges.size(), false);
        // all sequential edges are counterclockwise.
        if (true) {
            for (auto& e : edgeIndexs) {
                const auto& neighbor = medgeFaces[e];
                const int f = neighbor.front();
                std::queue<int> vertexs;
                for (const auto& v : mfaces[f]) {
                    vertexs.emplace(v);
                }
                int v = EdgeOppositePoint(e, f);
                for (int i = 0; i < 3; ++i) {
                    int fr = vertexs.front();
                    if (fr == v) break;
                    vertexs.emplace(fr);
                    vertexs.pop();
                }
                vertexs.pop();
                const int a = medges[e].a;
                const int vt = vertexs.front();
                if (vt == a) {
                    medges[e].a = medges[e].a + medges[e].b;
                    medges[e].b = medges[e].a - medges[e].b;
                    medges[e].a = medges[e].a - medges[e].b;
                    masks[e] = true;
                }
            }
        }
        std::queue<int> Queues;
        std::vector<int> pointStarts;
        std::multimap<int, int> pEdgeStartMap;
        std::multimap<int, int> pEdgeEndMap;
        pointStarts.reserve(nums);
        for (int i = 0; i < nums; ++i) {
            const auto& einx = edgeIndexs[i];
            pointStarts.emplace_back(medges[einx].a);
            pEdgeStartMap.emplace(medges[einx].a, einx);
            pEdgeEndMap.emplace(medges[einx].b, einx);
            Queues.emplace(einx);
        }
        //first. mark all possible cross knot point at next edge index.
        std::unordered_set<int> uniqueSets;
        std::unordered_set<int> crossKnots;
        for (const auto& v : pointStarts) {
            if (uniqueSets.count(v) == 0) {
                uniqueSets.insert(v);
            } else {
                crossKnots.insert(v);
            }
        }
        int knotnums = crossKnots.size();
        std::queue<int> crossPoints;
        //several sequentials about edges.
        std::vector<std::vector<int>> edgeRings;
        //second. begin to deal with crossKnots on sequential edges.
        if (!crossKnots.empty()) {
            std::vector<bool> knotMarks(mpoints.size(), false);
            std::queue<std::queue<int>> starts, ends;
            for (const auto& v : uniqueSets) {
                if (crossKnots.count(v)) {
                    crossPoints.emplace(v);
                    knotMarks[v] = true;
                    std::queue<int> start, end;
                    auto range1 = pEdgeStartMap.equal_range(v);
                    for (auto itr = range1.first; itr != range1.second; ++itr) {
                        start.emplace(itr->second);
                    }
                    auto range2 = pEdgeEndMap.equal_range(v);
                    for (auto itr = range2.first; itr != range2.second; ++itr) {
                        end.emplace(itr->second);
                    }
                    starts.emplace(start);
                    ends.emplace(end);
                }
            }
            //thrid. truncate sequential edges according to crossKnots.
            std::queue<std::vector<int>> ringQueues;
            while (!Queues.empty()) {
                if (crossPoints.empty()) break;
                int currentKnot = crossPoints.front();
                while (!Queues.empty()) {
                    if (!starts.front().empty() && !ends.front().empty()) {
                        for (int i = 0; i < Queues.size(); ++i) {
                            const int ef = Queues.front();
                            if (medges[ef].a == currentKnot) {
                                break;
                            }
                            Queues.emplace(ef);
                            Queues.pop();
                        }
                        const int fe = Queues.front();
                        std::vector<int> current;
                        current.emplace_back(fe);
                        Queues.pop();
                        int times = 0;
                        int queuesize = Queues.size();
                        while (!Queues.empty()) {
                            const int back = current.back();
                            if (knotMarks[medges[back].b]) {
                                break;
                            }
                            const int ef = Queues.front();
                            if (medges[ef].a == medges[back].b) {
                                current.emplace_back(ef);
                                Queues.pop();
                                times = 0;
                            } else {
                                Queues.emplace(ef);
                                Queues.pop();
                                ++times;
                            }
                            if (times > queuesize) {
                                result = false;
                                break;
                            }
                        }
                        ringQueues.emplace(current);
                        starts.front().pop();
                        ends.front().pop();
                    } else {
                        starts.pop();
                        ends.pop();
                        break;
                    }                   
                }
                crossPoints.pop();
            }
            int circles = 0;
            int ringSize = ringQueues.size();
            //forth. priority deal with closed edge sequences.
            while (circles < ringSize) {
                const auto& ring = ringQueues.front();
                if (medges[ring.front()].a == medges[ring.back()].b) {
                    edgeRings.emplace_back(ring);
                    ringQueues.pop();
                } else {
                    ringQueues.emplace(ring);
                    ringQueues.pop();
                }
                ++circles;
            }
            //fifth. connect with each other for unenclosed series by sequence.
            while (!ringQueues.empty()) {
                std::vector<std::vector<int>> current;
                const auto& ring = ringQueues.front();
                ringSize = ringQueues.size();
                current.reserve(ringSize);
                current.emplace_back(ring);
                ringQueues.pop();
                while (!ringQueues.empty()) {
                    int times = 0;
                    int count = ringQueues.size();
                    std::map<int, std::vector<int>> connects;
                    const int fr = current.front().front();
                    const int ba = current.back().back();
                    if (medges[fr].a == medges[ba].b) break;
                    while (times < ringQueues.size()) {
                        const auto& fr = ringQueues.front();
                        const int ef = fr.front();
                        if (medges[ef].a == medges[ba].b) {
                            connects.emplace(times, fr);
                        }
                        ringQueues.emplace(fr);
                        ringQueues.pop();
                        ++times;
                    }
                    int lowest = -1;
                    float minAlpha = 2.0 * M_PI;
                    const int f1 = medgeFaces[ba].front();
                    const FFace fn1 = mfaces[f1];
                    trimesh::vec3 norm1 = trimesh::trinorm(mpoints[fn1[0]], mpoints[fn1[1]], mpoints[fn1[2]]);
                    float area1 = trimesh::len(norm1);
                    trimesh::normalize(norm1);
                    const auto& a = mpoints[medges[ba].a];
                    const auto& b = mpoints[medges[ba].b];
                    trimesh::vec3 dir1 = trimesh::normalized(a - b);
                    constexpr float minArea = std::numeric_limits<float>::max();
                    for (auto itr = connects.begin(); itr != connects.end(); ++itr) {
                        const auto& tmp = itr->second;
                        const int fr = tmp.front();
                        const int f2 = medgeFaces[fr].front();
                        const FFace fn2 = mfaces[f2];
                        trimesh::vec3 norm2 = trimesh::trinorm(mpoints[fn2[0]], mpoints[fn2[1]], mpoints[fn2[2]]);
                        const auto& c = mpoints[medges[fr].a];
                        const auto& d = mpoints[medges[fr].b];
                        const auto& dir2 = trimesh::normalized(d - c);
                        float alpha = std::acos(dir2 DOT dir1);
                        const auto& normal = dir2 TRICROSS dir1;
                        float area2 = trimesh::len(norm2);
                        trimesh::normalize(norm2);
                        float sumArea = area1 + area2;
                        trimesh::vec3 referDir = (area1 * norm1 + area2 * norm2) / sumArea;
                        float zvalue = normal DOT referDir;
                        if (zvalue < 0) {
                            alpha = 2 * M_PI - alpha;
                        }
                        if (alpha <= minAlpha) {
                            minAlpha = alpha;
                            lowest = itr->first;
                        }
                    }
                    times = 0;
                    while (times < lowest) {
                        const auto& fr = ringQueues.front();
                        ringQueues.emplace(fr);
                        ringQueues.pop();
                        ++times;
                    }
                    const auto ef = ringQueues.front();
                    current.emplace_back(ef);
                    ringQueues.pop();
                }
                /*while (!ringQueues.empty()) {
                    const auto front = current.front().front();
                    const auto back = current.back().back();
                    const auto ef = ringQueues.front();
                    const auto ef_front = ef.front();
                    const auto ef_back = ef.back();
                    if (medges[ef_back].b == medges[front].a) {
                        current.insert(current.begin(), ef);
                        ringQueues.pop();
                        times = 0;
                    } else if (medges[ef_front].a == medges[back].b) {
                        current.emplace_back(ef);
                        ringQueues.pop();
                        times = 0;
                    } else {
                        ringQueues.pop();
                        ringQueues.push(ef);
                        ++times;
                    }
                    if (medges[front].a == medges[back].b) {
                        break;
                    }
                    if (times > count) {
                        break;
                    }
                }*/
                int totalSize = 0;
                std::vector<int> sequence;
                for (const auto&  ring: current) {
                    totalSize += ring.size();
                }
                sequence.reserve(totalSize);
                for (const auto& ring : current) {
                    sequence.insert(sequence.end(), ring.begin(), ring.end());
                }
                if (sequence.size() < 3) result = false;
                edgeRings.emplace_back(sequence);
            }
        }
        // other. there are no crossKnots on sequential edges.
        while (!Queues.empty()) {
            std::vector<int> current;
            current.reserve(nums);
            const auto& e = Queues.front();
            Queues.pop();
            current.emplace_back(e);
            int count = Queues.size();
            int times = 0;
            while (!Queues.empty()) {
                const auto& front = current.front();
                const auto& back = current.back();
                const auto& ef = Queues.front();
                if (medges[ef].b == medges[front].a) {
                    current.insert(current.begin(), ef);
                    Queues.pop();
                    times = 0;
                } else if (medges[ef].a == medges[back].b) {
                    current.emplace_back(ef);
                    Queues.pop();
                    times = 0;
                } else {
                    Queues.pop();
                    Queues.push(ef);
                    ++times;
                }
                if (medges[front].a == medges[back].b) {
                    break;
                }
                if (times > count) {
                    result = false;
                    break;
                }
            }
            edgeRings.emplace_back(current);
        }
        sequentials.reserve(edgeRings.size());
        //restore some edges amended before.
        for (auto& es : edgeRings) {
            std::vector<int> pointList;
            pointList.reserve(es.size());
            const auto& fr = medges[es.front()].a;
            const auto& ba = medges[es.back()].b;
            if (fr != ba) {
                pointList.emplace_back(fr);
            }
            for (auto& e : es) {
                pointList.emplace_back(medges[e].b);
                if (masks[e]) {
                    medges[e].a = medges[e].a + medges[e].b;
                    medges[e].b = medges[e].a - medges[e].b;
                    medges[e].a = medges[e].a - medges[e].b;
                }
            }
            if (pointList.size() < 3) result = false;
            sequentials.emplace_back(pointList);
        }
        return result;
    }

    CMesh CMesh::SavePointsToMesh(std::vector<int>& pointIndexs, double radius, size_t nrows, size_t ncolumns)
    {
        CMesh pointMesh;
        const size_t spherenums = pointIndexs.size();
        const size_t nums = nrows * ncolumns + 2;
        const size_t pointnums = nums * spherenums;
        const size_t facenums = 2 * (nums - 2) * spherenums;
        auto& points = pointMesh.mpoints;
        points.reserve(pointnums);
        for (int k = 0; k < spherenums; ++k) {
            const auto& p = mpoints[pointIndexs[k]];
            points.emplace_back(p.x, p.y, p.z + radius);
            for (int i = 0; i < nrows; ++i) {
                const auto& phi = M_PI * (i + 1.0) / double(nrows + 1.0);
                const auto & z = radius * std::cos(phi);
                const auto & r = radius * std::sin(phi);
                for (int j = 0; j < ncolumns; ++j) {
                    const auto& theta = 2.0 * M_PI * j / ncolumns;
                    const auto& x = r * std::cos(theta);
                    const auto& y = r * std::sin(theta);
                    points.emplace_back(p.x + x, p.y + y, p.z + z);
                }
            }
            points.emplace_back(p.x, p.y, p.z - radius);
            const auto & maxInx = points.size() - 1;
            const auto & v0 = k * nums;
            //
            for (size_t i = 0; i < ncolumns; ++i) {
                const auto& i0 = i + 1 + v0;
                const auto& i1 = (i + 1) % ncolumns + 1 + v0;
                pointMesh.AddFace(v0, i0, i1);
                const auto & j0 = i0 + (nrows - 1) * ncolumns;
                const auto & j1 = i1 + (nrows - 1) * ncolumns;
                pointMesh.AddFace(j1, j0, maxInx);
            }
            //
            for (size_t i = 0; i < nrows - 1; ++i) {
                const auto& j0 = i * ncolumns + 1 + v0;
                const auto& j1 = (i + 1) * ncolumns + 1 + v0;
                for (size_t j = 0; j < ncolumns; ++j) {
                    const auto& i0 = j0 + j;
                    const auto& i1 = j0 + (j + 1) % ncolumns;
                    const auto & i2 = j1 + j;
                    const auto & i3 = j1 + (j + 1) % ncolumns;
                    pointMesh.AddFace(i2, i1, i0);
                    pointMesh.AddFace(i2, i3, i1);
                }
            }
        }
        return pointMesh;
    }
    CMesh CMesh::SaveEdgesToMesh(std::vector<int>& edgeIndexs, double r, size_t nslices)
    {
        CMesh edgeMesh;
        const size_t nums = edgeIndexs.size();
        auto& points = edgeMesh.mpoints;
        points.reserve(2 * nums * nslices);
        double delta = 2.0 * M_PI / nslices;
        trimesh::vec3 z(0, 0, 1);
        for (size_t i = 0; i < nums; ++i) {
            const auto& a = mpoints[medges[edgeIndexs[i]].a];
            const auto& b = mpoints[medges[edgeIndexs[i]].b];
            const auto& n = trimesh::normalized(b - a);
            auto&& x = std::move(z.cross(n));
            if (trimesh::len(x) < EPS) {
                x = PPoint(1, 0, 0);
            }
            trimesh::normalize(x);
            const auto& y = n.cross(x);
            for (int j = 0; j < nslices; ++j) {
                const auto& theta = delta * j;
                const auto& p = b + x * r * std::cos(theta) + y * r * std::sin(theta);
                points.emplace_back(p);
            }
            for (int j = 0; j < nslices; ++j) {
                const auto& theta = delta * j;
                const auto& p = a + x * r * std::cos(theta) + y * r * std::sin(theta);
                points.emplace_back(p);
            }
        }
        for (size_t i = 0; i < nums; ++i) {
            for (size_t j = 0; j < nslices; ++j) {
                const auto& i0 = j + 2 * nslices * i;
                const auto& i1 = (j + 1) % nslices + 2 * nslices * i;
                const auto & j0 = i0 + nslices;
                const auto & j1 = i1 + nslices;
                edgeMesh.AddFace(j0, i1, i0);
                edgeMesh.AddFace(j0, j1, i1);
            }
        }
        return edgeMesh;
    }
    CMesh CMesh::SaveFacesToMesh(std::vector<int>& faceIndexs)
    {
        CMesh faceMesh;
        std::vector<int> selectFaces;
        const int facenums = mfaces.size();
        for (const auto& f : faceIndexs) {
            if (f < facenums) {
                selectFaces.emplace_back(f);
            }
        }
        faceIndexs.swap(selectFaces);
        const auto& points = mpoints;
        const auto& indexs = mfaces;
        for (const auto& f : faceIndexs) {
            const auto& vertexs = indexs[f];
            int v0 = faceMesh.AddPoint(points[vertexs[0]]);
            int v1 = faceMesh.AddPoint(points[vertexs[1]]);
            int v2 = faceMesh.AddPoint(points[vertexs[2]]);
            faceMesh.AddFace(v0, v1, v2);
        }
        return faceMesh;
    }
}

