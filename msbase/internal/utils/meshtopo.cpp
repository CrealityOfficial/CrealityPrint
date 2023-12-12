#include "msbase/utils/meshtopo.h"
#include "msbase/space/intersect.h"

using namespace trimesh;
namespace msbase
{
	static constexpr float EPSILON = 1e-4;

	MeshTopo::MeshTopo()
		:m_topoBuilded(false)
		, m_mesh(nullptr)
	{
	}

	MeshTopo::~MeshTopo()
	{
	}

	bool MeshTopo::builded()
	{
		return m_topoBuilded;
	}

	void MeshTopo::build(trimesh::TriMesh* mesh)
	{
		m_mesh = mesh;
		if (m_mesh && !m_topoBuilded)
		{
			int vertexNum = (int)m_mesh->vertices.size();
			int faceNum = (int)m_mesh->faces.size();
			//build topo
			if (vertexNum) m_outHalfEdges.resize(vertexNum);
			if (faceNum) m_oppositeHalfEdges.resize(faceNum, ivec3(-1, -1, -1));
			for (int i = 0; i < faceNum; ++i)
			{
				TriMesh::Face& f = m_mesh->faces.at(i);
				bool old[3] = { false };
				for (int j = 0; j < 3; ++j)
				{
					std::vector<int>& outHalfs = m_outHalfEdges.at(f[j]);
					old[j] = outHalfs.size() > 0;
				}

				ivec3& halfnew = m_oppositeHalfEdges.at(i);
				for (int j = 0; j < 3; ++j)
				{
					int start = j;
					int end = (j + 1) % 3;

					int v2 = f[start];
					int v1 = f[end];

					std::vector<int>& halfs1 = m_outHalfEdges.at(v1);

					int nh = halfcode(i, j);
					if (old[start] && old[end])//两个相邻顶点都有向外的半边数据
					{
						int hsize = (int)halfs1.size();
						for (int k = 0; k < hsize; ++k)
						{
							int h = halfs1.at(k);
							int ev = endvertexid(h);
							if (ev == v2)//如果半边数据未点与V2相等
							{
								halfnew.at(j) = h;

								int faceid = 0;
								int idxid = 0;
								halfdecode(h, faceid, idxid);
								m_oppositeHalfEdges.at(faceid)[idxid] = nh;
								break;
							}
						}
					}
				}

				for (int j = 0; j < 3; ++j)
				{
					std::vector<int>& outHalfs = m_outHalfEdges.at(f[j]);
					outHalfs.push_back(halfcode(i, j));
				}
			}

			m_topoBuilded = true;
		}
	}

	void MeshTopo::lowestVertex(std::vector<trimesh::vec3>& vertexes, std::vector<int>& indices)
	{
		int vertexNum = vertexes.size();
		for (int vertexID = 0; vertexID < vertexNum; ++vertexID)
		{
			std::vector<int>& vertexHalfs = m_outHalfEdges.at(vertexID);
			int halfSize = (int)vertexHalfs.size();
			vec3& vertex = vertexes.at(vertexID);

			bool lowest = true;
			for (int halfIndex = 0; halfIndex < halfSize; ++halfIndex)
			{
				int endVertexID = endvertexid(vertexHalfs.at(halfIndex));
				if (vertexes.at(endVertexID).z <= vertex.z)
				{
					lowest = false;
					break;
				}
			}
			if (lowest)
			{
				indices.push_back(vertexID);
			}
		}
	}

	void MeshTopo::hangEdgeCloud(std::vector<trimesh::vec3>& vertexes, std::vector<trimesh::vec3>& normals, std::vector<float>& dotValues, float faceCosValue, std::vector<trimesh::ivec2>& supportEdges)
	{
		int faceNum = (int)m_mesh->faces.size();
		std::vector<ivec3> edgesFlags(faceNum, ivec3(0, 0, 0));
		float edgeCosValue = cosf(M_PIf * 70.0f / 180.0f);
		float edgeFaceCosValue = cosf(M_PIf * 60.0f / 180.0f);
		for (int faceID = 0; faceID < faceNum; ++faceID)
		{
			ivec3& oppoHalfs = m_oppositeHalfEdges.at(faceID);
			ivec3& edgeFlag = edgesFlags.at(faceID);
			TriMesh::Face& tFace = m_mesh->faces.at(faceID);
			vec3& faceNormal = normals.at(faceID);

			bool faceSupport = dotValues.at(faceID) < (-edgeFaceCosValue);
			for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
			{
				if (edgeFlag[edgeIndex] == 0)
				{
					edgeFlag[edgeIndex] = 1;

					int vertexID1 = tFace[edgeIndex];
					int vertexID2 = tFace[(edgeIndex + 1) % 3];
					vec3 edge = vertexes.at(vertexID1) - vertexes.at(vertexID2);
					vec3 nedge = normalized(edge);

					if (abs(trimesh::dot(nedge, vec3(0.0f, 0.0f, 1.0f))) < edgeCosValue)
					{
						int oppoHalf = oppoHalfs.at(edgeIndex);
						bool shouldAdd = false;
						if (oppoHalf >= 0)
						{
							int oppoFaceID;
							int edgeID;
							halfdecode(oppoHalf, oppoFaceID, edgeID);
							edgesFlags.at(oppoFaceID)[edgeID] = 1;

							vec3& oppoFaceNormal = normals.at(oppoFaceID);
							bool oppoFaceSupport = dotValues.at(oppoFaceID) < (-edgeFaceCosValue);
							if (oppoFaceSupport && faceSupport)
							{

								if (trimesh::dot(faceNormal, oppoFaceNormal) < 0.0f)
								{
									shouldAdd = true;
								}
							}
						}
						else  // hole edge
						{
							shouldAdd = faceSupport;
						}

						if (shouldAdd)
						{
							ivec2 edgeID(vertexID1, vertexID2);
							supportEdges.push_back(edgeID);
						}
					}
				}
			}
		}
	}

	void MeshTopo::hangEdge(std::vector<trimesh::vec3>& vertexes, std::vector<trimesh::vec3>& normals, std::vector<float>& dotValues, float faceCosValue, std::vector<trimesh::ivec2>& supportEdges)
	{
		auto middlePt = [vertexes](int vertexID1, int vertexID2, int vertexID3, vec3 &middlept)->bool {
#if 0
			vec3 A(1, 1, 1);
			vec3 B(3, 1, 1);
			vec3 C(3, 3, 1);
#else
			vec3 A = vertexes.at(vertexID1);
			vec3 B = vertexes.at(vertexID2);
			vec3 C = vertexes.at(vertexID3);
#endif
			bool ret = true;
			vec3 E = (A + B) / 2;
			vec3 abN = (B - A);
			vec3 baN = (A - B);

			float t = -1;
			if (rayIntersectPlane(A, (C - A), E, abN, t)&&((t>0)&& (t <= 1)))
			{
				 middlept = A * (1 - t) + C * t;
			}
			else
			{
				if (rayIntersectPlane(B, (C - B), E, baN, t) && ((t > 0) && (t <= 1)))
				{
					middlept = B * (1 - t) + C * t;
				}
				else
				{
					ret = false;
				}
			}




			//vec3 G = A * (1 - t) + C * t;
			//vec3 GE =(E-G)= (A - B) / 2+(C-A)*t;
			//vec3 BA=(A-B);
			//GE* BA = 0;//x1*x2+y1*y2+z1*z2=0

			//vec3 GEtemp = (A - B) / 2;
			//vec3 CAtemp = (C - A);
			//vec3 BA = (A - B);
			//float xtemp = BA.x * BA.x / 2;
			//float ytemp = BA.y * BA.y / 2;
			//float ztemp = BA.z * GEtemp.z / 2;
			//float xtemp_t = BA.x * CAtemp.x;
			//float ytemp_t = BA.y * CAtemp.y;
			//float ztemp_t = BA.z * CAtemp.z;
			//float t = -(xtemp + ytemp + ztemp) / (xtemp_t + ytemp_t + ztemp_t);
			//vec3 middlept = A * (1 - t) + C * t;
			return ret;
		};

		int faceNum = (int)m_mesh->faces.size();
		std::vector<ivec3> edgesFlags(faceNum, ivec3(0, 0, 0));
		float edgeCosValue = faceCosValue;//cosf(M_PIf * 45.0f / 180.0f);
		float thresAngle = acosf(faceCosValue) * 180.0 / M_PIf;
		float faceThresCosValue = 180.0 - thresAngle;
		for (int faceID = 0; faceID < faceNum; ++faceID)
		{
			ivec3& oppoHalfs = m_oppositeHalfEdges.at(faceID);
			TriMesh::Face& tFace = m_mesh->faces.at(faceID);
			ivec3& edgeFlag = edgesFlags.at(faceID);

			for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++)
			{
				if (edgeFlag[edgeIndex] == 0)
				{
					edgeFlag[edgeIndex] = 1;//标示该边已检测处理过
					int vertexID1 = tFace[edgeIndex % 3];
					int vertexID2 = tFace[(edgeIndex + 1) % 3];
					int vertexID3 = tFace[(edgeIndex + 2) % 3];
					vec3 edge = vertexes.at(vertexID1) - vertexes.at(vertexID2);
					if (trimesh::length(edge) < EPSILON)
						continue;
					vec3 nedge = normalized(edge);
					vec3 nedgeXY= normalized(vec3(nedge.x, nedge.y, 0.0f));

					//vec3 nedge12 = vec3(0, 0, 1.0f);
					//vec3 nedgeXY12= normalized(vec3(nedge.x, nedge.y, 0.0f));
					//float testangle = trimesh::dot(nedge12, nedgeXY12);
					if (abs(trimesh::dot(nedge, nedgeXY)) > edgeCosValue)//悬吊线与XY平面夹角小于某一角度
					{
						int oppoHalf = -1;
						bool shouldAdd = false;
						int oppoFaceID=-1;
						int oppoedgeIndex =-1;
						int oppoEdgeVertexID=-1;
						int connectfaceVetexn = 0;
						for (int offedgeIndex = 0; offedgeIndex < 3; offedgeIndex++)
						{
							int oppoFaceIDtemp;
							int oppoEdgeVertexIDtemp;
							oppoHalf = oppoHalfs.at(offedgeIndex);
							if (oppoHalf < 0)
							{
								continue;
							}

							halfdecode(oppoHalf, oppoFaceIDtemp, oppoEdgeVertexIDtemp);
							oppoEdgeVertexIDtemp = startvertexid(oppoHalf);

							if (oppoEdgeVertexIDtemp == vertexID2)//相邻面
							{
								oppoFaceID = oppoFaceIDtemp;
								oppoEdgeVertexID = oppoEdgeVertexIDtemp;
								oppoedgeIndex = offedgeIndex;
								connectfaceVetexn += 1;
								//break;
							}

						}
						if (connectfaceVetexn > 1)
						{
						}
						if (oppoHalf >= 0)
						{
							if (oppoEdgeVertexID == vertexID2)//相邻面
							{
								if (oppoFaceID >= 0 || oppoedgeIndex >= 0)
								{
									ivec3& edgeOppoFlag = edgesFlags.at(oppoFaceID);
									edgeOppoFlag[oppoedgeIndex] = 1;//标示该边已检测处理过
								}

								//bool oppoFaceSupport = dotValues.at(oppoFaceID) < (-edgeFaceCosValue);
								//bool faceSupport = dotValues.at(faceID) < (-edgeFaceCosValue);
								//if ((oppoFaceSupport == false) && (faceSupport == false))//相邻非支撑面
								TriMesh::Face& oppoFace = m_mesh->faces.at(oppoFaceID);
								int oppovertexID3 = -1;
								if (oppoFace[0] == oppoEdgeVertexID)
									oppovertexID3 = oppoFace[2];
								else if (oppoFace[1] == oppoEdgeVertexID)
									oppovertexID3 = oppoFace[0];
								else if (oppoFace[2] == oppoEdgeVertexID)
									oppovertexID3 = oppoFace[1];
								vec3 G,H;
								if (middlePt(vertexID1, vertexID2, vertexID3, G) == false
									|| middlePt(vertexID1, vertexID2, oppovertexID3, H) == false
									)
								{
									continue;
								}
								vec3 E = (vertexes.at(vertexID1) + vertexes.at(vertexID2)) / 2;
								if ((G.z - E.z > 0.0) && (H.z - E.z > 0.0))
								{
									vec3& faceNormal = normals.at(faceID);
									vec3& oppoFaceNormal = normals.at(oppoFaceID);
									if (dotValues.at(faceID) < 0.0 || dotValues.at(oppoFaceID) < 0.0)//至少有一个三角面法向量与Z轴负方向的夹角为90度
									{
										const float& faceNormal_dot = dotValues.at(faceID);
										const float& oppoFaceNormal_dot = dotValues.at(oppoFaceID);
										float faceCosValue = acosf(dotValues.at(faceID)) * 180.0 / M_PIf;
										float oppofaceCosValue = acosf(dotValues.at(oppoFaceID)) * 180.0 / M_PIf;
										bool faceThresCosflg = (faceCosValue - faceThresCosValue) > EPSILON || abs(faceThresCosValue - faceCosValue) < EPSILON;
										bool oppofaceThresCosflg = (oppofaceCosValue - faceThresCosValue) > EPSILON || abs(oppofaceCosValue - faceCosValue) < EPSILON;
										vec3 faceNormalAdd = faceNormal + oppoFaceNormal;

										if ((trimesh::dot(trimesh::normalized(faceNormalAdd), vec3(0.0f, 0.0f, -1.0f)) > 0.0))
										{
											//if ((faceThresCosflg == false) && (oppofaceThresCosflg == false))
											{
												if (faceNormal_dot * oppoFaceNormal_dot < 0.0f)//两个面法向量一个向上，一个向下，法向量向下的面应当不是需要加支撑的面，不然单边都可以自支撑起来
												{
													if (dotValues.at(faceID) < 0.0)
													{
														float faceAngle = 180.0 - faceCosValue;
														float oppoFaceAngle = oppofaceCosValue;
														float faceAngle1 = 90.0 - faceAngle;
														float oppoFaceAngle1 = 90.0 - oppoFaceAngle;

														if (faceAngle1 > oppoFaceAngle1)//面法向量向下的面与向上向量的夹角大于面法向量向上的面与向上向量的夹角
														{
															shouldAdd = true;
														}
													}
													else
													{
														float faceAngle = faceCosValue;
														float oppoFaceAngle = 180.0 - oppofaceCosValue;
														float faceAngle1 = 90.0 - faceAngle;
														float oppoFaceAngle1 = 90.0 - oppoFaceAngle;

														if (faceAngle1 < oppoFaceAngle1)
														{
															shouldAdd = true;
														}

													}
												}
												else//两个面法向量同时向下
												{
													float faceAngle = faceCosValue;
													float oppoFaceAngle = oppofaceCosValue;
													if (((faceAngle+ oppoFaceAngle-180.0) > EPSILON) &&
														((faceAngle + oppoFaceAngle - 180.0) < 180.0)&&
														(std::abs(faceAngle - oppoFaceAngle) > EPSILON)
														)//只要不是共面
													{

														shouldAdd = true;
													}
												}
											}
										}
										if(0)//两个面法向量同时向下
										{
											float tempdot = trimesh::dot(faceNormal, oppoFaceNormal);
											float tempdotAngle = acos(tempdot) * 180.0 / M_PIf;
											if (tempdotAngle > EPSILON)//只要不是共面
											{
												vec3 faceNormalAdd = faceNormal + oppoFaceNormal;

												{
													typedef struct VERTEX_INFOR
													{
														vec3 pos;
														int vertexID;
													} vertexInfor;
													auto cmp_elements_sort = [](const vertexInfor& e1, const vertexInfor& e2) -> bool {
														return e1.pos.z < e2.pos.z;

													};

													std::vector<vertexInfor> vertexPos;
													std::vector<vertexInfor> oppovertexPos;
													for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
													{
														vertexInfor vertexinfor;
														TriMesh::Face& tFace = m_mesh->faces.at(faceID);
														TriMesh::Face& toppoFace = m_mesh->faces.at(oppoFaceID);
														int vertexID = tFace[edgeIndex];
														vertexinfor.pos = vertexes.at(vertexID);
														vertexinfor.vertexID = vertexID;
														vertexPos.emplace_back(vertexinfor);
														vertexID = toppoFace[edgeIndex];
														vertexinfor.pos = vertexes.at(vertexID);
														vertexinfor.vertexID = vertexID;
														oppovertexPos.emplace_back(vertexinfor);
													}
													std::sort(vertexPos.begin(), vertexPos.end(), cmp_elements_sort);
													std::sort(oppovertexPos.begin(), oppovertexPos.end(), cmp_elements_sort);
													//vertexInfor& lowestvertexinfor = vertexPos.at(0);
													//vertexInfor& lowestoppovertexinfor = oppovertexPos.at(0);
													//if ((lowestvertexinfor.vertexID == vertexID1) && (lowestoppovertexinfor.vertexID == vertexID2) || (lowestvertexinfor.vertexID == vertexID2) && (lowestoppovertexinfor.vertexID == vertexID1))
													vertexInfor& lowestvertexinfor = vertexPos.at(2);
													vertexInfor& lowestoppovertexinfor = oppovertexPos.at(2);
													if ((lowestvertexinfor.vertexID != vertexID1) && (lowestvertexinfor.vertexID != vertexID2) && (lowestoppovertexinfor.vertexID != vertexID2) && (lowestoppovertexinfor.vertexID != vertexID1))
													{
														shouldAdd = true;
													}
													if (tempdotAngle < 0.1)//排除已是支撑双面
													{
														if ((faceThresCosflg == true) && (oppofaceThresCosflg == true))
															shouldAdd = false;
													}


												}
											}
										}


									}
								}
								else if ((G.z - E.z >= 0.0) && (H.z - E.z >= 0.0))
								{
									//两个面法向量同时向下,且有一个是水平面
									if(0)
									{
										float faceCosValue = acosf(dotValues.at(faceID)) * 180.0 / M_PIf;
										float oppofaceCosValue = acosf(dotValues.at(oppoFaceID)) * 180.0 / M_PIf;

										float faceAngle = faceCosValue;
										float oppoFaceAngle = oppofaceCosValue;
										if (((faceAngle + oppoFaceAngle - 180.0) > EPSILON) &&
											((faceAngle + oppoFaceAngle - 180.0) < 180.0)&&
											(std::abs(faceAngle - oppoFaceAngle)> 1.0)
											)//只要不是共面
										{

											shouldAdd = true;
										}
									}

								}

							}


						}
						else  // hole edge
						{
							shouldAdd = true;
						}

						if (shouldAdd)
						{
							ivec2 edgeID(vertexID1, vertexID2);
							supportEdges.push_back(edgeID);
						}
					}
				}
			}
		}
	}

	void MeshTopo::chunkFace(std::vector<float>& dotValues, std::vector<std::vector<int>>& supportFaces, float faceCosValue)
	{
		int faceNum = (int)m_mesh->faces.size();
		std::vector<bool> visitFlags(faceNum, false);
		std::vector<int> visitStack;
		std::vector<int> nextStack;
		float thresAngle = acosf(faceCosValue) * 180.0 / M_PIf;
		float faceThresCosValue = 180.0 - thresAngle;
		for (int faceID = 0; faceID < faceNum; ++faceID)
		{
			float faceCosValue = acosf(dotValues.at(faceID)) * 180.0 / M_PIf;

			//if (faceCosValue - faceThresCosValue > EPSILON)
			bool faceThresCosflg = (faceCosValue - faceThresCosValue) > EPSILON || abs(faceThresCosValue - faceCosValue) < EPSILON;
			//if (((dotValues.at(faceID)- EPSILON) < -faceCosValue) && (visitFlags.at(faceID) == false))
			if ((faceThresCosflg == true) && (visitFlags.at(faceID) == false))
			{
				visitFlags.at(faceID) = true;
				visitStack.push_back(faceID);

				std::vector<int> facesChunk;
				facesChunk.push_back(faceID);
				while (!visitStack.empty())
				{
					int seedSize = (int)visitStack.size();
					for (int seedIndex = 0; seedIndex < seedSize; ++seedIndex)
					{
						int cFaceID = visitStack.at(seedIndex);
						ivec3& oppoHalfs = m_oppositeHalfEdges.at(cFaceID);
						for (int halfID = 0; halfID < 3; ++halfID)
						{
							int oppoHalf = oppoHalfs.at(halfID);
							if (oppoHalf >= 0)
							{
								int oppoFaceID = faceid(oppoHalf);
								float oppofaceCosValue = acosf(dotValues.at(oppoFaceID)) * 180.0 / M_PIf;

								faceThresCosflg = (oppofaceCosValue - faceThresCosValue) > EPSILON || abs(faceThresCosValue - oppofaceCosValue) < EPSILON;
								if ((faceThresCosflg == true) && (visitFlags.at(oppoFaceID) == false))
								{
									nextStack.push_back(oppoFaceID);
									facesChunk.push_back(oppoFaceID);
									visitFlags.at(oppoFaceID) = true;
								}
								else
								{
									visitFlags.at(oppoFaceID) = true;
								}
							}
						}
					}

					visitStack.swap(nextStack);
					nextStack.clear();
				}

				supportFaces.push_back(facesChunk);
			}
			else
			{
				visitFlags.at(faceID) = true;
			}
		}
	}

	void MeshTopo::flipFace(std::vector<int>& facesChunk)
	{
		int faceNum = (int)m_mesh->faces.size();
		std::vector<bool> visitFlags(faceNum, false);
		std::vector<int> visitStack;
		std::vector<int> nextStack;

		for (int faceID = 0; faceID < faceNum; ++faceID)
		{
			if (visitFlags.at(faceID) == false)
			{
				visitFlags.at(faceID) = true;
				visitStack.push_back(faceID);

				while (!visitStack.empty())
				{
					int seedSize = (int)visitStack.size();
					for (int seedIndex = 0; seedIndex < seedSize; ++seedIndex)
					{
						int cFaceID = visitStack.at(seedIndex);
						ivec3& oppoHalfs = m_oppositeHalfEdges.at(cFaceID);
						for (int halfID = 0; halfID < 3; ++halfID)
						{
							int cStartVertex = m_mesh->faces.at(cFaceID)[halfID];
							int oppoHalf = oppoHalfs.at(halfID);
							if (oppoHalf >= 0)
							{
								int oppoFaceID = faceid(oppoHalf);
								if (visitFlags.at(oppoFaceID) == false)
								{
									int oStartVertex = startvertexid(oppoHalf);
									nextStack.push_back(oppoFaceID);
									if(oStartVertex == cStartVertex)
										facesChunk.push_back(oppoFaceID);
									visitFlags.at(oppoFaceID) = true;
								}
								else
								{
									visitFlags.at(oppoFaceID) = true;
								}
							}
						}
					}

					visitStack.swap(nextStack);
					nextStack.clear();
				}
			}
		}
	}
}