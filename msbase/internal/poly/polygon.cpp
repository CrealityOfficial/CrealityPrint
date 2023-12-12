#include "polygon.h"
#include "polygon2util.h"

namespace msbase
{
	int Polygon2::m_test = 0;
	Polygon2::Polygon2()
		: m_close(false)
		, m_root(nullptr)
		, m_circleSize(0)
		, m_points(nullptr)
	{
	}
	
	Polygon2::~Polygon2()
	{
		releaseNode();
	}

	void Polygon2::setup(const std::vector<int>& polygon, std::vector<trimesh::dvec2>& points)
	{
		m_close = false;
		m_indexes = polygon;
		m_points = &points;
		if (m_indexes.size() >= 2 && m_indexes.at(0) == m_indexes.back())
		{
			m_close = true;
			m_indexes.erase(m_indexes.begin());
		}

		int size = (int)m_indexes.size();

		if (!m_close || size == 0)
			return;

		releaseNode();

		m_circleSize = size;
		m_debugNodes.resize(m_circleSize);
		m_root = &m_debugNodes.at(0);

		for (int i = 0; i < size; ++i)
		{
			VNode& n = m_debugNodes.at(i);
			n.index = m_indexes.at(i);
			n.prev = &m_debugNodes.at((i + size - 1) % size);
			n.next = &m_debugNodes.at((i + 1) % size);
		}

		m_ears.clear();
		for (int i = 0; i < size; ++i)
		{		
			VNode& cur = m_debugNodes.at(i);
			trimesh::dvec2& o = m_points->at(cur.index);
			trimesh::dvec2& next = m_points->at(cur.next->index);
			trimesh::dvec2& prev = m_points->at(cur.prev->index);
			trimesh::dbox2& b = cur.b;
			b += o;
			b += next;
			b += prev;

			calType(&cur);

			if (cur.type == 3)
				m_ears.push_back(&cur);
		}
	}

	std::vector<int> Polygon2::toIndices()
	{
		return m_indexes;
	}

	void Polygon2::releaseNode()
	{
		m_debugNodes.clear();
		m_root = nullptr;
		m_circleSize = 0;
		m_ears.clear();
	}

	void Polygon2::getEars(std::vector<int>* earIndices)
	{
		if (earIndices)
		{
			for (VNode* n : m_ears)
			{
				earIndices->push_back(n->index);
			}
		}
	}

	std::vector<int> Polygon2::debugIndex()
	{
		return m_indexes;
	}

	void Polygon2::earClipping(std::vector<trimesh::TriMesh::Face>& triangles)
	{
		if (!m_root || !m_points) return;

		triangles.reserve(m_points->size());
		trimesh::TriMesh::Face f;
		while (earClipping(f))
		{
			triangles.push_back(f);
		}
	}

	bool Polygon2::earClipping(trimesh::TriMesh::Face& f, std::vector<int>* earIndices)
	{
		if (!m_root || !m_points || !m_close) return false;

		if (m_circleSize >= 3)
		{
			if (m_circleSize == 3)
			{
				f.x = m_root->index;
				f.y = m_root->next->index;
				f.z = m_root->next->next->index;
				releaseNode();
				return true;
			}
			else
			{
				{ // ear clipping
					if (m_ears.size() == 0)
						return false;

					VNode* used = m_ears.front();
					for(VNode* vn : m_ears)
					{// find sharpest vertex
						if (vn->dot > used->dot)
						{
							used = vn;
						}
					} 
					
					f = nodeTriangle(used);
					
					VNode* np = used->prev;
					VNode* nn = used->next;

					np->next = nn;
					nn->prev = np;
					
					m_ears.remove(used);

					if (used == m_root)
					{
						m_root = nn;
					}
					//delete used;
					--m_circleSize;

					auto check = [this](VNode* node) {
						trimesh::dvec2& o = m_points->at(node->index);
						trimesh::dvec2& next = m_points->at(node->next->index);
						trimesh::dvec2& prev = m_points->at(node->prev->index);
						trimesh::dbox2& b = node->b;
						b.clear();
						b += o;
						b += next;
						b += prev;

						bool in = node->type == 3;
						calType(node);
						if (in && node->type != 3)
							m_ears.remove(node);
						if (!in && node->type == 3)
							m_ears.push_back(node);
					};

					check(np);
					check(nn);

					if (m_ears.size() == 0 && m_circleSize >= 3)
					{// recal  // self intersection
						m_ears.clear();
						VNode* cur = m_root;
						do
						{
							calType(cur, false);

							if (cur->type == 3)
								m_ears.push_back(cur);
							cur = cur->next;
						} while (cur != m_root);
					}

					getEars(earIndices);
					return true;
				}
			}
		}
		return false;
	}

	trimesh::TriMesh::Face Polygon2::nodeTriangle(VNode* node)
	{
		VNode* np = node->prev;
		VNode* nn = node->next;

		trimesh::TriMesh::Face f;
		f.x = node->index;
		f.y = nn->index;
		f.z = np->index;
		
		return f;
	}

	void Polygon2::calType(VNode* node, bool testContainEdge)
	{
		trimesh::dvec2& o = m_points->at(node->index);
		trimesh::dvec2& next = m_points->at(node->next->index);
		trimesh::dvec2& prev = m_points->at(node->prev->index);
		trimesh::dvec2 onext = next - o;
		trimesh::dvec2 oprev = prev - o;

		double cvalue = crossValue(onext, oprev);
		if (cvalue < 0.0) {
			node->type = 1;
		}
		else
		{
			node->type = 2;
		}
		
		if (node->type == 2)
		{// check
			//node->type = 3;
			double dvalue = dotValue(onext, oprev);
			node->dot = dvalue;
			
			VNode* cur = m_root;
			bool ear = true;
			do
			{
				if ((cur->index != node->index) && (cur->index != node->next->index) && (cur->index != node->prev->index))
				{
					trimesh::dvec2& c = m_points->at(cur->index);
					if (node->b.contains(c))
					{
						if (testContainEdge && insideTriangle(o, next, prev, c))
						{
							ear = false;
							break;
						}
						else if (!testContainEdge && insideTriangleEx(o, next, prev, c))
						{
							ear = false;
							break;
						}
					}
				}
				cur = cur->next;
			} while (cur != m_root);

			if (ear) node->type = 3;
		}
	}
}