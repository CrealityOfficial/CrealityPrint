#include "fillhoneycombs.h"

#include "topomesh/interface/utils.h"
#include "topomesh/interface/subdivision.h"
#include "topomesh/interface/hex.h"
#include "volumeMesh.h"

#include "internal/data/mmesht.h"
#include "internal/alg/letter.h"
#include "internal/alg/earclipping.h"
#include "internal/alg/solidtriangle.h"
#include "internal/polygon/comb.h"
#include "internal/data/const.h"

#include "trimesh2/TriMesh_algo.h"
#include "msbase/mesh/dumplicate.h"
#include "msbase/mesh/get.h"
#include <random>


namespace topomesh {
    void GenerateTriPolygonsHexagons(const TriPolygons&polys, const HoneyCombParam& honeyparams, honeyLetterOpt& letterOpts, HoneyCombDebugger* debugger) {
        if (polys.empty()) return;
        if (debugger) {
            //显示底面边界轮廓多边形
            debugger->onGenerateInfillPolygons(polys);
        }
        //第0步，底面边界轮廓抽壳
        const double resolution = honeyparams.resolution;
        const double side = honeyparams.honeyCombRadius;
        const double thickness = honeyparams.shellThickness;
        const double radius = side + honeyparams.nestWidth / SQRT3;
        Polygons bpolygons; ///<底面边界轮廓多边形
        Polygons mpolygons; ///<底面边界抽壳多边形
        {
            for (const auto& poly : polys) {
                ClipperLib::Path path;
                path.reserve(poly.size());
                for (const auto& p : poly) {
                    const auto& x = std::round(p.x / resolution);
                    const auto& y = std::round(p.y / resolution);
                    path.emplace_back(ClipperLib::IntPoint(x, y));
                }
                bpolygons.paths.emplace_back(std::move(path));
            }
            Polygons cpolygons(bpolygons);
            mpolygons = cpolygons.offset(-std::round(thickness / resolution));
            mpolygons.simplify();
        }
        if (debugger) {
            //显示底面轮廓抽壳多边形
            TriPolygons polygons;
            ClipperLib::Paths paths = mpolygons.paths;
            polygons.reserve(paths.size());
            for (const auto& path : paths) {
                TriPolygon poly;
                poly.reserve(path.size());
                for (const auto& p : path) {
                    poly.emplace_back(trimesh::vec3(p.X * resolution, p.Y * resolution, 0));
                }
                polygons.emplace_back(std::move(poly));
            }
            debugger->onGenerateBottomPolygons(polygons);
        }
        //第1步，底面边界抽壳同时，生成六角网格阵列
        trimesh::dvec3 min, max;
        min.x = std::numeric_limits<double>::max();
        min.y = std::numeric_limits<double>::max();
        max.x = std::numeric_limits<double>::lowest();
        max.y = std::numeric_limits<double>::lowest();
        for (const auto& poly : polys) {
            for (const auto& p : poly) {
                if (p.x < min.x) min.x = p.x;
                if (p.y < min.y) min.y = p.y;
                if (p.x > max.x) max.x = p.x;
                if (p.y > max.y) max.y = p.y;
            }
        }
        const auto& xdist = 3.0 / 2.0 * radius;
        const auto& ydist = SQRT3 / 2.0 * radius;
        trimesh::vec3 origin(min.x, max.y, 0);

        HexagonArrayParam hexagonparams;
        hexagonparams.pos = origin;
        hexagonparams.radius = radius;
        hexagonparams.nestWidth = honeyparams.nestWidth;
        hexagonparams.ncols = std::ceil((max.x - min.x) / xdist);
        hexagonparams.nrows = (std::ceil((max.y - min.y) / ydist) + 1) / 2;
        HexaPolygons hexagons = GenerateHexagonsGridArray(hexagonparams);
        if (debugger) {
            //显示六角网格阵列
            debugger->onGenerateBottomPolygons(convertFromHexaPolygons(hexagons));
        }
        //第2步，计算网格阵列与抽壳轮廓交集
        std::vector<Polygons> hpolygons; ///<完整六角网格多边形序列
        std::vector<Polygons> ipolygons; ///<初次求交网格多边形序列
        std::vector<HexaPolygon> ihexagons;
        ihexagons.reserve(hexagons.polys.size());
        {
            hpolygons.reserve(hexagons.polys.size());
            for (auto& hexa : hexagons.polys) {
                Polygons polygons;
                ClipperLib::Path path;
                path.reserve(hexa.poly.size());
                for (const auto& p : hexa.poly) {
                    const auto& x = std::round(p.x / resolution);
                    const auto& y = std::round(p.y / resolution);
                    path.emplace_back(ClipperLib::IntPoint(x, y));
                }
                bool belong = true;
                for (const auto& p : path) {
                    if (!mpolygons.inside(p)) {
                        belong = false;
                        break;
                    }
                }
                polygons.paths.emplace_back(std::move(path));
                if (belong) {
                    hexa.standard = true;
                    ipolygons.emplace_back(std::move(polygons));
                    ihexagons.emplace_back(std::move(hexa));
                } else {
                    Polygons&& ipolys = mpolygons.intersection(polygons);
                    if (!ipolys.empty() && ipolys.paths.size() == 1) {
                        if (ipolys.area() < 0) {
                            ClipperLib::Path& path = ipolys.paths.front();
                            ClipperLib::Path tmp;
                            tmp.swap(path);
                            std::reverse(tmp.begin(), tmp.end());
                            ipolys.paths.emplace_back(std::move(tmp));
                        }

                        if (debugger) {
                            AABB box(polygons);
                            box.expand(50000);
                            std::string filename = "hexagon.svg";
                            SVG svg(filename, box, 0.01);
                            svg.writePolygons(mpolygons, SVG::Color::BLACK, 2);
                            svg.writePolygons(polygons, SVG::Color::RED, 2);
                            svg.writePolygons(ipolys, SVG::Color::BLUE, 2);
                            svg.writePoints(polygons, false, 2, SVG::Color::RAINBOW);
                            svg.writePoints(ipolys, true, 3, SVG::Color::GREEN);
                        }
                        auto&& edgemaps = GetHexagonEdgeMap(ipolys, polygons.paths[0], hexagons.side, resolution);
                        hexa.standard = false;
                        hexa.p2hPointMap.swap(edgemaps[0]);
                        hexa.h2pPointMap.swap(edgemaps[1]);
                        hexa.p2hEdgeMap.swap(edgemaps[2]);
                        hexa.h2pEdgeMap.swap(edgemaps[3]);
                        ipolygons.emplace_back(std::move(ipolys));
                        ihexagons.emplace_back(std::move(hexa));
                    }
                }
                hpolygons.emplace_back(std::move(polygons));
            }
        }
        if (debugger) {
            //显示初始求交结果
            TriPolygons tripolys;
            tripolys.reserve(ipolygons.size());
            for (const auto& ipolys : ipolygons) {
                TriPolygon poly;
                ClipperLib::Path path = ipolys.paths.front();
                poly.reserve(path.size());
                for (const auto& p : path) {
                    poly.emplace_back(trimesh::vec3(p.X * resolution, p.Y * resolution, 0));
                }
                tripolys.emplace_back(std::move(poly));
            }
            debugger->onGenerateBottomPolygons(tripolys);
        }
        //第3步，筛选符合需求的网格多边形
        TriPolygons outtripolys; ///<最终保留求交网格多边形序列
        const double minrate = honeyparams.keepHexagonRate;
        const double keeprate = 3.0 / 2.0 * SQRT3 * std::pow(side / resolution, 2) * minrate;
        const double keeparea = honeyparams.keepHexagonArea / std::pow(resolution, 2);
        const double minhexarea = std::min(keeprate, keeparea);
        std::vector<HexaPolygon> ohexagons;
        const int isize = ipolygons.size();
        ohexagons.reserve(isize);
        for (int i = 0; i < isize; i++) {
            TriPolygon tripolys;
            Polygons ipolys = ipolygons[i];
            ClipperLib::Path& path = ipolys.paths.front();
            if (ipolys.area() >= minhexarea) {
                for (const auto& p : path) {
                    const auto& point = trimesh::vec3(p.X * resolution, p.Y * resolution, 0);
                    tripolys.emplace_back(std::move(point));
                }
                outtripolys.emplace_back(tripolys);
                ihexagons[i].poly.swap(tripolys);
                ohexagons.emplace_back(ihexagons[i]);
            }
        }
        if (debugger) {
            //显示最终符合需求的网格多边形
            debugger->onGenerateBottomPolygons(outtripolys);
        }
        const int osize = ohexagons.size();
        letterOpts.hexgons.reserve(osize);
        letterOpts.side = side;
        if (honeyparams.bKeepHexagon) {
            for (int i = 0; i < osize; ++i) {
                HexaPolygon& h = ohexagons[i];
                Hexagon hexagon(h.center, hexagons.side);
                h.hexagon.swap(hexagon.border);
                Polygons polygons = SaveTriPolygonToPolygons(h.hexagon);
                Polygons ipolys = SaveTriPolygonToPolygons(h.poly);
                AABB box(polygons);
                box.expand(50000);
                std::string filename = "test/hexagon" + std::to_string(i) + ".svg";
                SVG svg(filename, box, 0.01);
                svg.writePolygons(mpolygons, SVG::Color::BLACK, 2);
                svg.writePolygons(polygons, SVG::Color::RED, 2);
                svg.writePolygons(ipolys, SVG::Color::BLUE, 2);
                svg.writePoints(polygons, false, 2, SVG::Color::RAINBOW);
                svg.writePoints(ipolys, true, 3, SVG::Color::GREEN);
            }
        }
        for (const auto& hexa : ohexagons) {
            letterOpts.hexgons.emplace_back(std::move(hexa));
        }
        return;
    }

    bool GenerateBottomHexagons(CMesh& honeyMesh, const HoneyCombParam& honeyparams, honeyLetterOpt& letterOpts, HoneyCombDebugger* debugger)
    {
        //拷贝一份数据
        CMesh cutMesh;
        cutMesh.MiniCopy(honeyMesh);
        //第3步，剪切掉底面得边界轮廓
        cutMesh.DeleteFaces(letterOpts.bottom, true);
        //cutMesh.WriteSTLFile("删除底面后剩余面片");
        std::vector<int> edges;
        cutMesh.SelectIndividualEdges(edges);
        //第4步，底面轮廓边界所有边排序
        std::vector<std::vector<int>>sequentials;
        if (!cutMesh.GetSequentialPoints(edges, sequentials))
            return false;
        //第5步，构建底面边界轮廓多边形
        TriPolygons polys;
        polys.reserve(sequentials.size());
        const auto& points = cutMesh.mpoints;
        for (const auto& seq : sequentials) {
            TriPolygon border;
            border.reserve(seq.size());
            for (const auto& v : seq) {
                const auto& p = points[v];
                border.emplace_back(p);
            }
            polys.emplace_back(border);
        }
        //第6步，在底面边界轮廓多边形内生成蜂窝六边形
        GenerateTriPolygonsHexagons(polys, honeyparams, letterOpts, debugger);
        honeyMesh.mbox = cutMesh.mbox;
        return true;
    }

    trimesh::vec3 adjustHoneyCombParam(trimesh::TriMesh* trimesh,const HoneyCombParam& honeyparams)
    {
       // if (trimesh == nullptr) return ;
        if (!honeyparams.faces.empty())
        {
            trimesh::vec3 ave_normal(0, 0, 0);
          /*  for (int fi : honeyparams.faces)
            {
                trimesh::vec3 v1 = trimesh->vertices[trimesh->faces[fi][1]] - trimesh->vertices[trimesh->faces[fi][0]];
                trimesh::vec3 v2 = trimesh->vertices[trimesh->faces[fi][2]] - trimesh->vertices[trimesh->faces[fi][0]];
                ave_normal += trimesh::normalized(v1 % v2);
            }*/
            ave_normal /= (ave_normal / (honeyparams.faces.size() * 1.f));
            trimesh::normalize(ave_normal);
           /* trimesh::vec3* dir = const_cast<trimesh::vec3*>(&honeyparams.axisDir);
            *dir = ave_normal;*/
            return ave_normal;
        }else
             return trimesh::point(0,0,1);
    }

    trimesh::TriMesh* findOutlineOfDir(trimesh::TriMesh* mesh,std::vector<int>& botfaces)
    {
        mesh->need_adjacentfaces();
        mesh->need_across_edge();
        int index;
        float min_z = std::numeric_limits<float>::max();
        for (int i = 0; i < mesh->vertices.size(); i++)
        {
            if (mesh->vertices[i].z < min_z)
            {
                min_z = mesh->vertices[i].z;
                index = i;
            }
        }
        int faceindex;
        for (int i = 0; i < mesh->adjacentfaces[index].size(); i++)
        {
            int face = mesh->adjacentfaces[index][i];
            trimesh::point v1 = mesh->vertices[mesh->faces[face][1]] - mesh->vertices[mesh->faces[face][0]];
            trimesh::point v2 = mesh->vertices[mesh->faces[face][2]] - mesh->vertices[mesh->faces[face][0]];
            trimesh::point nor = v1 % v2;
            float arc = nor ^ trimesh::point(0, 0, 1);
            if (arc < 0)
            {
                faceindex = face;
                break;
            }
        }
        std::vector<int> botface = { faceindex };
        std::vector<int> vis(mesh->faces.size(), 0);      
        std::queue<int> facequeue;
        facequeue.push(faceindex);
        while (!facequeue.empty())
        {
            vis[facequeue.front()] = 1;
            for (int i = 0; i < mesh->across_edge[facequeue.front()].size(); i++)
            {
                int face = mesh->across_edge[facequeue.front()][i];
                if (vis[face]) continue;
                trimesh::point v1 = mesh->vertices[mesh->faces[face][1]] - mesh->vertices[mesh->faces[face][0]];
                trimesh::point v2 = mesh->vertices[mesh->faces[face][2]] - mesh->vertices[mesh->faces[face][0]];
                trimesh::point nor = v1 % v2;
                float arc = trimesh::normalized(nor) ^ trimesh::point(0, 0, 1);
                arc = arc >= 1.f ? 1.f : arc;
                arc = arc <= -1.f ? -1.f : arc;
                float ang = std::acos(arc) * 180 / M_PI;
                if (arc<0/*&&ang >120*/)                                
                {
                    vis[face] = 1;
                    facequeue.push(face);
                    botface.push_back(face);
                }
            }
            facequeue.pop();
        }

       
        trimesh::TriMesh* newmesh=new trimesh::TriMesh(*mesh);
        std::vector<bool> deleteFace(mesh->faces.size(), true);
        for (int i = 0; i < botface.size(); i++)
            deleteFace[botface[i]] = false; 
        trimesh::remove_faces(newmesh, deleteFace);
        trimesh::remove_unused_vertices(newmesh);
        for (trimesh::point& v : newmesh->vertices)
            v = trimesh::point(v.x, v.y, 0);
        botfaces.swap(botface);
        /*newmesh->write("removeface.ply");
        mesh->write("mesh.ply");*/
        return newmesh;
    }

    std::shared_ptr<trimesh::TriMesh> GenerateHoneyCombs(trimesh::TriMesh* trimesh , int& code_error, const HoneyCombParam& honeyparams ,
        ccglobal::Tracer* tracer , HoneyCombDebugger* debugger )
    {
        //0.初始化Cmesh,并旋转
        if (!trimesh) {
            code_error = 1;
            return std::make_shared<trimesh::TriMesh>();
        }

        CMesh cmesh(trimesh);
        //1.重新整理输入参数
        /*trimesh::vec3 dir=adjustHoneyCombParam(trimesh, honeyparams);
        cmesh.Rotate(dir, trimesh::vec3(0, 0, 1));*/
        //2.找到生成蜂窝指定的区域（自定义或者是用户自己指定）          
        if (honeyparams.mode==0) {
            //自定义最大底面朝向
            if (honeyparams.axisDir == trimesh::vec3(0,0,0))
            {
                honeyLetterOpt letterOpts;
                //inputMesh.WriteSTLFile("inputmesh");
                //第1步，寻找底面（最大平面）朝向
                std::vector<int>bottomFaces;
                trimesh::vec3 dir = cmesh.FindBottomDirection(&bottomFaces);
                cmesh.Rotate(dir, trimesh::vec3(0, 0, -1));
                cmesh.GenerateBoundBox();
                const auto minPt = cmesh.mbox.min;
                cmesh.Translate(-minPt);
                letterOpts.bottom.resize(bottomFaces.size());
                letterOpts.bottom.assign(bottomFaces.begin(), bottomFaces.end());
                std::vector<int> honeyFaces;
                honeyFaces.reserve(cmesh.mfaces.size());
                for (int i = 0; i < cmesh.mfaces.size(); ++i) {
                    honeyFaces.emplace_back(i);
                }
                std::sort(bottomFaces.begin(), bottomFaces.end());
                std::vector<int> otherFaces;
                std::set_difference(honeyFaces.begin(), honeyFaces.end(), bottomFaces.begin(), bottomFaces.end(), std::back_inserter(otherFaces));
                letterOpts.others = std::move(otherFaces);
                //第2步，平移至xoy平面后底面整平
                cmesh.FlatBottomSurface(&bottomFaces);
                if (debugger) {
                    //显示底面区域
                    TriPolygons polygons;
                    const auto& inPoints = cmesh.mpoints;
                    const auto& inIndexs = cmesh.mfaces;
                    polygons.reserve(bottomFaces.size());
                    for (const auto& f : bottomFaces) {
                        TriPolygon poly;
                        poly.reserve(3);
                        for (int i = 0; i < 3; ++i) {
                            const auto& p = inPoints[inIndexs[f][i]];
                            poly.emplace_back(trimesh::vec3(p.x, p.y, 0));
                        }
                        polygons.emplace_back(std::move(poly));
                    }
                    debugger->onGenerateBottomPolygons(polygons);
                }
                tracer->progress(0.35f);
                //第3步，生成底面六角网格
                if (!GenerateBottomHexagons(cmesh, honeyparams, letterOpts, debugger)) {
                    code_error = 2;
                    return std::make_shared<trimesh::TriMesh>();
                }
                trimesh::TriMesh&& mesh = cmesh.GetTriMesh();            
                trimesh = &mesh;
                tracer->progress(0.5f);
                if (letterOpts.hexgons.empty()) {
                    code_error = 3;
                    return std::make_shared<trimesh::TriMesh>();
                }
                std::shared_ptr<trimesh::TriMesh> newmesh= SetHoneyCombHeight(trimesh, honeyparams, letterOpts);
                tracer->progress(0.65f);
                JointBotMesh(trimesh,newmesh.get(), bottomFaces, honeyparams.mode);
                tracer->progress(0.95f);
                trimesh::trans(newmesh.get(), minPt);
                trimesh::apply_xform(newmesh.get(), trimesh::xform::rot_into(trimesh::vec3(0, 0, -1), dir));
               // trimesh->write("trimesh.ply");
                //newmesh->write("holesColumnar.stl");
                return newmesh;
            }           
        } 
        else if(honeyparams.mode == 1){
            //user indication faceindex
            honeyLetterOpt letterOpts;
            trimesh::point normal(0,0,0);
            for (int fi = 0; fi < honeyparams.faces[0].size(); fi++)
            {
                int f = honeyparams.faces[0][fi];
                trimesh::point v1 = trimesh->vertices[trimesh->faces[f][1]] - trimesh->vertices[trimesh->faces[f][0]];
                trimesh::point v2 = trimesh->vertices[trimesh->faces[f][2]] - trimesh->vertices[trimesh->faces[f][0]];
                trimesh::point nor = v1 % v2;
                normal += nor;
            }
            //normal /= (honeyparams.faces[0].size()*1.f);
            trimesh::normalize(normal);
            cmesh.Rotate(normal, trimesh::vec3(0, 0, -1));
            cmesh.GenerateBoundBox();
            const auto minPt = cmesh.mbox.min;
            cmesh.Translate(-minPt);
            for (int hf = 0; hf < honeyparams.faces.size(); hf++)
            {
                for (int fi = 0; fi < honeyparams.faces[hf].size(); fi++)
                {
                    letterOpts.bottom.push_back(honeyparams.faces[hf][fi]);
                }
            }
            std::vector<int>bottomFaces = letterOpts.bottom;
            std::vector<int> honeyFaces;
            honeyFaces.reserve(cmesh.mfaces.size());
            for (int i = 0; i < cmesh.mfaces.size(); ++i) {
                honeyFaces.emplace_back(i);
            }
            std::sort(bottomFaces.begin(), bottomFaces.end());
            std::vector<int> otherFaces;
            std::set_difference(honeyFaces.begin(), honeyFaces.end(), bottomFaces.begin(), bottomFaces.end(), std::back_inserter(otherFaces));
            letterOpts.others = std::move(otherFaces);
            tracer->progress(0.35f);
            if (!GenerateBottomHexagons(cmesh, honeyparams, letterOpts, debugger)) {
                code_error = 2;
                return std::make_shared<trimesh::TriMesh>();
            }
            trimesh::TriMesh&& mesh = cmesh.GetTriMesh();
            trimesh = &mesh;
            tracer->progress(0.5f);
            if (letterOpts.hexgons.empty()) {
                code_error = 3;
                return std::make_shared<trimesh::TriMesh>();
            }
            std::shared_ptr<trimesh::TriMesh> newmesh = SetHoneyCombHeight(trimesh, honeyparams, letterOpts);
            tracer->progress(0.65f);
            JointBotMesh(trimesh, newmesh.get(), letterOpts.bottom, honeyparams.mode);
            tracer->progress(0.95f);
            trimesh::trans(newmesh.get(), minPt);
            trimesh::apply_xform(newmesh.get(), trimesh::xform::rot_into(trimesh::vec3(0, 0, -1), normal));
            return newmesh;
        } else {
            code_error = 4;
            return std::make_shared<trimesh::TriMesh>();
        }
    }

    void LastFaces(trimesh::TriMesh* mesh, const std::vector<int>& in, std::vector<std::vector<int>>& out, int threshold, std::vector<int>& other_shells)
    {
        std::vector<bool> mark(mesh->faces.size(), false);
        for (int i : in)
            mark[i] = true;
        bool pass = true;
        int ii = 0;
        for (;ii<mesh->faces.size();ii++)
        {
            if (mark[ii] == false)
                break;
        }
        while (pass)
        {
            std::queue<int> facequeue;
            facequeue.push(ii);
            std::vector<int> result;
            while (!facequeue.empty())
            {
                if (facequeue.front() == -1 || mark[facequeue.front()])
                {
                    facequeue.pop();
                    continue;
                }
                mark[facequeue.front()] = true;
                result.push_back(facequeue.front());
                for (int i = 0; i < mesh->across_edge[facequeue.front()].size(); i++)
                {
                    int face = mesh->across_edge[facequeue.front()][i];
                    if (face==-1||mark[face]) continue;               
                    facequeue.push(face);
                }
                facequeue.pop();
            }
            float vol = topomesh::getMeshVolume(mesh, result);
            vol = std::abs(vol);
            if (vol > threshold)
                out.push_back(result);
            else
            {
                other_shells.insert(other_shells.end(), result.begin(), result.end());
                std::set<int> filter(other_shells.begin(), other_shells.end());
                other_shells.assign(filter.begin(), filter.end());
            }
            ii = 0;
            for (; ii < mesh->faces.size(); ii++)
            {
                if (mark[ii] == false)
                    break;
            }
            if (ii == mesh->faces.size())
                pass = false;
        }
        std::sort(out.begin(), out.end(), [&](std::vector<int>& a, std::vector<int>& b)->bool
            {
               /* float va = topomesh::getMeshVolume(mesh, a);
                float vb = topomesh::getMeshVolume(mesh, b);*/
                
                float a_max_z = std::numeric_limits<float>::min();
                float a_min_z = std::numeric_limits<float>::max();
                for(int fi:a)
                    for (int v = 0; v < 3; v++)
                    {
                        float z = mesh->vertices[mesh->faces[fi][v]].z;
                        if (z > a_max_z)
                            a_max_z = z;
                        if (z < a_min_z)
                            a_min_z = z;
                    }
                float da = a_max_z - a_min_z;

                float b_max_z = std::numeric_limits<float>::min();
                float b_min_z = std::numeric_limits<float>::max();
                for (int fi : b)
                    for (int v = 0; v < 3; v++)
                    {
                        float z = mesh->vertices[mesh->faces[fi][v]].z;
                        if (z > b_max_z)
                            b_max_z = z;
                        if (z < b_min_z)
                            b_min_z = z;
                    }
                float db = b_max_z - b_min_z;

                return da > db;
            });
    }

    void FilterShells(trimesh::TriMesh* mesh, int threshold, std::vector<int>& other_shells)
    {
        std::vector<bool> mark(mesh->faces.size(), false);
        std::vector<std::vector<int>> shells_faces;
        bool pass = true;
        int ii = 0;
        for (; ii < mesh->faces.size(); ii++)
        {
            if (mark[ii] == false)
                break;
        }
        while (pass)
        {
            std::queue<int> facequeue;
            facequeue.push(ii);
            std::vector<int> result;
            while (!facequeue.empty())
            {
                if (facequeue.front() == -1 || mark[facequeue.front()])
                {
                    facequeue.pop();
                    continue;
                }
                mark[facequeue.front()] = true;
                result.push_back(facequeue.front());
                for (int i = 0; i < mesh->across_edge[facequeue.front()].size(); i++)
                {
                    int face = mesh->across_edge[facequeue.front()][i];
                    if (face == -1 || mark[face]) continue;
                    facequeue.push(face);
                }
                facequeue.pop();
            }
            shells_faces.push_back(result);

            ii = 0;
            for (; ii < mesh->faces.size(); ii++)
            {
                if (mark[ii] == false)
                    break;
            }
            if (ii == mesh->faces.size())
                pass = false;
        }
        for (int i = 0; i < shells_faces.size(); i++)
        {
            if (shells_faces[i].size() <= threshold)
            {
                for (int fi = 0; fi < shells_faces[i].size(); fi++)
                {
                    other_shells.push_back(shells_faces[i][fi]);
                }
            }
        }

    }

    std::shared_ptr<trimesh::TriMesh> SetHoneyCombHeight(trimesh::TriMesh* mesh, const HoneyCombParam& honeyparams, honeyLetterOpt& letterOpts)
    {
        mesh->bbox.valid = false;
        mesh->need_bbox();
        int row = 400; int col = 400;
        std::vector<std::tuple<trimesh::point, trimesh::point, trimesh::point>> Upfaces;
        for (int i : letterOpts.others)
            Upfaces.push_back(std::make_tuple(mesh->vertices[mesh->faces[i][0]], mesh->vertices[mesh->faces[i][1]],
                mesh->vertices[mesh->faces[i][2]]));
        topomesh::SolidTriangle upST(&Upfaces, row, col, mesh->bbox.max.x, mesh->bbox.min.x, mesh->bbox.max.y, mesh->bbox.min.y);
        upST.work();

        HexaPolygons hexpolys;
        hexpolys.side = letterOpts.side;
        trimesh::point max_xy = mesh->bbox.max;
        trimesh::point min_xy = mesh->bbox.min;
        float lengthx = (mesh->bbox.max.x - mesh->bbox.min.x) / (col * 1.f);
        float lengthy = (mesh->bbox.max.y - mesh->bbox.min.y) / (row * 1.f);

        for (auto& hg : letterOpts.hexgons)
        {
            std::vector<float> height;
            for (int i = 0; i < hg.poly.size(); i++)
            {
                float min_x = hg.poly[i].x - honeyparams.shellThickness * 3.f / 5.f;
                float min_y = hg.poly[i].y - honeyparams.shellThickness * 3.f / 5.f;
                float max_x = hg.poly[i].x + honeyparams.shellThickness * 3.f / 5.f;
                float max_y = hg.poly[i].y + honeyparams.shellThickness * 3.f / 5.f;
                int min_xi = (min_x - min_xy.x) / lengthx;
                int min_yi = (min_y - min_xy.y) / lengthy;
                int max_xi = (max_x - min_xy.x) / lengthx;
                int max_yi = (max_y - min_xy.y) / lengthy;
                float min_z = std::numeric_limits<float>::max();
                for (int xii = min_xi; xii <= max_xi; xii++)
                    for (int yii = min_yi; yii <= max_yi; yii++)
                    {
                        float zz = upST.getDataMinZInterpolation(xii, yii);
                        if (zz < min_z)
                            min_z = zz;
                    }
                if (min_z != std::numeric_limits<float>::max())
                {
                    if (min_z > 1.2 * honeyparams.shellThickness)
                        min_z -= 1.2 * honeyparams.shellThickness;
                    height.push_back(min_z);
                }
                else
                {
                    height.push_back(0.f);
                }
               // trimesh::point p = hg.poly[i] - min_xy;
               // int xi = p.x / lengthx;
               // int yi = p.y / lengthy;
               // xi = xi == col ? --xi : xi;
               // yi = yi == row ? --yi : yi;
               //// float min_z = upST.getDataMinZInterpolation(xi, yi);
               // float min_z = upST.getDataMinZCoord(xi, yi);
               // if (min_z != std::numeric_limits<float>::max())
               // {
               //     min_z -= honeyparams.shellThickness;
               //     height.push_back(min_z);
               // }
               // else
               // {
               //     height.push_back(0.f);
               // }
            }
            hg.edges.resize(hg.poly.size());
            for (int i = 0; i < hg.edges.size(); i++)
            {
                hg.edges[i].topHeight = height[i];
            }
            hexpolys.polys.push_back(hg);
        }

        topomesh::ColumnarHoleParam columnParam;
        columnParam.nslices = honeyparams.nslices;
        columnParam.ratio = honeyparams.ratio;
        columnParam.height = honeyparams.cheight;
        columnParam.delta = honeyparams.delta;
        columnParam.holeConnect = honeyparams.holeConnect;
        std::shared_ptr<trimesh::TriMesh> newmesh(topomesh::generateHolesColumnar(hexpolys, columnParam));

        std::vector<int> topfaces;
        topfaces.swap(hexpolys.topfaces);
        std::vector<int> outfaces;
        for (int l = 0; l < 2; l++)
        {
            topomesh::loopSubdivision(newmesh.get(), topfaces, outfaces);
            outfaces.swap(topfaces);
            outfaces.clear();
        }
        for (int fi : topfaces)
        {
            for (int vi = 0; vi < 3; vi++)
            {
                float min_x = newmesh->vertices[newmesh->faces[fi][vi]].x - honeyparams.shellThickness * 3.f / 5.f;
                float min_y = newmesh->vertices[newmesh->faces[fi][vi]].y - honeyparams.shellThickness * 3.f / 5.f;
                float max_x = newmesh->vertices[newmesh->faces[fi][vi]].x + honeyparams.shellThickness * 3.f / 5.f;
                float max_y = newmesh->vertices[newmesh->faces[fi][vi]].y + honeyparams.shellThickness * 3.f / 5.f;
                int min_xi = (min_x - min_xy.x) / lengthx;
                int min_yi = (min_y - min_xy.y) / lengthy;
                int max_xi = (max_x - min_xy.x) / lengthx;
                int max_yi = (max_y - min_xy.y) / lengthy;
                float min_z = std::numeric_limits<float>::max();
                for (int xii = min_xi; xii <= max_xi; xii++)
                    for (int yii = min_yi; yii <= max_yi; yii++)
                    {
                        float zz = upST.getDataMinZInterpolation(xii, yii);
                        if (zz < min_z)
                            min_z = zz;
                    }               
                if (min_z > 1.2 * honeyparams.shellThickness)
                    min_z -= 1.2 * honeyparams.shellThickness;
                newmesh->vertices[newmesh->faces[fi][vi]] = trimesh::point(newmesh->vertices[newmesh->faces[fi][vi]].x, newmesh->vertices[newmesh->faces[fi][vi]].y, min_z);              
            }
        }
        return newmesh;
    }

    trimesh::TriMesh* generateHoneyCombs(trimesh::TriMesh* trimesh, const HoneyCombParam& honeyparams,
        ccglobal::Tracer* tracer, HoneyCombDebugger* debugger)
    {	        
        honeyLetterOpt letterOpts;
        CMesh inputMesh(trimesh);
        //inputMesh.WriteSTLFile("inputmesh");
        //第1步，寻找底面（最大平面）朝向
        std::vector<int>bottomFaces;
        trimesh::vec3 dir = inputMesh.FindBottomDirection(&bottomFaces);
        inputMesh.Rotate(dir, trimesh::vec3(0, 0, -1));
        const auto& minPt = inputMesh.mbox.min;
        inputMesh.Translate(-minPt);
        letterOpts.bottom.resize(bottomFaces.size());
        letterOpts.bottom.assign(bottomFaces.begin(), bottomFaces.end());
        std::vector<int> honeyFaces;
        honeyFaces.reserve(inputMesh.mfaces.size());
        for (int i = 0; i < inputMesh.mfaces.size(); ++i) {
            honeyFaces.emplace_back(i);
        }
        std::sort(bottomFaces.begin(), bottomFaces.end());
        std::vector<int> otherFaces;
        std::set_difference(honeyFaces.begin(), honeyFaces.end(), bottomFaces.begin(), bottomFaces.end(), std::back_inserter(otherFaces));
        letterOpts.others = std::move(otherFaces);
        //第2步，平移至xoy平面后底面整平
        inputMesh.FlatBottomSurface(&bottomFaces);
        if (debugger) {
            //显示底面区域
            TriPolygons polygons;
            const auto& inPoints = inputMesh.mpoints;
            const auto& inIndexs = inputMesh.mfaces;
            polygons.reserve(bottomFaces.size());
            for (const auto& f : bottomFaces) {
                TriPolygon poly;
                poly.reserve(3);
                for (int i = 0; i < 3; ++i) {
                    const auto& p = inPoints[inIndexs[f][i]];
                    poly.emplace_back(trimesh::vec3(p.x, p.y, 0));
                }
                polygons.emplace_back(std::move(poly));
            }
            debugger->onGenerateBottomPolygons(polygons);
        }
        //第3步，底面生成六角网格
        GenerateBottomHexagons(inputMesh, honeyparams, letterOpts, debugger);
        trimesh::TriMesh&& mesh = inputMesh.GetTriMesh();

        //std::vector<bool> delectface(mesh.faces.size(), false);
        //for (int i : bottomFaces)
        //    delectface[i] = true;
        //trimesh::remove_faces(&mesh, delectface); 

        //mesh.need_bbox();
        //std::vector<std::tuple<trimesh::point, trimesh::point, trimesh::point>> data;
        //for (trimesh::TriMesh::Face& f : mesh.faces)
        //    data.push_back(std::make_tuple(mesh.vertices[f[0]], mesh.vertices[f[1]], mesh.vertices[f[2]]));
        //topomesh::SolidTriangle sd(&data, 100, 100, mesh.bbox.max.x, mesh.bbox.min.x, mesh.bbox.max.y, mesh.bbox.min.y);
        //sd.work();
        //std::vector<std::vector<float>> result = sd.getResult();
        //trimesh::TriMesh* newmesh = new trimesh::TriMesh();
        //

        //for(int i=0;i<result.size();i++)
        //    for (int j = 0; j < result[i].size(); j++)
        //    {
        //        if (result[i][j] == std::numeric_limits<float>::max()) { result[i][j] = 0.f;/* std::cout << "i :" << i << " j : " << j << "\n";*/ }
        //        newmesh->vertices.push_back(trimesh::point(i, j, result[i][j]));
        //    }
    
        //newmesh->write("newhoneymesh.ply");   
        //mesh.write("honeyhole.ply");
        //return &mesh;
		mesh.need_adjacentfaces();
		mesh.need_neighbors();
		MMeshT mt(&mesh);
		std::vector<std::vector<trimesh::vec2>> honeycombs;
		for (int i = 0; i < letterOpts.hexgons.size(); i++)		
		{
			std::vector<trimesh::vec2> tmp;
			for (int j = 0; j < letterOpts.hexgons[i].poly.size(); j++)
			{
				tmp.push_back(trimesh::vec2(letterOpts.hexgons[i].poly[j].x, letterOpts.hexgons[i].poly[j].y));
			}
			honeycombs.emplace_back(tmp);
		}	
		int old_face_size = mt.faces.size();
		embedingAndCutting(&mt,honeycombs,letterOpts.bottom);
		std::vector<std::vector<std::vector<trimesh::vec2>>> container;
		container.push_back(honeycombs);
		std::vector<int> outfacesIndex;
		int face_size = mt.faces.size();
		std::vector<int> faceindex;
		for (int i = 0; i < letterOpts.bottom.size(); i++)
			faceindex.push_back(letterOpts.bottom[i]);
		for (int i = old_face_size; i < face_size; i++)
			faceindex.push_back(i);
		polygonInnerFaces(&mt, container, faceindex, outfacesIndex);
		//concaveOrConvexOfFaces(&mt, outfacesIndex, true, 10.0f);
		for (int i = 0; i < outfacesIndex.size(); i++)
		{
			mt.faces[outfacesIndex[i]].V0(0)->SetS();
			mt.faces[outfacesIndex[i]].V1(0)->SetS();
			mt.faces[outfacesIndex[i]].V2(0)->SetS();
			mt.faces[outfacesIndex[i]].SetS();
		}		
		for (int vi = 0; vi < mt.vertices.size(); vi++)if(!mt.vertices[vi].IsD())
		{
			if (mt.vertices[vi].IsS())
			{
				for (MMeshFace* f : mt.vertices[vi].connected_face)
				{
					if (!f->IsS())
					{
						mt.vertices[vi].SetV(); break;
					}
				}
			}
		}
		std::vector<std::pair<int, float>> vd;
		findNeighVertex(&mt, letterOpts.others, outfacesIndex, vd);
		for (int i = 0; i < vd.size(); i++)
		{
			float z = vd[i].second - honeyparams.shellThickness;
			if (z < mt.vertices[vd[i].first].p.z) continue;
			if (mt.vertices[vd[i].first].IsV())
				splitPoint(&mt, &mt.vertices[vd[i].first], trimesh::point(0, 0, z));	
				//mt.appendVertex(trimesh::point(mt.vertices[vd[i].first].p + trimesh::point(0, 0, z)));
			else
				mt.vertices[vd[i].first].p += trimesh::point(0, 0, z);
		}		
		trimesh::TriMesh* newMesh = new trimesh::TriMesh();
		//mt.mmesh2trimesh(newMesh);
		mt.quickTransform(newMesh);
        //restore original pose
        const auto& dir1 = trimesh::vec3(0, 0, -1);
        const auto& dir2 = trimesh::vec3(dir.x, dir.y, dir.z);
        trimesh::fxform && mat = trimesh::fxform::rot_into(dir1, dir2);
        for (auto& p : newMesh->vertices) {
            p += trimesh::vec3(minPt.x, minPt.y, minPt.z);
            p = mat * p;
        }
		newMesh->write("honeycombs.ply");
		return newMesh;
	}

    //mode ==1  botfaces
    void JointBotMesh(trimesh::TriMesh* mesh, trimesh::TriMesh* newmesh,  std::vector<int>& botfaces, int mode)
    {
        int facesize;
        int next_facesize;
        if (mode)
        {
            std::vector<bool> deletefaces(mesh->faces.size(), false);
            for (int f = 0; f < botfaces.size(); f++)
            {
                deletefaces[botfaces[f]] = true;
            }
            trimesh::remove_faces(mesh, deletefaces);
            trimesh::remove_unused_vertices(mesh);
            facesize = newmesh->faces.size();
            int vertexsize = newmesh->vertices.size();
            for (int vi = 0; vi < mesh->vertices.size(); vi++)
                newmesh->vertices.push_back(mesh->vertices[vi]);
            for (int fi = 0; fi < mesh->faces.size(); fi++)
                newmesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[fi][0] + vertexsize, mesh->faces[fi][1] + vertexsize, mesh->faces[fi][2] + vertexsize));
            next_facesize = newmesh->faces.size();

            topomesh::MMeshT joinmesh(162460, 242460);
            std::vector<std::vector<trimesh::point>> sequentials = GetOpenMeshBoundarys(newmesh);
            std::vector<std::pair<float, int>> arc_array;
            std::vector<std::pair<int, int>> begin_and_end;
            std::vector<bool> is_reverse(sequentials.size(), false);
            for (int li = 0; li < sequentials.size(); li++)
            {
                float arc = 0.f;
                begin_and_end.push_back(std::make_pair(joinmesh.vertices.size(), joinmesh.vertices.size() + sequentials[li].size()));
                for (int vi = 0; vi < sequentials[li].size(); vi++)
                {
                    joinmesh.appendVertex(sequentials[li][vi]);
                    int next_vi = (vi + 1) % sequentials[li].size();
                    arc += (sequentials[li][vi].x * sequentials[li][next_vi].y - sequentials[li][vi].y * sequentials[li][next_vi].x);
                }
                if (arc < 0) is_reverse[li] = true;
                arc_array.push_back(std::make_pair(std::abs(arc), li));
            }

            auto arc_sort = [](std::pair<float, int>& a, std::pair<float, int>& b) {
                return a.first > b.first;
            };

            std::sort(arc_array.begin(), arc_array.end(), arc_sort);

            for (int begin = 0; begin < arc_array.size(); begin++)
            {
                std::vector<int> outfaceIndex;
                if (begin != 0)
                {
                    std::vector<std::vector<std::vector<trimesh::vec2>>> polygon(1, std::vector<std::vector<trimesh::vec2>>(1, std::vector<trimesh::vec2>()));
                    for (int vi = begin_and_end[arc_array[begin].second].first; vi < begin_and_end[arc_array[begin].second].second; vi++)
                    {
                        polygon[0][0].push_back(trimesh::vec2(joinmesh.vertices[vi].p.x, joinmesh.vertices[vi].p.y));
                    }
                    std::vector<int> infaceIndex(joinmesh.faces.size());
                    std::iota(infaceIndex.begin(), infaceIndex.end(), 0);
                    topomesh::embedingAndCutting(&joinmesh, polygon[0], infaceIndex);
                    std::vector<int> newinfaceIndex(joinmesh.faces.size());
                    std::iota(newinfaceIndex.begin(), newinfaceIndex.end(), 0);
                    topomesh::polygonInnerFaces(&joinmesh, polygon, newinfaceIndex, outfaceIndex);
                    for (int& fi : newinfaceIndex)
                    {
                        float k1 = std::abs((joinmesh.faces[fi].V0(0)->p.y - joinmesh.faces[fi].V0(1)->p.y) / (joinmesh.faces[fi].V0(0)->p.x - joinmesh.faces[fi].V0(1)->p.x));
                        float k2 = std::abs((joinmesh.faces[fi].V0(0)->p.y - joinmesh.faces[fi].V0(2)->p.y) / (joinmesh.faces[fi].V0(0)->p.x - joinmesh.faces[fi].V0(2)->p.x));
                        if (std::abs(k1 - k2) < 1e-4)
                            outfaceIndex.push_back(fi);
                    }
                }

                if (outfaceIndex.empty())
                {
                    std::vector<std::pair<trimesh::point, int>> lines;
                    if (!is_reverse[arc_array[begin].second])
                        for (int vi = begin_and_end[arc_array[begin].second].first; vi < begin_and_end[arc_array[begin].second].second; vi++)
                        {
                            lines.push_back(std::make_pair(joinmesh.vertices[vi].p, vi));
                        }
                    else
                        for (int vi = begin_and_end[arc_array[begin].second].second - 1; vi >= begin_and_end[arc_array[begin].second].first; vi--)
                        {
                            lines.push_back(std::make_pair(joinmesh.vertices[vi].p, vi));
                        }
                    topomesh::EarClipping earclip(lines);
                    std::vector<trimesh::ivec3> result = earclip.getResult();
                    for (int fi = 0; fi < result.size(); fi++)
                    {
                        joinmesh.appendFace(result[fi].x, result[fi].y, result[fi].z);
                    }
                }
                else
                {
                    for (int fi = 0; fi < outfaceIndex.size(); fi++)
                        joinmesh.deleteFace(outfaceIndex[fi]);
                }
            }

            trimesh::TriMesh* jointmesh = new trimesh::TriMesh();
            joinmesh.quickTransform(jointmesh);
            vertexsize = newmesh->vertices.size();
            for (int vi = 0; vi < jointmesh->vertices.size(); vi++)
                newmesh->vertices.push_back(jointmesh->vertices[vi]);
            for (int fi = 0; fi < jointmesh->faces.size(); fi++)
                newmesh->faces.push_back(trimesh::TriMesh::Face(jointmesh->faces[fi][0] + vertexsize, jointmesh->faces[fi][2] + vertexsize, jointmesh->faces[fi][1] + vertexsize));
            msbase::dumplicateMesh(newmesh);
            msbase::mergeNearPoints(newmesh, nullptr, 4e-3f);
            if (newmesh->vertices.empty() && newmesh->faces.empty())
                return;
        }
        else {
            std::map<int, int> vmap;
            std::map<int, int> fmap;
            topomesh::MMeshT mt(mesh, botfaces, vmap, fmap);
            std::vector<bool> deletefaces(mesh->faces.size(), false);
            for (int f = 0; f < botfaces.size(); f++)
            {
                deletefaces[botfaces[f]] = true;
            }
            trimesh::remove_faces(mesh, deletefaces);
            trimesh::remove_unused_vertices(mesh);


            std::vector<std::vector<trimesh::point>> sequentials = GetOpenMeshBoundarys(newmesh);
            std::vector<std::vector<std::vector<trimesh::vec2>>> polygon(1, std::vector<std::vector<trimesh::vec2>>(sequentials.size(), std::vector<trimesh::vec2>()));
            for (int i = 0; i < sequentials.size(); i++)
            {
                for (int j = 0; j < sequentials[i].size(); j++)
                    polygon[0][i].push_back(trimesh::vec2(sequentials[i][j].x, sequentials[i][j].y));
            }
            std::vector<int> faces(mt.faces.size());
            std::iota(faces.begin(), faces.end(), 0);
            topomesh::embedingAndCutting(&mt, polygon[0], faces);
            std::vector<int> newinfaceIndex(mt.faces.size());
            std::iota(newinfaceIndex.begin(), newinfaceIndex.end(), 0);
            std::vector<int> outfaceIndex;
            topomesh::polygonInnerFaces(&mt, polygon, newinfaceIndex, outfaceIndex);
            for (int i = 0; i < outfaceIndex.size(); i++)
                mt.deleteFace(outfaceIndex[i]);
            trimesh::TriMesh* resultmesh = new trimesh::TriMesh();
            mt.quickTransform(resultmesh);
            
            for (int fi = 0; fi < resultmesh->faces.size(); fi++)
            {
                trimesh::point v1 = resultmesh->vertices[resultmesh->faces[fi][1]] - resultmesh->vertices[resultmesh->faces[fi][0]];
                trimesh::point v2 = resultmesh->vertices[resultmesh->faces[fi][2]] - resultmesh->vertices[resultmesh->faces[fi][0]];
                float z = (v1 % v2).z;
                if (z > 0)
                    resultmesh->faces[fi] = trimesh::TriMesh::Face(resultmesh->faces[fi][0], resultmesh->faces[fi][2], resultmesh->faces[fi][1]);
            }

            facesize = newmesh->faces.size();
            int vertexsize = newmesh->vertices.size();
            for (int vi = 0; vi < resultmesh->vertices.size(); vi++)
                newmesh->vertices.push_back(resultmesh->vertices[vi]);
            for (int fi = 0; fi < resultmesh->faces.size(); fi++)
                newmesh->faces.push_back(trimesh::TriMesh::Face(resultmesh->faces[fi][0] + vertexsize, resultmesh->faces[fi][1] + vertexsize, resultmesh->faces[fi][2] + vertexsize));
            int resulrfacesize = newmesh->faces.size();

            vertexsize = newmesh->vertices.size();
            for (int vi = 0; vi < mesh->vertices.size(); vi++)
                newmesh->vertices.push_back(mesh->vertices[vi]);
            for (int fi = 0; fi < mesh->faces.size(); fi++)
                newmesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[fi][0] + vertexsize, mesh->faces[fi][1] + vertexsize, mesh->faces[fi][2] + vertexsize));
            //newmesh->write("step1.stl");
            msbase::dumplicateMesh(newmesh);           
            msbase::mergeNearPoints(newmesh, nullptr, 4e-3f);          
            if (newmesh->vertices.empty() && newmesh->faces.empty())
                return;
        }
       // newmesh->write("step2.ply");
      
        
        std::vector<bool> is_vis(newmesh->vertices.size(), false);      
        std::vector<bool> deleteface1(newmesh->faces.size(), false);
             
        auto function = [&](int begin, int end, bool other = false)
        {
            newmesh->clear_across_edge();
            newmesh->need_across_edge();
            newmesh->clear_neighbors();
            newmesh->need_neighbors();
            newmesh->clear_adjacentfaces();
            newmesh->need_adjacentfaces();

            std::vector<bool> is_boundarys(newmesh->vertices.size(), false);
            for (int vi = 0; vi < newmesh->vertices.size(); vi++)
            {
                if (newmesh->adjacentfaces[vi].size() == newmesh->neighbors[vi].size() - 1)
                    is_boundarys[vi] = true;
            }
            for (int fi = begin; fi < end; fi++)
            {
                if (other)
                    if (std::abs(newmesh->vertices[newmesh->faces[fi][0]].z - 0.f) < 1e-4f && std::abs(newmesh->vertices[newmesh->faces[fi][1]].z - 0.f) < 1e-4f &&
                        std::abs(newmesh->vertices[newmesh->faces[fi][2]].z - 0.f) < 1e-4f)
                        continue;
                bool is_boundary = false;
                int fii = 0;
                for (; fii < newmesh->across_edge[fi].size(); fii++)
                {
                    if (newmesh->across_edge[fi][fii] == -1)
                    {
                        is_boundary = true;
                        break;
                    }
                }
                if (is_boundary)
                {
                    deleteface1[fi] = true;
                    int oppovertex = fii;
                    trimesh::point fn = msbase::getFaceNormal(newmesh, fi);

                    int beginindex = newmesh->faces[fi][(oppovertex + 1) % 3];
                    int endindex = newmesh->faces[fi][(oppovertex + 2) % 3];
                    trimesh::point fdir = newmesh->vertices[endindex] - newmesh->vertices[beginindex];   
                    trimesh::normalize(fdir);
                    std::vector<int> vertexlines;
                    int vsize = vertexlines.size();
                    for (int vi = 0; vi < newmesh->neighbors[beginindex].size(); vi++)
                    {
                        int index = newmesh->neighbors[beginindex][vi];
                        if (index == newmesh->faces[fi][(oppovertex + 2) % 3]) continue;
                        if (is_boundarys[index] && !is_vis[index])
                        {
                            trimesh::point tn = (newmesh->vertices[index] - newmesh->vertices[beginindex]) %
                                (newmesh->vertices[newmesh->faces[fi][oppovertex]] - newmesh->vertices[beginindex]);
                            trimesh::normalize(tn);
                            trimesh::point tdir = newmesh->vertices[index] - newmesh->vertices[beginindex];
                            trimesh::normalize(tdir);
                            if (std::fabs(tn.x - fn.x) <= 5e-3f && std::fabs(tn.y - fn.y) <= 5e-3f && std::fabs(tn.z - fn.z) <= 5e-3f &&
                                std::fabs(tdir.x - fdir.x) <= 5e-3f && std::fabs(tdir.y - fdir.y) <= 5e-3f && std::fabs(tdir.z - fdir.z) <= 5e-3f)
                            {                               
                                vertexlines.push_back(newmesh->neighbors[beginindex][vi]);
                                is_vis[newmesh->neighbors[beginindex][vi]] = true;
                                break;
                            }
                        }
                    }
                    while (vsize != vertexlines.size())
                    {
                        vsize = vertexlines.size();
                        int index = vertexlines.back();
                        bool is_break = false;
                        if (index == newmesh->faces[fi][(oppovertex + 2) % 3]) break;
                        for (int vi = 0; vi < newmesh->neighbors[index].size(); vi++)
                        {
                            if (newmesh->neighbors[index][vi] == newmesh->faces[fi][(oppovertex + 2) % 3])
                            {
                                is_break = true;
                                break;
                            }
                        }
                        if (is_break)
                            break;
                        for (int vi = 0; vi < newmesh->neighbors[index].size(); vi++)
                        {
                            if (is_boundarys[newmesh->neighbors[index][vi]] && !is_vis[newmesh->neighbors[index][vi]])
                            {
                                trimesh::point tn = (newmesh->vertices[newmesh->neighbors[index][vi]] - newmesh->vertices[beginindex]) %
                                    (newmesh->vertices[newmesh->faces[fi][oppovertex]] - newmesh->vertices[beginindex]);
                                trimesh::normalize(tn);
                                trimesh::point tdir = newmesh->vertices[newmesh->neighbors[index][vi]] - newmesh->vertices[beginindex];
                                trimesh::normalize(tdir);
                                if (std::fabs(tn.x - fn.x) <= 5e-3f && std::fabs(tn.y - fn.y) <= 5e-3f && std::fabs(tn.z - fn.z) <= 5e-3f &&
                                    std::fabs(tdir.x - fdir.x) <= 5e-3f && std::fabs(tdir.y - fdir.y) <= 5e-3f && std::fabs(tdir.z - fdir.z) <= 5e-3f)
                                {
                                    vertexlines.push_back(newmesh->neighbors[index][vi]);
                                    is_vis[newmesh->neighbors[index][vi]] = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (vertexlines.empty())
                    {
                        deleteface1[fi] = false;
                        continue;
                    }

                    newmesh->faces.push_back(trimesh::ivec3(newmesh->faces[fi][oppovertex], newmesh->faces[fi][(oppovertex + 1) % 3], vertexlines[0]));
                    for (int ii = 0; ii < vertexlines.size() - 1; ii++)
                    {
                        newmesh->faces.push_back(trimesh::ivec3(newmesh->faces[fi][oppovertex], vertexlines[ii], vertexlines[ii + 1]));
                    }
                    newmesh->faces.push_back(trimesh::ivec3(newmesh->faces[fi][oppovertex], vertexlines.back(), newmesh->faces[fi][(oppovertex + 2) % 3]));
                }
            }
        };
        if (mode)
        {
            function(0, facesize);
            function(facesize, next_facesize, true);
        }
        else {
            function(0, facesize);
        }

        int beginsize = deleteface1.size();
        for (int fi = beginsize; fi < newmesh->faces.size(); fi++)
            deleteface1.push_back(false);

        trimesh::remove_faces(newmesh, deleteface1);
        trimesh::remove_unused_vertices(newmesh);
            

        //newmesh->write("newmesh.stl");
    }

    void SelectInnerFaces(trimesh::TriMesh* mesh,const std::vector<int>& in,int indicate, std::vector<int>& out)
    {
        std::vector<bool> mark(mesh->faces.size(),false);
        for (int i = 0; i < in.size(); i++)
            mark[in[i]] = true;
        std::queue<int> facequeue;
        facequeue.push(indicate);
        out.push_back(indicate);
        while (!facequeue.empty())
        {
            mark[facequeue.front()] = true;
            for (int i = 0; i < mesh->across_edge[facequeue.front()].size(); i++)
            {
                int face = mesh->across_edge[facequeue.front()][i];
                if (mark[face]) continue;
                mark[face] = true;
                out.push_back(face);
                facequeue.push(face);
            }
            facequeue.pop();
        }

    }

    void SelectBorderFaces(trimesh::TriMesh* mesh, int indicate, std::vector<int>& out)
    {
        findNeignborFacesOfSameAsNormal(mesh,indicate,2.f,out);
    }

    bool getTriMeshBoundarys(trimesh::TriMesh& trimesh, std::vector<std::vector<int>>& sequentials)
    {
        CMesh mesh(&trimesh);
        std::vector<int> edges;
        mesh.SelectIndividualEdges(edges);             
        return mesh.GetSequentialPoints(edges, sequentials);
    }

    HexaPolygons GenerateTriPolygonsHexagons(const TriPolygons& polys, const HexagonArrayParam& hexagonparams)
    {

        return HexaPolygons();
    }

    TriPolygon traitPlanarCircle(const trimesh::vec3& center, float r, std::vector<int>& indexs, const trimesh::vec3& dir, int nums)
    {
        TriPolygon points;
        points.reserve(nums);
        trimesh::vec3 ydir = trimesh::normalized(dir);
        trimesh::vec3 zdir(0, 0, 1);
        const float phi = 2.0f * M_PI / float(nums);
        for (int i = 0; i < nums; ++i) {
            const auto& theta = phi * (float)i + (float)M_PI_2;
            const auto& y = ydir * r * std::cos(theta);
            const auto& z = zdir * r * std::sin(theta);
            points.emplace_back(std::move(center + y + z));
        }
        float miny = points.front() DOT ydir;
        float minz = points.front() DOT zdir;
        float maxy = points.front() DOT ydir;
        float maxz = points.front() DOT zdir;
        for (int i = 1; i < nums; ++i) {
            float yn = points[i] DOT ydir;
            float zn = points[i] DOT zdir;
            if (yn < miny) miny = yn;
            if (zn < minz) minz = zn;
            if (yn > maxy) maxy = yn;
            if (zn > maxz) maxz = zn;
        }
        if (nums % 2 == 0) indexs.reserve(4);
        else indexs.reserve(5);
        for (int i = 0; i < nums; ++i) {
            float zn = points[i] DOT zdir;
            if (std::abs(zn - maxz) < EPS) {
                indexs.emplace_back(i);
            }
        }
        for (int i = 0; i < nums; ++i) {
            float yn = points[i] DOT ydir;
            if (std::abs(yn - miny) < EPS) {
                indexs.emplace_back(i);
            }
        }
        for (int i = 0; i < nums; ++i) {
            float zn = points[i] DOT zdir;
            if (std::abs(zn - minz) < EPS) {
                indexs.emplace_back(i);
            }
        }
        for (int i = 0; i < nums; ++i) {
            float yn = points[i] DOT ydir;
            if (std::abs(yn - maxy) < EPS) {
                indexs.emplace_back(i);
            }
        }
        return points;
    }

    TopoTriMeshPtr generateHolesColumnar(HexaPolygons& hexas, const ColumnarHoleParam& param)
    {
        std::vector<trimesh::vec3> points;
        std::vector<trimesh::ivec3> faces;
        int hexagonsize = 0, columnvertexs = 0;
        for (auto& hexa : hexas.polys) {
            hexa.startIndex = columnvertexs;
            const auto& poly = hexa.poly;
            for (int i = 0; i < poly.size(); ++i) {
                const auto& p = poly[i];
                points.emplace_back(trimesh::vec3(p.x, p.y, hexa.edges[i].lowHeight));
                ++columnvertexs, ++hexagonsize;
            }
        }

        for (const auto& hexa : hexas.polys) {
            const auto& poly = hexa.poly;
            for (int i = 0; i < poly.size(); ++i) {
                const auto& p = poly[i];
                points.emplace_back(trimesh::vec3(p.x, p.y, hexa.edges[i].topHeight));
                ++columnvertexs;
            }
        }
        //每条边的最低点最高点可能被更新
        GenerateHexagonNeighbors(hexas, param);
        const float cradius = hexas.side * param.ratio * 0.5f;
        const float cdelta = param.delta + 2 * cradius; ///<相邻两层圆心高度差
        const float cheight = param.height + cradius; ///<第一层圆心高度
        float cmax = param.height + 2.0 * cradius; ///<第一层圆最高点
        float addz = cdelta / 2.0; ///<相邻圆心高度差的一半
        trimesh::vec3 trans(0, 0, cdelta); ///相邻两圆平移量
        const int nslices = param.nslices;
        const int singlenums = nslices + 4;
        std::vector<int> holeStarts; ///<每个孔的第一个顶点索引
        int singleHoleEdges = 0;
        int mutiHoleEdges = 0;
        int holesnum = 0;
        int bottomfacenums = 0;
        ///六角网格底部
        if (hexas.bSewBottom) {
            ///底部网格中间矩形区域
            for (auto& hexa : hexas.polys) {
                const int size = hexa.poly.size();
                //底部完整网格中间矩形区域
                for (int i = 0; i < size; ++i) {
                    const int res = hexa.p2hEdgeMap[i]; ///<当前线段对应六角网格边长
                    if ((!hexa.edges[i].addRect) && (hexa.edges[i].neighbor >= 0) && (res >= 0)) {
                        auto& oh = hexas.polys[hexa.edges[i].neighbor];
                        const auto& h2pmap = oh.h2pEdgeMap;
                        const int inx = (res + 3) % 6; ///<临近线段对应六角网格的边长
                        auto itr = h2pmap.find(inx);
                        if (itr != h2pmap.end()) {
                            const int& start = hexa.startIndex;
                            const int& a = start + i;
                            const int& b = start + (i + 1) % size;

                            const int& ostart = oh.startIndex;
                            const int osize = oh.poly.size();
                            const int ind = itr->second;
                            const int& c = ostart + ind;
                            const int& d = ostart + (ind + 1) % osize;
                            faces.emplace_back(trimesh::ivec3(b, c, d));
                            faces.emplace_back(trimesh::ivec3(b, d, a));
                            hexa.edges[i].addRect = true;
                            oh.edges[ind].addRect = true;
                            bottomfacenums += 2;
                        }
                    }
                }
                //底部边界裁剪网格中间矩形区域
                for (int i = 0; i < size; ++i) {
                    const int res = hexa.p2hPointMap[i]; ///<当前线段起始点对应六角网格顶点
                    if ((!hexa.edges[i].addRect) && (hexa.edges[i].associate >= 0) && (res >= 0)) {
                        auto& oh = hexas.polys[hexa.edges[i].associate];
                        const auto& h2pmap = oh.h2pPointMap;
                        const int inx = (res + 3 + 1) % 6; ///<关联线段终点对应六角网格的边顶点
                        auto itr = h2pmap.find(inx);
                        if (itr != h2pmap.end()) {
                            const int& start = hexa.startIndex;
                            const int& a = start + i;
                            const int& b = start + (i + 1) % size;

                            const int& ostart = oh.startIndex;
                            const int osize = oh.poly.size();
                            const int ind = itr->second;
                            const int& d = ostart + ind;
                            const int& c = ostart + (ind + osize - 1) % osize;
                            faces.emplace_back(trimesh::ivec3(b, c, d));
                            faces.emplace_back(trimesh::ivec3(b, d, a));
                            hexa.edges[i].addRect = true;
                            oh.edges[(ind + osize - 1) % osize].addRect = true;
                            bottomfacenums += 2;
                        }
                    }
                }
            }
            //内外轮廓之间的两个裁剪网格
            for (auto& hexa : hexas.polys) {
                const int size = hexa.poly.size();
                for (int i = 0; i < size; ++i) {
                    const int res = hexa.p2hPointMap[i]; ///<当前线段始点对应六角网格顶点
                    if ((!hexa.edges[i].addRect) && (hexa.edges[i].associate >= 0) && (res >= 0)) {
                        auto& oh = hexas.polys[hexa.edges[i].associate];
                        const auto& h2pmap = oh.h2pPointMap;
                        const int inx = (res + 3) % 6; ///<始点
                        auto itr = h2pmap.find(inx);
                        if (itr != h2pmap.end()) {
                            const int& start = hexa.startIndex;
                            const int& a = start + i;
                            const int& b = start + (i + 1) % size;

                            const int& ostart = oh.startIndex;
                            const int osize = oh.poly.size();
                            const int ind = itr->second;
                            const int& c = ostart + ind;
                            const int& d = ostart + (ind + 1) % osize;
                            faces.emplace_back(trimesh::ivec3(b, c, d));
                            faces.emplace_back(trimesh::ivec3(b, d, a));
                            hexa.edges[i].addRect = true;
                            oh.edges[ind].addRect = true;
                            bottomfacenums += 2;
                        }
                    }
                }
                for (int i = 0; i < size; ++i) {
                    const int res = hexa.p2hPointMap[(i + 1) % size]; ///<当前线段终点对应六角网格顶点
                    if ((!hexa.edges[i].addRect) && (hexa.edges[i].associate >= 0) && (res >= 0)) {
                        auto& oh = hexas.polys[hexa.edges[i].associate];
                        const auto& h2pmap = oh.h2pPointMap;
                        const int inx = (res + 3) % 6; ///<终点
                        auto itr = h2pmap.find(inx);
                        if (itr != h2pmap.end()) {
                            const int& start = hexa.startIndex;
                            const int& a = start + i;
                            const int& b = start + (i + 1) % size;

                            const int& ostart = oh.startIndex;
                            const int osize = oh.poly.size();
                            const int ind = itr->second;
                            const int& c = ostart + (ind + osize - 1) % osize;
                            const int& d = ostart + ind;
                            faces.emplace_back(trimesh::ivec3(b, c, d));
                            faces.emplace_back(trimesh::ivec3(b, d, a));
                            hexa.edges[i].addRect = true;
                            oh.edges[(ind + osize - 1) % osize].addRect = true;
                            bottomfacenums += 2;
                        }
                    }
                }
            }
            ///底部三个网格中间部分
            for (auto& hexa : hexas.polys) {
                const int size = hexa.poly.size();
                for (int i = 0; i < size; ++i) {
                    const int nb = hexa.edges[i].relate;
                    const int nc = hexa.edges[(i + 1) % size].relate;
                    const int res = hexa.p2hPointMap[(i + 1) % size];
                    if ((!hexa.edges[(i + 1) % size].addTriangle) && (nb >= 0) && (nc >= 0) && (res >= 0)) {
                        auto& ohb = hexas.polys[nb];
                        auto& ohc = hexas.polys[nc];
                        const int& a = hexa.startIndex + (i + 1) % size; ///<线段终点

                        const auto& h2pmapb = ohb.h2pPointMap;
                        const int inxb = (res + 2) % 6; ///<线段始点
                        auto itrb = h2pmapb.find(inxb);
                        if (itrb != h2pmapb.end()) {
                            const int indb = itrb->second;
                            const int& b = ohb.startIndex + indb; 

                            const auto& h2pmapc = ohc.h2pPointMap;
                            const int inxc = (res + 3 + 1) % 6; ///<线段终点
                            auto itrc = h2pmapc.find(inxc);
                            if (itrc != h2pmapc.end()) {
                                const int indc = itrc->second;
                                const int sizec = ohc.poly.size();
                                const int& c = ohc.startIndex + indc;
                                faces.emplace_back(trimesh::ivec3(a, c, b));
                                hexa.edges[(i + 1) % size].addTriangle = true;
                                ohb.edges[indb].addTriangle = true;
                                ohc.edges[indc].addTriangle = true;
                                ++bottomfacenums;
                            }
                        }
                    }
                }
            }
            for (auto& hexa : hexas.polys) {
                const int size = hexa.poly.size();
                for (int i = 0; i < size; ++i) {
                    const int nb = hexa.edges[i].relate;
                    const int nc = hexa.edges[(i + 1) % size].relate;
                    const int res = hexa.p2hPointMap[(i + 1) % size];
                    if ((!hexa.edges[(i + 1) % size].addTriangle) && (nb >= 0) && (nc >= 0) && (res >= 0)) {
                        auto& ohb = hexas.polys[nb];
                        auto& ohc = hexas.polys[nc];
                        const int& a = hexa.startIndex + (i + 1) % size; ///<六角网格顶点

                        const auto& h2pmapb = ohb.h2pPointMap;
                        const int inxb = (res + 3) % 6; ///<下一个是六角网格顶点
                        auto itrb = h2pmapb.find(inxb);
                        if (itrb != h2pmapb.end()) {
                            const int sizeb = ohb.poly.size();
                            const int indb = (itrb->second + sizeb - 1) % sizeb;
                            const int& b = ohb.startIndex + indb;

                            const auto& h2pmapc = ohc.h2pPointMap;
                            const int inxc = (res + 3) % 6; ///<上一个是六角网格顶点
                            auto itrc = h2pmapc.find(inxc);
                            if (itrc != h2pmapc.end()) {
                                const int sizec = ohc.poly.size();
                                const int indc = (itrc->second + 1) % sizec;
                                const int& c = ohc.startIndex + indc;
                                faces.emplace_back(trimesh::ivec3(a, c, b));
                                hexa.edges[(i + 1) % size].addTriangle = true;
                                ohb.edges[indb].addTriangle = true;
                                ohc.edges[indc].addTriangle = true;
                                ++bottomfacenums;
                            }
                        }
                    }
                }
            }
        }
        ///添加连接孔洞的顶点坐标
        for (auto& hexa : hexas.polys) {
            const int size = hexa.poly.size();
            for (int i = 0; i < size; ++i) {
                const int res = hexa.p2hEdgeMap[i];
                if (hexa.edges[i].canAdd && (!hexa.edges[i].hasAdd) && (hexa.edges[i].neighbor >= 0) && (res >= 0)) {
                    auto& edge = hexa.edges[i];
                    auto& next = hexa.edges[(i + 1) % size];
                    auto& oh = hexas.polys[hexa.edges[i].neighbor];
                    const int inx = (res + 3) % 6;
                    const auto& h2pmap = oh.h2pEdgeMap;
                    auto itr = h2pmap.find(inx);
                    if (itr != h2pmap.end()) {
                        const int ind = itr->second;
                        const int osize = oh.poly.size();
                        auto& oe = oh.edges[ind];
                        auto& onext = oh.edges[(ind + 1) % osize];
                        float lowerlimit = std::max(edge.lowHeight, oe.lowHeight);
                        float upperlimit = std::min(edge.topHeight, oe.topHeight);
                        float medium = (lowerlimit + upperlimit) * 0.5;
                        float scope = (upperlimit - lowerlimit) * 0.5;
                        const auto& a = hexa.poly[i];
                        const auto& b = hexa.poly[(i + 1) % size];
                        const auto& c = oh.poly[ind];
                        const auto& d = oh.poly[(ind + 1) % osize];
                        if (medium < cheight && (cmax - medium) < scope) {
                            ///此时最高点下方有几个圆就可增加几个圆
                            int n = std::floor((upperlimit - cmax) / cdelta);
                            edge.holenums = n + 1, oe.holenums = n + 1;
                            edge.starts.reserve((size_t)n + 1);
                            oe.starts.reserve((size_t)n + 1);
                            const auto& ab = (a + b) * 0.5;
                            const auto& c1 = trimesh::vec3(ab.x, ab.y, cheight);
                            std::vector<int> corner1;
                            auto&& poly1 = traitPlanarCircle(c1, cradius, corner1, a - b, nslices);
                            edge.corners.swap(corner1);
                            edge.starts.emplace_back(points.size());
                            holeStarts.emplace_back(points.size());
                            points.insert(points.end(), poly1.begin(), poly1.end());

                            const auto& cd = (c + d) * 0.5;
                            const auto& c2 = trimesh::vec3(cd.x, cd.y, cheight);
                            std::vector<int> corner2;
                            auto&& poly2 = traitPlanarCircle(c2, cradius, corner2, c - d, nslices);
                            oe.corners.swap(corner2);
                            oe.starts.emplace_back(points.size());
                            holeStarts.emplace_back(points.size());
                            points.insert(points.end(), poly2.begin(), poly2.end());
                            holesnum += 2;
                            if (n < 1) singleHoleEdges += 2;
                            else {
                                mutiHoleEdges += 2;
                                edge.bmutihole = true;
                                oe.bmutihole = true;
                                edge.pointIndexs.reserve(n);
                                edge.holeIndexs.reserve(n);
                                oe.pointIndexs.reserve(n);
                                oe.holeIndexs.reserve(n);
                            }
                            for (int j = 1; j <= n; ++j) {
                                const auto& z = cheight + float(2 * j - 1) * addz;
                                edge.pointIndexs.emplace_back(points.size());
                                edge.holeIndexs.emplace_back(j);
                                points.emplace_back(trimesh::vec3(a.x, a.y, z));
                                points.emplace_back(trimesh::vec3(b.x, b.y, z));
                                oe.pointIndexs.emplace_back(points.size());
                                oe.holeIndexs.emplace_back(j);
                                points.emplace_back(trimesh::vec3(c.x, c.y, z));
                                points.emplace_back(trimesh::vec3(d.x, d.y, z));

                                edge.starts.emplace_back(points.size());
                                holeStarts.emplace_back(points.size());
                                translateTriPolygon(poly1, trans);
                                points.insert(points.end(), poly1.begin(), poly1.end());
                                oe.starts.emplace_back(points.size());
                                holeStarts.emplace_back(points.size());
                                translateTriPolygon(poly2, trans);
                                points.insert(points.end(), poly2.begin(), poly2.end());
                                holesnum += 2;
                            }
                            hexa.edges[i].hasAdd = true;
                            oh.edges[ind].hasAdd = true;
                        } else if (medium >= cheight) {
                            float ratio = (medium - cheight) / cdelta;
                            float appro = std::round(ratio);
                            float dist = std::abs((medium - cheight) - cdelta * appro);
                            if (dist + cradius < scope) {
                                ///最低点下方有几个圆孔
                                const int& n1 = lowerlimit < cmax ? 0 : ((lowerlimit - cmax) / cdelta + 1);
                                ///最高点下方有几个圆孔
                                const int& n2 = (upperlimit - cmax) / cdelta + 1;
                                const int& n = n2 - n1;
                                edge.holenums = n, oe.holenums = n;
                                edge.starts.reserve((size_t)n);
                                oe.starts.reserve((size_t)n);
                                const auto& ab = (a + b) * 0.5;
                                const auto& c1 = trimesh::vec3(ab.x, ab.y, cheight + n1 * cdelta);
                                std::vector<int> corner1;
                                auto&& poly1 = traitPlanarCircle(c1, cradius, corner1, a - b, nslices);
                                edge.corners.swap(corner1);
                                edge.starts.emplace_back(points.size());
                                holeStarts.emplace_back(points.size());
                                points.insert(points.end(), poly1.begin(), poly1.end());

                                const auto& cd = (c + d) * 0.5;
                                const auto& c2 = trimesh::vec3(cd.x, cd.y, cheight + n1 * cdelta);
                                std::vector<int> corner2;
                                auto&& poly2 = traitPlanarCircle(c2, cradius, corner2, c - d, nslices);
                                oe.corners.swap(corner2);
                                oe.starts.emplace_back(points.size());
                                holeStarts.emplace_back(points.size());
                                points.insert(points.end(), poly2.begin(), poly2.end());
                                holesnum += 2;
                                if (n <= 1) singleHoleEdges += 2;
                                else {
                                    mutiHoleEdges += 2;
                                    edge.bmutihole = true;
                                    oe.bmutihole = true;
                                    edge.pointIndexs.reserve(size_t(n) - 1);
                                    oe.pointIndexs.reserve(size_t(n) - 1);
                                    edge.holeIndexs.reserve(size_t(n) - 1);
                                    oe.holeIndexs.reserve(size_t(n) - 1);
                                }
                                edge.lowerHoles = n1;
                                oe.lowerHoles = n1;
                                for (int j = n1; j < n2 - 1; ++j) {
                                    const auto& z = cheight + float(2 * j + 1) * addz;
                                    edge.pointIndexs.emplace_back(points.size());
                                    edge.holeIndexs.emplace_back(j + 1);
                                    points.emplace_back(trimesh::vec3(a.x, a.y, z));
                                    points.emplace_back(trimesh::vec3(b.x, b.y, z));
                                    oe.pointIndexs.emplace_back(points.size());
                                    oe.holeIndexs.emplace_back(j + 1);
                                    points.emplace_back(trimesh::vec3(c.x, c.y, z));
                                    points.emplace_back(trimesh::vec3(d.x, d.y, z));

                                    edge.starts.emplace_back(points.size());
                                    holeStarts.emplace_back(points.size());
                                    translateTriPolygon(poly1, trans);
                                    points.insert(points.end(), poly1.begin(), poly1.end());
                                    oe.starts.emplace_back(points.size());
                                    holeStarts.emplace_back(points.size());
                                    translateTriPolygon(poly2, trans);
                                    points.insert(points.end(), poly2.begin(), poly2.end());
                                    holesnum += 2;
                                }
                                hexa.edges[i].hasAdd = true;
                                oh.edges[ind].hasAdd = true;
                            }
                        }
                    }
                }
            }
        }
        int noHoleEdges = hexagonsize - singleHoleEdges - mutiHoleEdges;
        int holeFaces = holesnum * singlenums;
        int rectfacenums = noHoleEdges * 2;
        int upperfacenums = hexagonsize;
        int allfacenums = holeFaces + rectfacenums + upperfacenums + bottomfacenums;
        faces.reserve(allfacenums);
        std::vector<int> topfaces;
        if (hexas.bSewTop) {
            for (int i = 0; i < hexas.polys.size(); ++i) {
                auto&& hexa = hexas.polys[i];
                const auto& poly = hexa.poly;
                int polynums = poly.size();
                const int& start = hexa.startIndex;
                int upstart = start + hexagonsize;
                ///六角网格顶部
                std::vector<std::pair<trimesh::point, int>> inputs;
                for (int i = 0; i < polynums; ++i) {
                    std::pair<trimesh::point, int> d(poly[i], i + upstart);
                    inputs.emplace_back(d);
                }
                topomesh::EarClipping clip(inputs);
                const auto& tops = clip.getResult();
                std::vector<trimesh::ivec3> fs;
                for (const auto& fa : tops) {
                    faces.emplace_back(fa[2], fa[1], fa[0]);
                    topfaces.emplace_back(faces.size() - 1);
                    //fs.emplace_back(faces.back() - upstart);
                }
                /*if (hexa.standard) {
                    for (int j = 1; j < polynums - 1; ++j) {
                        const int& cur = upstart + j;
                        const int& next = upstart + j + 1;
                        faces.emplace_back(trimesh::ivec3(upstart, next, cur));
                        topfaces.push_back(faces.size() - 1);
                    }
                } else {
                    int near = 0, far = 0;
                    const auto& center = hexa.center;
                    float mindist = trimesh::len2(poly[0] - center);
                    for (int j = 1; j < polynums; ++j) {
                        float d = trimesh::len2(poly[j] - hexa.center);
                        if (d < mindist) {
                            mindist = d;
                            near = j;
                        }
                    }
                    const trimesh::vec3& nearPt = poly[near];
                    const auto& dir = nearPt - center;
                    float maxdist = 0;
                    if (hexa.h2pPointMap.size() > 0) {
                        for (int j = 0; j < polynums; ++j) {
                            if (hexa.p2hPointMap[j] >= 0) {
                                float d = std::fabs((poly[j] - center) DOT dir);
                                if (d > maxdist) {
                                    maxdist = d;
                                    far = j;
                                }
                            }
                        }
                    } else {
                        for (int j = 0; j < polynums; ++j) {
                            float d = std::fabs((poly[j] - center) DOT dir);
                            if (d > maxdist) {
                                maxdist = d;
                                far = j;
                            }
                        }
                    }
                    int newupstart = start + hexagonsize + far;
                    for (int j = far + 1; j < polynums + far - 1; ++j) {
                        const int& cur = upstart + j % polynums;
                        const int& next = upstart + (j + 1) % polynums;
                        faces.emplace_back(trimesh::ivec3(newupstart, next, cur));
                        topfaces.push_back(faces.size() - 1);
                    }
                }*/
            }
        }
        hexas.topfaces.swap(topfaces);
        for (int i = 0; i < hexas.polys.size(); ++i) {
            const auto& hexa = hexas.polys[i];
            const auto& poly = hexa.poly;
            const int polysize = poly.size();
            const int& start = hexa.startIndex;
            //六角网格侧面
            for (int j = 0; j < polysize; ++j) {
                ///侧面下方两个顶点索引
                int a = start + j;
                int b = start + (j + 1) % polysize;
                ///侧面上方两个顶点索引
                int c = a + hexagonsize;
                int d = b + hexagonsize;
                if (hexa.edges[j].canAdd) {
                    const auto& edge = hexa.edges[j];
                    const auto& corner = edge.corners;
                    int r = corner[0];
                    int s = corner[1];
                    int t = corner[2];
                    int u = corner[4];
                    if (nslices % 2 == 0) {
                        u = corner[3];
                    }
                    const auto& ledge = hexa.edges[(j + polysize - 1) % polysize];
                    const auto& nedge = hexa.edges[(j + 1) % polysize];
                    const auto& cIndexs = edge.holeIndexs;
                    const auto& lIndexs = ledge.holeIndexs;
                    const auto& nIndexs = nedge.holeIndexs;

                    if (edge.bmutihole) {
                        //第一个圆孔的第一个顶点的索引
                        std::vector<int> ldiffs, ndiffs;
                        std::set_difference(lIndexs.begin(), lIndexs.end(), cIndexs.begin(), cIndexs.end(), std::back_inserter(ldiffs));
                        std::set_difference(nIndexs.begin(), nIndexs.end(), cIndexs.begin(), cIndexs.end(), std::back_inserter(ndiffs));
                        int cstart = edge.starts.front();
                        c = edge.pointIndexs.front(); d = c + 1;
                        faces.emplace_back(trimesh::ivec3(cstart + r, c, d));
                        for (int k = 0; k < s; ++k) {
                            faces.emplace_back(trimesh::ivec3(cstart + k, d, cstart + k + 1));
                        }
                        //处理顶端b附近区域
                        if (!ndiffs.empty()) {
                            auto bf = std::find(ndiffs.begin(), ndiffs.end(), cIndexs.front() - 1);
                            if (bf != ndiffs.end()) {
                                int n0 = nedge.lowerHoles + 1;
                                int pos = std::distance(ndiffs.begin(), bf);
                                const auto& npoints = nedge.pointIndexs;
                                const int& b0 = npoints[(size_t)ndiffs[pos] - n0];
                                faces.emplace_back(trimesh::ivec3(b0, cstart + s, d));
                                for (size_t k = 0; k < pos; ++k) {
                                    faces.emplace_back(trimesh::ivec3(npoints[(size_t)ndiffs[k] - n0], cstart + s, npoints[(size_t)ndiffs[k + 1] - n0]));
                                }
                                faces.emplace_back(trimesh::ivec3(b, cstart + s, npoints.front()));
                            } else {
                                faces.emplace_back(trimesh::ivec3(b, cstart + s, d));
                            }
                        } else {
                            faces.emplace_back(trimesh::ivec3(b, cstart + s, d));
                        }
                        for (int k = s; k < t; ++k) {
                            faces.emplace_back(trimesh::ivec3(cstart + k, b, cstart + k + 1));
                        }
                        faces.emplace_back(trimesh::ivec3(b, a, cstart + t));
                        //处理顶端a附近区域
                        if (!ldiffs.empty()) {
                            auto af = std::find(ldiffs.begin(), ldiffs.end(), cIndexs.front() - 1);
                            if (af != ldiffs.end()) {
                                int n0 = ledge.lowerHoles + 1;
                                int pos = std::distance(ldiffs.begin(), af);
                                const auto& lpoints = ledge.pointIndexs;
                                faces.emplace_back(trimesh::ivec3(lpoints.front() + 1, cstart + t, a));
                                for (size_t k = 0; k < pos; ++k) {
                                    faces.emplace_back(trimesh::ivec3(lpoints[(size_t)ldiffs[k] - n0] + 1, cstart + t, lpoints[(size_t)ldiffs[k + 1] - n0]) + 1);
                                }
                                a = lpoints[(size_t)ldiffs[pos] - n0] + 1;
                            }
                        }
                        for (int k = t; k < u; ++k) {
                            faces.emplace_back(trimesh::ivec3(a, cstart + k + 1, cstart + k));
                        }
                        faces.emplace_back(trimesh::ivec3(a, c, cstart + u));
                        for (int k = u; k < nslices; ++k) {
                            faces.emplace_back(trimesh::ivec3(c, cstart + (k + 1) % nslices, cstart + k));
                        }
                        //中间的圆孔构建面片
                        for (int num = 1; num < edge.holenums - 1; ++num) {
                            cstart = edge.starts[num];
                            a = c; b = d;
                            c = edge.pointIndexs[num]; d = c + 1;

                            faces.emplace_back(trimesh::ivec3(cstart + r, c, d));
                            for (int k = 0; k < s; ++k) {
                                faces.emplace_back(trimesh::ivec3(cstart + k, d, cstart + k + 1));
                            }
                            faces.emplace_back(trimesh::ivec3(b, cstart + s, d));
                            for (int k = s; k < t; ++k) {
                                faces.emplace_back(trimesh::ivec3(cstart + k, b, cstart + k + 1));
                            }
                            faces.emplace_back(trimesh::ivec3(b, a, cstart + t));
                            for (int k = t; k < u; ++k) {
                                faces.emplace_back(trimesh::ivec3(a, cstart + k + 1, cstart + k));
                            }
                            faces.emplace_back(trimesh::ivec3(a, c, cstart + u));
                            for (int k = u; k < nslices; ++k) {
                                faces.emplace_back(trimesh::ivec3(c, cstart + (k + 1) % nslices, cstart + k));
                            }
                        }
                        //最上方圆孔构建面
                        cstart = edge.starts.back();
                        a = c; b = d;
                        c = start + j + hexagonsize;
                        d = start + (j + 1) % polysize + hexagonsize;
                        faces.emplace_back(trimesh::ivec3(cstart + r, c, d));
                        //处理顶端d附近区域
                        if (!ndiffs.empty()) {
                            auto df = std::find(ndiffs.begin(), ndiffs.end(), cIndexs.back() + 1);
                            if (df != ndiffs.end()) {
                                int n0 = nedge.lowerHoles + 1;
                                int pos = std::distance(ndiffs.begin(), df);
                                const auto& npoints = nedge.pointIndexs;
                                for (size_t k = pos; k < ndiffs.size() - 1; ++k) {
                                    faces.emplace_back(trimesh::ivec3(npoints[(size_t)ndiffs[k] - n0], cstart + r, npoints[(size_t)ndiffs[k + 1] - n0]));
                                }
                                faces.emplace_back(trimesh::ivec3(npoints[(size_t)ndiffs.back() - n0], cstart + r, d));
                                d = npoints[(size_t)ndiffs[pos] - n0];
                            }
                        }
                        for (int k = 0; k < s; ++k) {
                            faces.emplace_back(trimesh::ivec3(cstart + k, d, cstart + k + 1));
                        }
                        faces.emplace_back(trimesh::ivec3(b, cstart + s, d));
                        for (int k = s; k < t; ++k) {
                            faces.emplace_back(trimesh::ivec3(cstart + k, b, cstart + k + 1));
                        }
                        faces.emplace_back(trimesh::ivec3(b, a, cstart + t));
                        for (int k = t; k < u; ++k) {
                            faces.emplace_back(trimesh::ivec3(a, cstart + k + 1, cstart + k));
                        }
                        //处理顶端c附近区域
                        if (!ldiffs.empty()) {
                            auto cf = std::find(ldiffs.begin(), ldiffs.end(), cIndexs.back() + 1);
                            if (cf != ldiffs.end()) {
                                int n0 = ledge.lowerHoles + 1;
                                int pos = std::distance(ldiffs.begin(), cf);
                                const auto& lpoints = ledge.pointIndexs;
                                faces.emplace_back(trimesh::ivec3(a, lpoints[(size_t)ldiffs[pos] - n0] + 1, cstart + u));
                                for (size_t k = pos; k < ldiffs.size() - 1; ++k) {
                                    faces.emplace_back(trimesh::ivec3(lpoints[(size_t)ldiffs[k] - n0] + 1, lpoints[(size_t)ldiffs[k + 1] - n0] + 1, cstart + r));
                                }
                                faces.emplace_back(trimesh::ivec3(lpoints.back() + 1, c, cstart + r));
                                c = lpoints[(size_t)ldiffs[pos] - n0] + 1;
                            } else {
                                faces.emplace_back(trimesh::ivec3(a, c, cstart + u));
                            }
                        } else {
                            faces.emplace_back(trimesh::ivec3(a, c, cstart + u));
                        }
                        for (int k = u; k < nslices; ++k) {
                            faces.emplace_back(trimesh::ivec3(c, cstart + (k + 1) % nslices, cstart + k));
                        }
                    } else {
                        ///只有单孔的侧面
                        int cstart = edge.starts.front();
                        faces.emplace_back(trimesh::ivec3(cstart + r, c, d));
                        //处理顶端d附近区域
                        if (!nIndexs.empty()) {
                            auto df = std::find(nIndexs.begin(), nIndexs.end(), 1);
                            if (df != nIndexs.end()) {
                                int pos = std::distance(nIndexs.begin(), df);
                                const auto& npoints = nedge.pointIndexs;
                                for (size_t k = pos; k < nIndexs.size() - 1; ++k) {
                                    faces.emplace_back(trimesh::ivec3(npoints[(size_t)nIndexs[k] - 1], cstart + r, npoints[(size_t)nIndexs[k + 1] - 1]));
                                }
                                faces.emplace_back(trimesh::ivec3(npoints[(size_t)nIndexs.back() - 1], cstart + r, d));
                                d = npoints[(size_t)nIndexs[pos] - 1];
                            }
                        }
                        for (int k = 0; k < s; ++k) {
                            faces.emplace_back(trimesh::ivec3(cstart + k, d, cstart + k + 1));
                        }
                        //b端不可能插入矩阵点，无需处理
                        faces.emplace_back(trimesh::ivec3(b, cstart + s, d));
                        for (int k = s; k < t; ++k) {
                            faces.emplace_back(trimesh::ivec3(cstart + k, b, cstart + k + 1));
                        }
                        faces.emplace_back(trimesh::ivec3(b, a, cstart + t));
                        //a端不可能插入矩阵点，无需处理
                        for (int k = t; k < u; ++k) {
                            faces.emplace_back(trimesh::ivec3(a, cstart + k + 1, cstart + k));
                        }
                        //处理顶端c附近区域
                        if (!lIndexs.empty()) {
                            auto cf = std::find(lIndexs.begin(), lIndexs.end(), 1);
                            if (cf != lIndexs.end()) {
                                int pos = std::distance(lIndexs.begin(), cf);
                                const auto& lpoints = ledge.pointIndexs;
                                faces.emplace_back(trimesh::ivec3(a, lpoints[(size_t)lIndexs[pos] - 1] + 1, cstart + u));
                                for (size_t k = pos; k < lIndexs.size() - 1; ++k) {
                                    faces.emplace_back(trimesh::ivec3(lpoints[(size_t)lIndexs[k] - 1] + 1, lpoints[(size_t)lIndexs[k + 1] - 1] + 1, cstart + r));
                                }
                                faces.emplace_back(trimesh::ivec3(lpoints.back() + 1, c, cstart + r));
                                c = lpoints[(size_t)lIndexs[pos] - 1] + 1;
                            } else {
                                faces.emplace_back(trimesh::ivec3(a, c, cstart + u));
                            }
                        } else {
                            faces.emplace_back(trimesh::ivec3(a, c, cstart + u));
                        }
                        for (int k = u; k < nslices; ++k) {
                            faces.emplace_back(trimesh::ivec3(c, cstart + (k + 1) % nslices, cstart + k));
                        }
                    }
                } else {
                    ///无孔的侧面
                    const auto& ledge = hexa.edges[(j + polysize - 1) % polysize];
                    const auto& nedge = hexa.edges[(j + 1) % polysize];
                    if (ledge.bmutihole && nedge.bmutihole) {
                        const auto& lIndexs = ledge.pointIndexs;
                        const auto& nIndexs = nedge.pointIndexs;
                        faces.emplace_back(trimesh::ivec3(b, a, lIndexs.front() + 1));
                        for (size_t k = 0; k < lIndexs.size() - 1; ++k) {
                            faces.emplace_back(trimesh::ivec3(b, lIndexs[k] + 1, lIndexs[k + 1] + 1));
                        }
                        faces.emplace_back(trimesh::ivec3(b, lIndexs.back() + 1, c));
                        faces.emplace_back(trimesh::ivec3(b, c, nIndexs.front()));
                        for (size_t k = 0; k < nIndexs.size() - 1; ++k) {
                            faces.emplace_back(trimesh::ivec3(nIndexs[k], c, nIndexs[k + 1]));
                        }
                        faces.emplace_back(trimesh::ivec3(nIndexs.back(), c, d));
                    } else if (ledge.bmutihole && (!nedge.bmutihole)) {
                        const auto& lIndexs = ledge.pointIndexs;
                        faces.emplace_back(trimesh::ivec3(b, a, lIndexs.front() + 1));
                        for (size_t k = 0; k < lIndexs.size() - 1; ++k) {
                            faces.emplace_back(trimesh::ivec3(b, lIndexs[k] + 1, lIndexs[k + 1] + 1));
                        }
                        faces.emplace_back(trimesh::ivec3(b, lIndexs.back() + 1, c));
                        faces.emplace_back(trimesh::ivec3(b, c, d));
                    } else if ((!ledge.bmutihole) && nedge.bmutihole) {
                        const auto& nIndexs = nedge.pointIndexs;
                        faces.emplace_back(trimesh::ivec3(b, a, c));
                        faces.emplace_back(trimesh::ivec3(b, c, nIndexs.front()));
                        for (size_t k = 0; k < nIndexs.size() - 1; ++k) {
                            faces.emplace_back(trimesh::ivec3(nIndexs[k], c, nIndexs[k + 1]));
                        }
                        faces.emplace_back(trimesh::ivec3(nIndexs.back(), c, d));
                    } else {
                        faces.emplace_back(trimesh::ivec3(b, a, c));
                        faces.emplace_back(trimesh::ivec3(b, c, d));
                    }
                }
            }
        }
        ///六角网格桥接的孔洞部
        for (int i = 0; i < holesnum; i += 2) {
            const int& start = holeStarts[i];
            for (int j = 0; j < nslices; ++j) {
                const int& lcur = start + j;
                const int& lnext = start + (j + 1) % nslices;
                const int& rcur = start + (nslices - j) % nslices + nslices;
                const int& rnext = start + (nslices - j - 1) % nslices + nslices;
                faces.emplace_back(trimesh::ivec3(lnext, rnext, rcur));
                faces.emplace_back(trimesh::ivec3(lnext, rcur, lcur));
            }
        }
        std::shared_ptr<trimesh::TriMesh> triMesh(new trimesh::TriMesh());
        triMesh->vertices.swap(points);
        triMesh->faces.swap(faces);
        msbase::dumplicateMesh(triMesh.get());
        return triMesh;
    }

    HexaPolygons generateEmbedHolesColumnar(trimesh::TriMesh* trimesh, const HoneyCombParam& honeyparams, ccglobal::Tracer* tracer, HoneyCombDebugger* debugger)
    {
        HexaPolygons hexas;
        //0.初始化Cmesh,并旋转
        if (trimesh == nullptr) return hexas;
        CMesh cmesh(trimesh);
        //1.重新整理输入参数
        /*trimesh::vec3 dir=adjustHoneyCombParam(trimesh, honeyparams);
        cmesh.Rotate(dir, trimesh::vec3(0, 0, 1));*/
        //2.找到生成蜂窝指定的区域（自定义或者是用户自己指定）          
        if (honeyparams.mode == 0) {
            //自定义最大底面朝向
            if (honeyparams.axisDir == trimesh::vec3(0, 0, 0)) {
                honeyLetterOpt letterOpts;
                cmesh.WriteSTLFile("inputmesh");
                //第1步，寻找底面（最大平面）朝向
                std::vector<int>bottomFaces;
                trimesh::vec3 dir = cmesh.FindBottomDirection(&bottomFaces);
                cmesh.Rotate(dir, trimesh::vec3(0, 0, -1));
                cmesh.GenerateBoundBox();
                const auto minPt = cmesh.mbox.min;
                cmesh.Translate(-minPt);
                letterOpts.bottom.resize(bottomFaces.size());
                letterOpts.bottom.assign(bottomFaces.begin(), bottomFaces.end());
                std::vector<int> honeyFaces;
                honeyFaces.reserve(cmesh.mfaces.size());
                for (int i = 0; i < cmesh.mfaces.size(); ++i) {
                    honeyFaces.emplace_back(i);
                }
                std::sort(bottomFaces.begin(), bottomFaces.end());
                std::vector<int> otherFaces;
                std::set_difference(honeyFaces.begin(), honeyFaces.end(), bottomFaces.begin(), bottomFaces.end(), std::back_inserter(otherFaces));
                letterOpts.others = std::move(otherFaces);
                //第2步，平移至xoy平面后底面整平
                cmesh.FlatBottomSurface(&bottomFaces);
                if (debugger) {
                    //显示底面区域
                    TriPolygons polygons;
                    const auto& inPoints = cmesh.mpoints;
                    const auto& inIndexs = cmesh.mfaces;
                    polygons.reserve(bottomFaces.size());
                    for (const auto& f : bottomFaces) {
                        TriPolygon poly;
                        poly.reserve(3);
                        for (int i = 0; i < 3; ++i) {
                            const auto& p = inPoints[inIndexs[f][i]];
                            poly.emplace_back(trimesh::vec3(p.x, p.y, 0));
                        }
                        polygons.emplace_back(std::move(poly));
                    }
                    debugger->onGenerateBottomPolygons(polygons);
                }
                //第3步，生成底面六角网格
                GenerateBottomHexagons(cmesh, honeyparams, letterOpts, debugger);
                trimesh::TriMesh&& mesh = cmesh.GetTriMesh();
                trimesh = &mesh;
                trimesh->bbox.valid = false;
                trimesh->need_bbox();
                int row = 400; int col = 400;
                std::vector<std::tuple<trimesh::point, trimesh::point, trimesh::point>> Upfaces;
                for (int i : letterOpts.others)
                    Upfaces.push_back(std::make_tuple(trimesh->vertices[trimesh->faces[i][0]], trimesh->vertices[trimesh->faces[i][1]],
                        trimesh->vertices[trimesh->faces[i][2]]));
                topomesh::SolidTriangle upST(&Upfaces, row, col, trimesh->bbox.max.x, trimesh->bbox.min.x, trimesh->bbox.max.y, trimesh->bbox.min.y);
                upST.work();

                hexas.side = letterOpts.side;
                trimesh::point max_xy = trimesh->bbox.max;
                trimesh::point min_xy = trimesh->bbox.min;
                float lengthx = (trimesh->bbox.max.x - trimesh->bbox.min.x) / (col * 1.f);
                float lengthy = (trimesh->bbox.max.y - trimesh->bbox.min.y) / (row * 1.f);

                for (auto& hg : letterOpts.hexgons) {
                    std::vector<float> height;
                    for (int i = 0; i < hg.poly.size(); i++) {
                        float min_x = hg.poly[i].x - honeyparams.shellThickness * 3.f / 5.f;
                        float min_y = hg.poly[i].y - honeyparams.shellThickness * 3.f / 5.f;
                        float max_x = hg.poly[i].x + honeyparams.shellThickness * 3.f / 5.f;
                        float max_y = hg.poly[i].y + honeyparams.shellThickness * 3.f / 5.f;
                        int min_xi = (min_x - min_xy.x) / lengthx;
                        int min_yi = (min_y - min_xy.y) / lengthy;
                        int max_xi = (max_x - min_xy.x) / lengthx;
                        int max_yi = (max_y - min_xy.y) / lengthy;
                        float min_z = std::numeric_limits<float>::max();
                        for (int xii = min_xi; xii <= max_xi; xii++)
                            for (int yii = min_yi; yii <= max_yi; yii++) {
                                float zz = upST.getDataMinZInterpolation(xii, yii);
                                if (zz < min_z)
                                    min_z = zz;
                            }
                        if (min_z != std::numeric_limits<float>::max()) {
                            if (min_z > 1.2 * honeyparams.shellThickness)
                                min_z -= 1.2 * honeyparams.shellThickness;
                            height.push_back(min_z);
                        } else {
                            height.push_back(0.f);
                        }
                        // trimesh::point p = hg.poly[i] - min_xy;
                        // int xi = p.x / lengthx;
                        // int yi = p.y / lengthy;
                        // xi = xi == col ? --xi : xi;
                        // yi = yi == row ? --yi : yi;
                        //// float min_z = upST.getDataMinZInterpolation(xi, yi);
                        // float min_z = upST.getDataMinZCoord(xi, yi);
                        // if (min_z != std::numeric_limits<float>::max())
                        // {
                        //     min_z -= honeyparams.shellThickness;
                        //     height.push_back(min_z);
                        // }
                        // else
                        // {
                        //     height.push_back(0.f);
                        // }
                    }
                    hg.edges.resize(hg.poly.size());
                    for (int i = 0; i < hg.edges.size(); i++) {
                        hg.edges[i].topHeight = height[i];
                    }
                    hexas.polys.push_back(hg);
                }
                return hexas;
            }
        } else if (honeyparams.mode == 1) {
            //user indication faceindex
            honeyLetterOpt letterOpts;
            trimesh::point normal(0, 0, 0);
            for (int fi = 0; fi < honeyparams.faces[0].size(); fi++) {
                int f = honeyparams.faces[0][fi];
                trimesh::point v1 = trimesh->vertices[trimesh->faces[f][1]] - trimesh->vertices[trimesh->faces[f][0]];
                trimesh::point v2 = trimesh->vertices[trimesh->faces[f][2]] - trimesh->vertices[trimesh->faces[f][0]];
                trimesh::point nor = v1 % v2;
                normal += nor;
            }
            //normal /= (honeyparams.faces[0].size()*1.f);
            trimesh::normalize(normal);
            cmesh.Rotate(normal, trimesh::vec3(0, 0, -1));
            cmesh.GenerateBoundBox();
            const auto minPt = cmesh.mbox.min;
            cmesh.Translate(-minPt);
            for (int hf = 0; hf < honeyparams.faces.size(); hf++) {
                for (int fi = 0; fi < honeyparams.faces[hf].size(); fi++) {
                    letterOpts.bottom.push_back(honeyparams.faces[hf][fi]);
                }
            }
            std::vector<int>bottomFaces = letterOpts.bottom;
            std::vector<int> honeyFaces;
            honeyFaces.reserve(cmesh.mfaces.size());
            for (int i = 0; i < cmesh.mfaces.size(); ++i) {
                honeyFaces.emplace_back(i);
            }
            std::sort(bottomFaces.begin(), bottomFaces.end());
            std::vector<int> otherFaces;
            std::set_difference(honeyFaces.begin(), honeyFaces.end(), bottomFaces.begin(), bottomFaces.end(), std::back_inserter(otherFaces));
            letterOpts.others = std::move(otherFaces);
            GenerateBottomHexagons(cmesh, honeyparams, letterOpts, debugger);
            trimesh::TriMesh && mesh = cmesh.GetTriMesh();
            trimesh = &mesh;
            trimesh->bbox.valid = false;
            trimesh->need_bbox();
            int row = 400; int col = 400;
            std::vector<std::tuple<trimesh::point, trimesh::point, trimesh::point>> Upfaces;
            for (int i : letterOpts.others)
                Upfaces.push_back(std::make_tuple(trimesh->vertices[trimesh->faces[i][0]], trimesh->vertices[trimesh->faces[i][1]],
                    trimesh->vertices[trimesh->faces[i][2]]));
            topomesh::SolidTriangle upST(&Upfaces, row, col, trimesh->bbox.max.x, trimesh->bbox.min.x, trimesh->bbox.max.y, trimesh->bbox.min.y);
            upST.work();

            hexas.side = letterOpts.side;
            trimesh::point max_xy = trimesh->bbox.max;
            trimesh::point min_xy = trimesh->bbox.min;
            float lengthx = (trimesh->bbox.max.x - trimesh->bbox.min.x) / (col * 1.f);
            float lengthy = (trimesh->bbox.max.y - trimesh->bbox.min.y) / (row * 1.f);

            for (auto& hg : letterOpts.hexgons) {
                std::vector<float> height;
                for (int i = 0; i < hg.poly.size(); i++) {
                    float min_x = hg.poly[i].x - honeyparams.shellThickness * 3.f / 5.f;
                    float min_y = hg.poly[i].y - honeyparams.shellThickness * 3.f / 5.f;
                    float max_x = hg.poly[i].x + honeyparams.shellThickness * 3.f / 5.f;
                    float max_y = hg.poly[i].y + honeyparams.shellThickness * 3.f / 5.f;
                    int min_xi = (min_x - min_xy.x) / lengthx;
                    int min_yi = (min_y - min_xy.y) / lengthy;
                    int max_xi = (max_x - min_xy.x) / lengthx;
                    int max_yi = (max_y - min_xy.y) / lengthy;
                    float min_z = std::numeric_limits<float>::max();
                    for (int xii = min_xi; xii <= max_xi; xii++)
                        for (int yii = min_yi; yii <= max_yi; yii++) {
                            float zz = upST.getDataMinZInterpolation(xii, yii);
                            if (zz < min_z)
                                min_z = zz;
                        }
                    if (min_z != std::numeric_limits<float>::max()) {
                        if (min_z > 1.2 * honeyparams.shellThickness)
                            min_z -= 1.2 * honeyparams.shellThickness;
                        height.push_back(min_z);
                    } else {
                        height.push_back(0.f);
                    }
                    // trimesh::point p = hg.poly[i] - min_xy;
                    // int xi = p.x / lengthx;
                    // int yi = p.y / lengthy;
                    // xi = xi == col ? --xi : xi;
                    // yi = yi == row ? --yi : yi;
                    //// float min_z = upST.getDataMinZInterpolation(xi, yi);
                    // float min_z = upST.getDataMinZCoord(xi, yi);
                    // if (min_z != std::numeric_limits<float>::max())
                    // {
                    //     min_z -= honeyparams.shellThickness;
                    //     height.push_back(min_z);
                    // }
                    // else
                    // {
                    //     height.push_back(0.f);
                    // }
                }
                hg.edges.resize(hg.poly.size());
                for (int i = 0; i < hg.edges.size(); i++) {
                    hg.edges[i].topHeight = height[i];
                }
                hexas.polys.push_back(hg);
            }
        } else
            return hexas;
    }



	void findNeighVertex(MMeshT* mesh, const std::vector<int>& upfaceid, const std::vector<int>& botfaceid, std::vector<std::pair<int, float>>& vertex_distance)
	{		
		
		std::vector<int> compara_vertex;		
		for (int i = 0; i < botfaceid.size(); i++)
		{
			compara_vertex.push_back(mesh->faces[botfaceid[i]].V0(0)->index);
			compara_vertex.push_back(mesh->faces[botfaceid[i]].V1(0)->index);
			compara_vertex.push_back(mesh->faces[botfaceid[i]].V2(0)->index);
		}
		std::sort(compara_vertex.begin(), compara_vertex.end());
		std::vector<int>::iterator it = std::unique(compara_vertex.begin(), compara_vertex.end());
		compara_vertex.resize(std::distance(compara_vertex.begin(), it));

		const int width = 100, height = 100;
		//std::vector<std::vector<float>> mapp(width, std::vector<float>(height, std::numeric_limits<float>::max()));
		std::vector<std::vector<std::vector<int>>> mapind(width, std::vector<std::vector<int>>(height));
		
		mesh->getBoundingBox();
		float length_x = (mesh->boundingbox.max_x - mesh->boundingbox.min_x) / (width * 1.0f);
		float length_y = (mesh->boundingbox.max_y - mesh->boundingbox.min_y) / (height * 1.0f);

		float begin_x = mesh->boundingbox.min_x;
		float begin_y = mesh->boundingbox.min_y;


		for (int i = 0; i < upfaceid.size(); i++)
		{
			float max_x=std::numeric_limits<float>::min(), max_y = std::numeric_limits<float>::min();
			float min_x = std::numeric_limits<float>::max(), min_y = std::numeric_limits<float>::max();
			for (int j = 0; j < 3; j++)
			{
				if (mesh->faces[upfaceid[i]].V0(j)->p.x > max_x)
					max_x = mesh->faces[upfaceid[i]].V0(j)->p.x;
				if (mesh->faces[upfaceid[i]].V0(j)->p.x < min_x)
					min_x = mesh->faces[upfaceid[i]].V0(j)->p.x;
				if (mesh->faces[upfaceid[i]].V0(j)->p.y > max_y)
					max_y = mesh->faces[upfaceid[i]].V0(j)->p.y;
				if (mesh->faces[upfaceid[i]].V0(j)->p.y < min_y)
					min_y = mesh->faces[upfaceid[i]].V0(j)->p.y;
			}
			int x_min_id =(min_x - begin_x) / length_x;
			int y_min_id = (min_y - begin_y) / length_y;
			int x_max_id = (max_x - begin_x) / length_x;
			int y_max_id = (max_y - begin_y) / length_y;

			if (x_max_id == width)
				x_max_id--;
			if (y_max_id == height)
				y_max_id--;

			for (int y = y_min_id; y <= y_max_id; y++)
				for (int x = x_min_id; x <= x_max_id; x++)
					mapind[x][y].push_back(upfaceid[i]);
		}

		for (int vi = 0; vi < compara_vertex.size(); vi++)
		{
			int xi = (mesh->vertices[compara_vertex[vi]].p.x - begin_x) / length_x;
			int yi = (mesh->vertices[compara_vertex[vi]].p.y - begin_y) / length_y;
			if (xi == width)
				xi--;
			if (yi == height)
				yi--;
			float min_z = std::numeric_limits<float>::max();
			for (int ii = 0; ii < mapind[xi][yi].size(); ii++)
			{
				for (int k = 0; k < 3; k++)
				{
					if (mesh->faces[mapind[xi][yi][ii]].V0(k)->p.z < min_z)
						min_z = mesh->faces[mapind[xi][yi][ii]].V0(k)->p.z;
				}
			}
			vertex_distance.push_back(std::make_pair(compara_vertex[vi], min_z));
		}


		/*for (int i = 0; i < vertexidx.size(); i++)
		{
			float x = mesh->vertices[vertexidx[i]].p.x - begin_x;
			float y = mesh->vertices[vertexidx[i]].p.y - begin_y;
			int x_ind = x / length_x;
			int y_ind = y / length_y;
			if (x_ind == width)
				x_ind--;
			if (y_ind == height)
				y_ind--;
			if (mesh->vertices[vertexidx[i]].p.z < mapp[x_ind][y_ind])
			{
				mapp[x_ind][y_ind] = mesh->vertices[vertexidx[i]].p.z;				
				mapind[x_ind][y_ind] = vertexidx[i];
			}
		}

		for (int vi = 0; vi < compara_vertex.size(); vi++)
		{
			int xi = (mesh->vertices[compara_vertex[vi]].p.x - begin_x) / length_x;
			int yi = (mesh->vertices[compara_vertex[vi]].p.y - begin_y) / length_y;
			if (mapp[xi][yi] != std::numeric_limits<float>::max())
			{				
				vertex_distance.push_back(std::make_pair(compara_vertex[vi], mapind[xi][yi]));
			}
			else {
				float max = std::numeric_limits<float>::max();
				int xi_x, yi_y;
				for (int i = 1; i < width; i++)
				{
					for (int ii = xi - i; ii <= xi + i; ii++)
						for (int jj = yi - i; jj <= yi + i; jj++)
						{
							if (std::abs(ii - xi) != i && std::abs(jj - yi) != i) continue;
							if (mapp[ii][jj] < max)
							{
								max = mapp[ii][jj];
								xi_x = ii; yi_y = jj;
							}
						}
					if (max != std::numeric_limits<float>::max())
					{						
						vertex_distance.push_back(std::make_pair(compara_vertex[vi], mapind[xi_x][yi_y]));
						break;
					}
				}
			}
		}*/
	}

	void findNeighVertex(trimesh::TriMesh* mesh, const std::vector<int>& upfaceid,const std::vector<int>& botfaceid, std::vector<std::pair<int, int>>& vertex_distance)
	{
		std::vector<int> vertexidx;
		std::vector<int> compara_vertex;
		for (int i = 0; i < upfaceid.size(); i++)
		{
			vertexidx.push_back(mesh->faces[upfaceid[i]][0]);
			vertexidx.push_back(mesh->faces[upfaceid[i]][1]);
			vertexidx.push_back(mesh->faces[upfaceid[i]][2]);
		}
		std::sort(vertexidx.begin(), vertexidx.end());
		std::vector<int>::iterator itr = std::unique(vertexidx.begin(), vertexidx.end());
		vertexidx.resize(std::distance(vertexidx.begin(), itr));

		for (int i = 0; i < botfaceid.size(); i++)
		{
			compara_vertex.push_back(mesh->faces[botfaceid[i]][0]);
			compara_vertex.push_back(mesh->faces[botfaceid[i]][1]);
			compara_vertex.push_back(mesh->faces[botfaceid[i]][2]);
		}
		std::sort(compara_vertex.begin(), compara_vertex.end());
		std::vector<int>::iterator it = std::unique(compara_vertex.begin(), compara_vertex.end());
		compara_vertex.resize(std::distance(compara_vertex.begin(), it));
#if 0
		const int K = 10;
		std::list<Point> points;
		for (int i = 0; i < vertexidx.size(); i++)
		{
			points.push_back(Point(mesh->vertices[vertexidx[i]].x, mesh->vertices[vertexidx[i]].y));
		}
		Tree kdtree(points.begin(), points.end());
		Point query(0, 0);
		Neighbor_search search(kdtree, query, K);
		for (Neighbor_search::iterator it = search.begin(); it != search.end(); it++)
			std::cout << "point :" << it->first << " distance^2 : " << it->second << "\n";
#else
		const int width = 100, height = 100;
		std::vector<std::vector<float>> mapp(width, std::vector<float>(height, std::numeric_limits<float>::min()));
		std::vector<std::vector<int>> mapind(width, std::vector<int>(height, -1));
		mesh->need_bbox();
		float length_x = (mesh->bbox.max.x - mesh->bbox.min.x) / (width * 1.0f);
		float length_y = (mesh->bbox.max.y - mesh->bbox.min.y) / (height * 1.0f);

		float begin_x = mesh->bbox.min.x;
		float begin_y = mesh->bbox.min.y;

		for (int i = 0; i < vertexidx.size(); i++)
		{
			float x = mesh->vertices[vertexidx[i]].x - begin_x;
			float y = mesh->vertices[vertexidx[i]].y - begin_y;
			int x_ind = x / length_x;
			int y_ind = y / length_y;
			if (x_ind == width)
				x_ind--;
			if (y_ind == height)
				y_ind--;
			if (mesh->vertices[vertexidx[i]].z > mapp[x_ind][y_ind])
			{
				mapp[x_ind][y_ind] = mesh->vertices[vertexidx[i]].z;
				//std::cout << " x_ind :" << x_ind << " y_ind :" << y_ind << " z :" << mesh->vertices[vertexidx[i]].z << "\n";
				mapind[x_ind][y_ind] = vertexidx[i];
			}
		}

		for (int vi = 0; vi < compara_vertex.size(); vi++)
		{
			int xi = (mesh->vertices[compara_vertex[vi]].x - begin_x) / length_x;
			int yi = (mesh->vertices[compara_vertex[vi]].y - begin_y) / length_y;
			if (mapp[xi][yi] != std::numeric_limits<float>::min())
			{
				//std::cout << "mapind :" << mapind[xi][yi] << " z : " << mapp[xi][yi] << "\n";
				vertex_distance.push_back(std::make_pair(compara_vertex[vi], mapind[xi][yi]));
			}
			else {
				float max = std::numeric_limits<float>::min();
				int xi_x, yi_y;
				for (int i = 1; i < width; i++)
				{
					for (int ii = xi - i; ii <= xi + i; ii++)
						for (int jj = yi - i; jj <= yi + i; jj++)
						{
							if (std::abs(ii - xi) != i && std::abs(jj - yi) != i) continue;
							if (mapp[ii][jj] > max)
							{
								max = mapp[ii][jj];
								xi_x = ii; yi_y = jj;
							}
						}
					if (max != std::numeric_limits<float>::min())
					{
						//std::cout << "mapind :" << mapind[xi_x][yi_y] << " z : " << max << "\n";
						vertex_distance.push_back(std::make_pair(compara_vertex[vi], mapind[xi_x][yi_y]));
						break;
					}
				}
			}
		}		
		
#endif
	}

	void innerHex(MMeshT* mesh, std::vector<std::vector<trimesh::vec2>>& poly, std::vector<int>& inFace, std::vector<int>& outFace, float len)
	{
		std::vector<trimesh::vec2> center(poly.size());
		trimesh::vec2 start(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		trimesh::vec2 top(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
		for (int i = 0; i < poly.size(); i++)
		{
			trimesh::vec2 v(0,0);
			for (int j = 0; j < poly[i].size(); j++)
			{
				v += poly[i][j];
				if (poly[i][j].x < start.x)
					start.x = poly[i][j].x;
				if (poly[i][j].y < start.y)
					start.y = poly[i][j].y;
				if (poly[i][j].x > top.x)
					top.x = poly[i][j].x;
				if (poly[i][j].y > top.y)
					top.y = poly[i][j].y;
			}
			v = v / poly[i].size() * 1.0f;
			center[i] = v;
		}
		int width = (top.x - start.x) / len + 1;
		int height = (top.y - start.y) / (std::sqrt(3) * len) + 1;

		std::vector<std::vector<std::pair<int,int>>> block(center.size());
		for (int i = 0; i < center.size(); i++)
		{
			trimesh::vec2 p = trimesh::vec2(center[i].x + 0.1 * len, center[i].y + 0.1*len) - start;
			int x = p.x / len;
			int y = p.y / (std::sqrt(3) * len);
			block[i].push_back(std::make_pair(x,y));
			block[i].push_back(std::make_pair(x + 1, y)); 
			block[i].push_back(std::make_pair(x - 1, y)); 
			block[i].push_back(std::make_pair(x - 2, y)); 
			block[i].push_back(std::make_pair(x, y - 1)); 
			block[i].push_back(std::make_pair(x + 1, y - 1)); 
			block[i].push_back(std::make_pair(x - 1, y - 1)); 
			block[i].push_back(std::make_pair(x - 2, y - 1)); 
		}
		std::vector<std::vector<std::vector<int>>> faceBlock(width,std::vector<std::vector<int>>(height));
		for (int fi = 0; fi < inFace.size(); fi++)
		{
			MMeshFace& f = mesh->faces[inFace[fi]];
			trimesh::point c = (f.V0(0)->p + f.V0(1)->p + f.V0(2)->p) / 3.0;
			int xi = (c.x - start.x) / len;
			int yi = (c.y - start.y) / (std::sqrt(3) * len);
			faceBlock[xi][yi].push_back(fi);
		}
		std::vector<std::vector<int>> polyInnerFaces(block.size());
		for (int i = 0; i < block.size(); i++)
		{
			for (int j = 0; j < block[i].size(); j++)
			{
				polyInnerFaces[i].insert(polyInnerFaces[i].end(), faceBlock[block[i][j].first][block[i][j].second].begin(), faceBlock[block[i][j].first][block[i][j].second].end());
			}
		}

		for (int i = 0; i < polyInnerFaces.size(); i++)
		{
			for (int j = 0; j < polyInnerFaces[i].size(); j++)
			{
				int leftCross = 0;int rightCross = 0;
				MMeshFace& f = mesh->faces[polyInnerFaces[i][j]];
				trimesh::point c= (f.V0(0)->p + f.V0(1)->p + f.V0(2)->p) / 3.0;
				for (int k = 0; k < poly[i].size(); k++)
				{
					if (std::abs(poly[i][k].y - poly[i][(k + 1) % poly[i].size()].y) < EPS) continue;
					if (c.y < std::min(poly[i][k].y, poly[i][(k + 1) % poly[i].size()].y)) continue;
					if (c.y > std::max(poly[i][k].y, poly[i][(k + 1) % poly[i].size()].y)) continue;
					double x = (c.y - poly[i][k].y) * (poly[i][(k + 1) % poly[i].size()].x - poly[i][k].x) / (poly[i][(k + 1) % poly[i].size()].y - poly[i][k].y) + poly[i][k].x;
					if (x - c.x <= 0)
						rightCross++;
					else if (x - c.x >= 0)
						leftCross++;
				}
				if (leftCross > 0 && rightCross > 0)
					outFace.push_back(polyInnerFaces[i][j]);
			}
		}

	}
}