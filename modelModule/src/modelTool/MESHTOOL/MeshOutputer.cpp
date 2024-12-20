#include "MESHTOOL/MeshOutputer.hpp"
#include "MESHTOOL/UnitConverter.hpp"
#include <fstream>

namespace MESHTOOL {

	MeshOutputer::MeshOutputer() {}

	bool MeshOutputer::writeSTLPlain(const MESH::MeshPtr& mesh, const std::string& filename)
	{
		return true;
	}

	bool MeshOutputer::writeSTLBinary(const MESH::MeshPtr& mesh, const std::string& filename)
	{
		UnitConverter converter;
		unsigned int faceNum = mesh->faceCount();
		std::vector<MESH::VertexPtr> vertices;
		std::vector<std::array<unsigned int, 3>> faceIndices;
		mesh->getverticesAndFaceIndex(vertices, faceIndices);
		std::vector<MESH::FacePtr> faces = mesh->getFaces();
		std::vector<GEOM::Vector> normals;
		normals.reserve(faceNum);
		std::vector< std::array<std::array<float, 3>, 3>> faceCoords;
		faceCoords.reserve(faceNum);
		for (auto& face : faces)
		{
			// 法向数据
			GEOM::Vector normal = face->normal();
			normals.emplace_back(normal);
		}
		for (auto& faceIndex: faceIndices) {
			// 顶点数据
			GEOM::Point p0 = vertices.at(faceIndex.at(0))->getPoint();
			GEOM::Point p1 = vertices.at(faceIndex.at(1))->getPoint();
			GEOM::Point p2 = vertices.at(faceIndex.at(2))->getPoint();
			std::array<float, 3> pCoord0 = converter.toOutside(p0);
			std::array<float, 3> pCoord1 = converter.toOutside(p1);
			std::array<float, 3> pCoord2 = converter.toOutside(p2);
			std::array<std::array<float, 3>, 3> faceCoord = { pCoord0 , pCoord1 ,pCoord2 };
			faceCoords.emplace_back(faceCoord);
		}
		return writeSTLBinaryTool(normals, faceCoords, filename);
	}

	bool MeshOutputer::writeSTLBinary(const MESH::SimpleMeshPtr& mesh, const std::string& filename)
	{
		UnitConverter converter;
		unsigned int faceNum = mesh->faceCount();
		std::vector<GEOM::Vector> normals = mesh->getNormals();
		std::vector< std::array<std::array<float, 3>, 3>> faceCoords;
		faceCoords.reserve(faceNum);
		auto points = mesh->getPoints();
		for (auto& face: mesh->getFaces()) {
			std::array<float, 3> pCoord0 = converter.toOutside(points.at(face.at(0)));
			std::array<float, 3> pCoord1 = converter.toOutside(points.at(face.at(1)));
			std::array<float, 3> pCoord2 = converter.toOutside(points.at(face.at(2)));
			std::array<std::array<float, 3>, 3> faceCoord = { pCoord0 , pCoord1 ,pCoord2 };
			faceCoords.emplace_back(faceCoord);
		}
		return writeSTLBinaryTool(normals, faceCoords, filename);
	}


	bool MeshOutputer::writeOFF(const MESH::MeshPtr& mesh, const std::string& filename)
	{
		unsigned int vertexCount = mesh->vertexCount();
		unsigned int faceCount = mesh->faceCount();
		std::ofstream fs(filename, std::ios::binary | std::ios::trunc);
		if (!fs) { fs.close(); return false; }
		fs << "OFF" << std::endl;
		fs << vertexCount << " " << faceCount << " 0" << std::endl;
		std::vector<MESH::VertexPtr> vertices;
		std::vector<std::array<unsigned int, 3>> faceIndices;
		mesh->getverticesAndFaceIndex(vertices, faceIndices);
		UnitConverter converter;
		for (auto &vertex: vertices)
		{
			auto pFloat = converter.toOutside(vertex->getPoint());
			fs << pFloat.at(0) << " " << pFloat.at(1) << " " << pFloat.at(2) << std::endl;
		}
		for (auto& faceIndex : faceIndices)
		{
			fs << "3 " << faceIndex.at(0) << " " << faceIndex.at(1) << " " << faceIndex.at(2) << std::endl;
		}
		return true;
	}

	bool MeshOutputer::writeOFF(const MESH::SimpleMeshPtr& mesh, const std::string& filename)
	{
		unsigned int vertexCount = mesh->pointCount();
		unsigned int faceCount = mesh->faceCount();
		std::ofstream fs(filename, std::ios::binary | std::ios::trunc);
		if (!fs) { fs.close(); return false; }
		fs << "OFF" << std::endl;
		fs << vertexCount << " " << faceCount << " 0" << std::endl;
		UnitConverter converter;
		for (auto& point : mesh->getPoints())
		{
			auto pFloat = converter.toOutside(point);
			fs << pFloat.at(0) << " " << pFloat.at(1) << " " << pFloat.at(2) << std::endl;
		}
		for (auto& faceIndex : mesh->getFaces())
		{
			fs << "3 " << faceIndex.at(0) << " " << faceIndex.at(1) << " " << faceIndex.at(2) << std::endl;
		}
		return true;
	}

	bool MeshOutputer::writeSTLBinaryTool(const std::vector<GEOM::Vector>& normals, const std::vector< std::array<std::array<float, 3>, 3>>& faceCoords, const std::string& filename) const
	{
		unsigned int faceNum = normals.size();

		std::ofstream fs(filename, std::ios::binary | std::ios::trunc);
		if (!fs) { fs.close(); return false; }

		int intSize = sizeof(int);
		unsigned int unIntSize = sizeof(unsigned int);
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
		fs.write((char*)(&faceNum), unIntSize);
		// 
		char a[2];
		int a_size = sizeof(char) * 2;
		for (int i = 0; i < faceNum; ++i) {
			// 法向数据
			GEOM::Vector normal = normals.at(i);
			float nx = static_cast<float>(normal.x);
			float ny = static_cast<float>(normal.y);
			float nz = static_cast<float>(normal.z);
			fs.write((char*)(&nx), floatSize);
			fs.write((char*)(&ny), floatSize);
			fs.write((char*)(&nz), floatSize);
			// 顶点数据
			std::array<float, 3> pCoordiante0 = faceCoords.at(i).at(0);
			std::array<float, 3> pCoordiante1 = faceCoords.at(i).at(1);
			std::array<float, 3> pCoordiante2 = faceCoords.at(i).at(2);

			float p0x = pCoordiante0.at(0);
			float p0y = pCoordiante0.at(1);
			float p0z = pCoordiante0.at(2);
			fs.write((char*)(&p0x), floatSize);
			fs.write((char*)(&p0y), floatSize);
			fs.write((char*)(&p0z), floatSize);

			float p1x = pCoordiante1.at(0);
			float p1y = pCoordiante1.at(1);
			float p1z = pCoordiante1.at(2);
			fs.write((char*)(&p1x), floatSize);
			fs.write((char*)(&p1y), floatSize);
			fs.write((char*)(&p1z), floatSize);

			float p2x = pCoordiante2.at(0);
			float p2y = pCoordiante2.at(1);
			float p2z = pCoordiante2.at(2);
			fs.write((char*)(&p2x), floatSize);
			fs.write((char*)(&p2y), floatSize);
			fs.write((char*)(&p2z), floatSize);
			fs.write(a, a_size);
		}
		fs.close();
		return true;
	}

}  // namespace MESHTOOL