#include "msbase/mesh/deserializecolor.h"

#include "trimesh2/TriMesh_algo.h"
#include <assert.h>
#include <array>
#include <memory>
#include <map>

namespace msbase
{
	constexpr double EPSILON = 1e-4;

	enum class EnforcerBlockerType : int8_t {
		// Maximum is 3. The value is serialized in TriangleSelector into 2 bits.
		NONE = 0,
		ENFORCER = 1,
		BLOCKER = 2,
		// Maximum is 15. The value is serialized in TriangleSelector into 6 bits using a 2 bit prefix code.
		Extruder1 = ENFORCER,
		Extruder2 = BLOCKER,
		Extruder3,
		Extruder4,
		Extruder5,
		Extruder6,
		Extruder7,
		Extruder8,
		Extruder9,
		Extruder10,
		Extruder11,
		Extruder12,
		Extruder13,
		Extruder14,
		Extruder15,
		ExtruderMax
	};

	enum class Partition {
		First,
		Second,
	};

	class Triangle {
	public:
		// Use TriangleSelector::push_triangle to create a new triangle.
		// It increments/decrements reference counter on vertices.
		Triangle(int a, int b, int c, int source_triangle, const EnforcerBlockerType init_state)
			: verts_idxs{ a, b, c },
			source_triangle{ source_triangle },
			state{ init_state }
		{
			// Initialize bit fields. Default member initializers are not supported by C++17.
			m_selected_by_seed_fill = false;
			m_valid = true;
		}
		// Indices into m_vertices.
		std::array<int, 3> verts_idxs;

		// Index of the source triangle at the initial (unsplit) mesh.
		int source_triangle;

		// Children triangles.
		std::array<int, 4> children;

		// Set the division type.
		void set_division(int sides_to_split, int special_side_idx);

		// Get/set current state.
		void set_state(EnforcerBlockerType type) { assert(!is_split()); state = type; }
		EnforcerBlockerType get_state() const { assert(!is_split()); return state; }

		// Set if the triangle has been selected or unselected by seed fill.
		void select_by_seed_fill() { assert(!is_split()); m_selected_by_seed_fill = true; }
		void unselect_by_seed_fill() { assert(!is_split()); m_selected_by_seed_fill = false; }
		// Get if the triangle has been selected or not by seed fill.
		bool is_selected_by_seed_fill() const { assert(!is_split()); return m_selected_by_seed_fill; }

		// Is this triangle valid or marked to be removed?
		bool valid() const noexcept { return m_valid; }
		// Get info on how it's split.
		bool is_split() const noexcept { return number_of_split_sides() != 0; }
		int number_of_split_sides() const noexcept { return number_of_splits; }
		int special_side() const noexcept { assert(is_split()); return special_side_idx; }

	private:
		// Packing the rest of member variables into 4 bytes, aligned to 4 bytes boundary.
		char number_of_splits{ 0 };
		// Index of a vertex opposite to the split edge (for number_of_splits == 1)
		// or index of a vertex shared by the two split edges (for number_of_splits == 2).
		// For number_of_splits == 3, special_side_idx is always zero.
		char special_side_idx{ 0 };
		EnforcerBlockerType state;
		bool m_selected_by_seed_fill : 1;
		// Is this triangle valid or marked to be removed?
		bool m_valid : 1;
	};

	// sides_to_split==-1 : just restore previous split
	void Triangle::set_division(int sides_to_split, int special_side_idx)
	{
		assert(sides_to_split >= 0 && sides_to_split <= 3);
		assert(special_side_idx >= 0 && special_side_idx < 3);
		assert(sides_to_split == 1 || sides_to_split == 2 || special_side_idx == 0);
		this->number_of_splits = char(sides_to_split);
		this->special_side_idx = char(special_side_idx);
	}

	struct Vertex {
		explicit Vertex(const trimesh::vec& vert)
			: v{ vert },
			ref_cnt{ 0 }
		{}
		trimesh::vec v;
		int ref_cnt;
	};

	template<typename INDEX_TYPE>
	INDEX_TYPE next_idx_modulo(INDEX_TYPE idx, const INDEX_TYPE count)
	{
		if (++idx == count)
			idx = 0;
		return idx;
	}

	template<typename INDEX_TYPE>
	inline INDEX_TYPE prev_idx_modulo(INDEX_TYPE idx, const INDEX_TYPE count)
	{
		if (idx == 0)
			idx = count;
		return --idx;
	}

	struct DeserialzeData
	{
		std::pair<std::vector<std::pair<int, int>>, std::vector<bool>> m_data;
		trimesh::TriMesh* mesh;
		std::vector<std::vector<int>> m_neighbors;
		// Lists of vertices and triangles, both original and new
		//std::vector<Vertex> m_vertices;
		//std::vector<Vertex> m_vertices;
		std::vector<int> m_ref_cnt;
		std::vector<Triangle> m_triangles;
		int m_free_vertices_head = -1;
		int m_invalid_triangles{ -1 };
		int m_free_triangles_head{ -1 };
	};

	int neighbor_child(DeserialzeData& deserialzedata,const Triangle& tr, int vertexi, int vertexj, Partition partition)
	{
		std::vector<Triangle>& m_triangles = deserialzedata.m_triangles;

		if (tr.number_of_split_sides() == 0)
			// If this triangle is not split, then there is no upper / lower subtriangle sharing the edge.
			return -1;

		// Find the triangle edge.
		int edge = tr.verts_idxs[0] == vertexi ? 0 : tr.verts_idxs[1] == vertexi ? 1 : 2;
		assert(tr.verts_idxs[edge] == vertexi);
		assert(tr.verts_idxs[next_idx_modulo(edge, 3)] == vertexj);

		int child_idx;
		if (tr.number_of_split_sides() == 1) {
			if (edge != next_idx_modulo(tr.special_side(), 3))
				// A child may or may not be split at this side.
				return neighbor_child(deserialzedata,m_triangles[tr.children[edge == tr.special_side() ? 0 : 1]], vertexi, vertexj, partition);
			child_idx = partition == Partition::First ? 0 : 1;
		}
		else if (tr.number_of_split_sides() == 2) {
			if (edge == next_idx_modulo(tr.special_side(), 3))
				// A child may or may not be split at this side.
				return neighbor_child(deserialzedata,m_triangles[tr.children[2]], vertexi, vertexj, partition);
			child_idx = edge == tr.special_side() ?
				(partition == Partition::First ? 0 : 1) :
				(partition == Partition::First ? 2 : 0);
		}
		else {
			assert(tr.number_of_split_sides() == 3);
			assert(tr.special_side() == 0);
			switch (edge) {
			case 0:  child_idx = partition == Partition::First ? 0 : 1; break;
			case 1:  child_idx = partition == Partition::First ? 1 : 2; break;
			default: assert(edge == 2);
				child_idx = partition == Partition::First ? 2 : 0; break;
			}
		}
		return tr.children[child_idx];
	}

	// Return child of itriangle at a CCW oriented side (vertexi, vertexj), either first or 2nd part.
// If itriangle == -1 or if the side sharing (vertexi, vertexj) is not split, return -1.
	int neighbor_child(DeserialzeData& deserialzedata,int itriangle, int vertexi, int vertexj, Partition partition)
	{
		std::vector<Triangle>& m_triangles = deserialzedata.m_triangles;
		return itriangle == -1 ? -1 : neighbor_child(deserialzedata,m_triangles[itriangle], vertexi, vertexj, partition);
	}

	std::vector<int> child_neighbors(DeserialzeData& deserialzedata,const Triangle& tr, const std::vector<int>& neighbors, int child_idx)
	{
		//assert(this->verify_triangle_neighbors(tr, neighbors));

		assert(child_idx >= 0 && child_idx <= tr.number_of_split_sides());
		int   i = tr.special_side();
		int   j = next_idx_modulo(i, 3);
		int   k = next_idx_modulo(j, 3);

		std::vector<int> out;
		out.resize(3);
		switch (tr.number_of_split_sides()) {
		case 1:
			switch (child_idx) {
			case 0:
				out[0] = neighbors.at(i);
				out[1] = neighbor_child(deserialzedata,neighbors.at(j), tr.verts_idxs[k], tr.verts_idxs[j], Partition::Second);
				out[2] = tr.children[1];
				break;
			default:
				assert(child_idx == 1);
				out[0] = neighbor_child(deserialzedata, neighbors.at(i), tr.verts_idxs[k], tr.verts_idxs[j], Partition::First);
				out[1] = neighbors.at(k);
				out[2] = tr.children[0];
				break;
			}
			break;

		case 2:
			switch (child_idx) {
			case 0:
				out[0] = neighbor_child(deserialzedata, neighbors.at(i), tr.verts_idxs[j], tr.verts_idxs[i], Partition::Second);
				out[1] = tr.children[1];
				out[2] = neighbor_child(deserialzedata, neighbors.at(k), tr.verts_idxs[i], tr.verts_idxs[k], Partition::First);
				break;
			case 1:
				assert(child_idx == 1);
				out[0] = neighbor_child(deserialzedata, neighbors.at(i), tr.verts_idxs[j], tr.verts_idxs[i], Partition::First);
				out[1] = tr.children[2];
				out[2] = tr.children[0];
				break;
			default:
				assert(child_idx == 2);
				out[0] = neighbors[j];
				out[1] = neighbor_child(deserialzedata, neighbors.at(k), tr.verts_idxs[i], tr.verts_idxs[k], Partition::Second);
				out[2] = tr.children[1];
				break;
			}
			break;

		case 3:
			assert(tr.special_side() == 0);
			switch (child_idx) {
			case 0:
				out[0] = neighbor_child(deserialzedata, neighbors.at(0), tr.verts_idxs[1], tr.verts_idxs[0], Partition::Second);
				out[1] = tr.children[3];
				out[2] = neighbor_child(deserialzedata, neighbors.at(2), tr.verts_idxs[0], tr.verts_idxs[2], Partition::First);
				break;
			case 1:
				out[0] = neighbor_child(deserialzedata, neighbors.at(0), tr.verts_idxs[1], tr.verts_idxs[0], Partition::First);
				out[1] = neighbor_child(deserialzedata, neighbors.at(1), tr.verts_idxs[2], tr.verts_idxs[1], Partition::Second);
				out[2] = tr.children[3];
				break;
			case 2:
				out[0] = neighbor_child(deserialzedata, neighbors.at(1), tr.verts_idxs[2], tr.verts_idxs[1], Partition::First);
				out[1] = neighbor_child(deserialzedata, neighbors.at(2), tr.verts_idxs[0], tr.verts_idxs[2], Partition::Second);
				out[2] = tr.children[3];
				break;
			default:
				assert(child_idx == 3);
				out[0] = tr.children[1];
				out[1] = tr.children[2];
				out[2] = tr.children[0];
				break;
			}
			break;

		default:
			assert(false);
		}

		//assert(this->verify_triangle_neighbors(tr, neighbors));
		//(this->verify_triangle_neighbors(m_triangles[tr.children[child_idx]], out));
		return out;
	}


	inline uint16_t next_highest_power_of_2(uint16_t v)
	{
		if (v != 0)
			--v;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		return ++v;
	}
	inline uint32_t next_highest_power_of_2(uint32_t v)
	{
		if (v != 0)
			--v;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		return ++v;
	}
	inline uint64_t next_highest_power_of_2(uint64_t v)
	{
		if (v != 0)
			--v;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v |= v >> 32;
		return ++v;
	}

	// Return existing midpoint of CCW oriented side (vertexi, vertexj).
// If itriangle == -1 or if the side sharing (vertexi, vertexj) is not split, return -1.
	int triangle_midpoint(DeserialzeData& deserialzedata,const Triangle& tr, int vertexi, int vertexj)
	{
		std::vector<Triangle>& m_triangles = deserialzedata.m_triangles;

		if (tr.number_of_split_sides() == 0)
			// If this triangle is not split, then there is no upper / lower subtriangle sharing the edge.
			return -1;

		// Find the triangle edge.
		int edge = tr.verts_idxs[0] == vertexi ? 0 : tr.verts_idxs[1] == vertexi ? 1 : 2;
		assert(tr.verts_idxs[edge] == vertexi);
		assert(tr.verts_idxs[next_idx_modulo(edge, 3)] == vertexj);

		if (tr.number_of_split_sides() == 1) {
			return edge == next_idx_modulo(tr.special_side(), 3) ?
				m_triangles[tr.children[0]].verts_idxs[2] :
				triangle_midpoint(deserialzedata,m_triangles[tr.children[edge == tr.special_side() ? 0 : 1]], vertexi, vertexj);
		}
		else if (tr.number_of_split_sides() == 2) {
			return edge == next_idx_modulo(tr.special_side(), 3) ?
				triangle_midpoint(deserialzedata,m_triangles[tr.children[2]], vertexi, vertexj) :
				edge == tr.special_side() ?
				m_triangles[tr.children[0]].verts_idxs[1] :
				m_triangles[tr.children[1]].verts_idxs[2];
		}
		else {
			assert(tr.number_of_split_sides() == 3);
			assert(tr.special_side() == 0);
			return
				(edge == 0) ? m_triangles[tr.children[0]].verts_idxs[1] :
				(edge == 1) ? m_triangles[tr.children[1]].verts_idxs[2] :
				m_triangles[tr.children[2]].verts_idxs[2];
		}
	}

	// Return existing midpoint of CCW oriented side (vertexi, vertexj).
	// If itriangle == -1 or if the side sharing (vertexi, vertexj) is not split, return -1.
	int triangle_midpoint(DeserialzeData& deserialzedata,int itriangle, int vertexi, int vertexj)
	{
		std::vector<Triangle>& m_triangles = deserialzedata.m_triangles;
		return itriangle == -1 ? -1 : triangle_midpoint(deserialzedata,m_triangles[itriangle], vertexi, vertexj);
	}

	int triangle_midpoint_or_allocate(DeserialzeData& deserialzedata, int itriangle, int vertexi, int vertexj)
	{
		std::vector<trimesh::point>& m_vertices = deserialzedata.mesh->vertices;
		int& m_free_vertices_head = deserialzedata.m_free_vertices_head;
		std::vector<int>& ref_cnt = deserialzedata.m_ref_cnt;

		int midpoint = triangle_midpoint(deserialzedata,itriangle, vertexi, vertexj);
		if (midpoint == -1) {
			trimesh::vec c = 0.5f * (m_vertices[vertexi] + m_vertices[vertexj]);
//#ifdef EXPENSIVE_DEBUG_CHECKS
//			//// Verify that the vertex is really a new one.
//			//auto it = std::find_if(m_vertices.begin(), m_vertices.end(), [c](const Vertex& v) {
//			//	return v.ref_cnt > 0 && (v.v - c).norm() < EPSILON; });
//			//assert(it == m_vertices.end());
//#endif // EXPENSIVE_DEBUG_CHECKS
			// Allocate a new vertex, possibly reusing the free list.
			if (m_free_vertices_head == -1) {
				// Allocate a new vertex.
				midpoint = int(m_vertices.size());
				m_vertices.emplace_back(c);
				ref_cnt.emplace_back(0);
			}
			else {
				// Reuse a vertex from the free list.
				assert(m_free_vertices_head >= -1 && m_free_vertices_head < int(m_vertices.size()));
				midpoint = m_free_vertices_head;
				memcpy(&m_free_vertices_head, &m_vertices[midpoint].x, sizeof(m_free_vertices_head));
				assert(m_free_vertices_head >= -1 && m_free_vertices_head < int(m_vertices.size()));
				m_vertices[midpoint] = c;
			}
			assert(ref_cnt[midpoint] == 0);
		}
		else {
#ifndef NDEBUG
			//trimesh::vec c1 = 0.5f * (m_vertices[vertexi].v + m_vertices[vertexj].v);
			//trimesh::vec c2 = m_vertices[midpoint].v;
			//float d = (c2 - c1).norm();
			//assert(std::abs(d) < EPSILON);
#endif // NDEBUG
			assert(ref_cnt[midpoint] > 0);
		}
		return midpoint;
	}

	int push_triangle(DeserialzeData& deserialzedata, int a, int b, int c, int source_triangle, const EnforcerBlockerType state)
	{
		std::vector<trimesh::point>& m_vertices = deserialzedata.mesh->vertices;
		int& m_free_triangles_head = deserialzedata.m_free_triangles_head;
		int& m_invalid_triangles = deserialzedata.m_invalid_triangles;
		std::vector<Triangle>& m_triangles = deserialzedata.m_triangles;

		int num = m_vertices.size() - deserialzedata.m_ref_cnt.size();
		if (num > 0 && num < m_vertices.size())
		{
			std::vector<int> add(num, 0);
			deserialzedata.m_ref_cnt.insert(deserialzedata.m_ref_cnt.end(), add.begin(),add.end());
		}

		for (int i : {a, b, c}) {
			assert(i >= 0 && i < int(m_vertices.size()));
			++deserialzedata.m_ref_cnt[i];
		}
		int idx;
		if (m_free_triangles_head == -1) {
			// Allocate a new triangle.
			assert(m_invalid_triangles == 0);
			idx = int(m_triangles.size());
			m_triangles.emplace_back(a, b, c, source_triangle, state);
		}
		else {
			// Reuse triangle from the free list.
			assert(m_free_triangles_head >= -1 && m_free_triangles_head < int(m_triangles.size()));
			assert(!m_triangles[m_free_triangles_head].valid());
			assert(m_invalid_triangles > 0);
			idx = m_free_triangles_head;
			m_free_triangles_head = m_triangles[idx].children[0];
			--m_invalid_triangles;
			assert(m_free_triangles_head >= -1 && m_free_triangles_head < int(m_triangles.size()));
			assert(m_free_triangles_head == -1 || !m_triangles[m_free_triangles_head].valid());
			assert(m_invalid_triangles >= 0);
			assert((m_invalid_triangles == 0) == (m_free_triangles_head == -1));
			m_triangles[idx] = { a, b, c, source_triangle, state };
		}
		assert(m_triangles[idx].valid());
		return idx;
	}

	// called by deserialize() and select_patch()->select_triangle()->...select_triangle()->split_triangle()
// Split a triangle based on Triangle::number_of_split_sides() and Triangle::special_side()
// by allocating child triangles and midpoint vertices.
// Midpoint vertices are possibly reused by traversing children of neighbor triangles.
	void perform_split(DeserialzeData& deserialzedata,int facet_idx, const std::vector<int>& neighbors, EnforcerBlockerType old_state)
	{
		std::vector<Triangle>& m_triangles = deserialzedata.m_triangles;

		// Reserve space for the new triangles upfront, so that the reference to this triangle will not change.
		{
			size_t num_triangles_new = m_triangles.size() + m_triangles[facet_idx].number_of_split_sides() + 1;
			if (m_triangles.capacity() < num_triangles_new)
				m_triangles.reserve(next_highest_power_of_2((uint64_t)num_triangles_new));
		}

		Triangle& tr = m_triangles[facet_idx];
		assert(tr.is_split());

		// indices of triangle vertices
//#ifdef NDEBUG
//		boost::container::small_vector<int, 6> verts_idxs;
//#else // NDEBUG
	// For easier debugging.
		std::vector<int> verts_idxs;
		verts_idxs.reserve(6);
//#endif // NDEBUG
		for (int j = 0, idx = tr.special_side(); j < 3; ++j, idx = next_idx_modulo(idx, 3))
			verts_idxs.push_back(tr.verts_idxs[idx]);

		auto get_alloc_vertex = [&neighbors, &verts_idxs,&deserialzedata](int edge, int i1, int i2) -> int {
			return triangle_midpoint_or_allocate(deserialzedata,neighbors.at(edge), verts_idxs[i1], verts_idxs[i2]);
		};

		int ichild = 0;
		switch (tr.number_of_split_sides()) {
		case 1:
			verts_idxs.insert(verts_idxs.begin() + 2, get_alloc_vertex(next_idx_modulo(tr.special_side(), 3), 2, 1));
			tr.children[ichild++] = push_triangle(deserialzedata,verts_idxs[0], verts_idxs[1], verts_idxs[2], tr.source_triangle, old_state);
			tr.children[ichild] = push_triangle(deserialzedata, verts_idxs[2], verts_idxs[3], verts_idxs[0], tr.source_triangle, old_state);
			break;

		case 2:
			verts_idxs.insert(verts_idxs.begin() + 1, get_alloc_vertex(tr.special_side(), 1, 0));
			verts_idxs.insert(verts_idxs.begin() + 4, get_alloc_vertex(prev_idx_modulo(tr.special_side(), 3), 0, 3));
			tr.children[ichild++] = push_triangle(deserialzedata, verts_idxs[0], verts_idxs[1], verts_idxs[4], tr.source_triangle, old_state);
			tr.children[ichild++] = push_triangle(deserialzedata, verts_idxs[1], verts_idxs[2], verts_idxs[4], tr.source_triangle, old_state);
			tr.children[ichild] = push_triangle(deserialzedata, verts_idxs[2], verts_idxs[3], verts_idxs[4], tr.source_triangle, old_state);
			break;

		case 3:
			assert(tr.special_side() == 0);
			verts_idxs.insert(verts_idxs.begin() + 1, get_alloc_vertex(0, 1, 0));
			verts_idxs.insert(verts_idxs.begin() + 3, get_alloc_vertex(1, 3, 2));
			verts_idxs.insert(verts_idxs.begin() + 5, get_alloc_vertex(2, 0, 4));
			tr.children[ichild++] = push_triangle(deserialzedata, verts_idxs[0], verts_idxs[1], verts_idxs[5], tr.source_triangle, old_state);
			tr.children[ichild++] = push_triangle(deserialzedata, verts_idxs[1], verts_idxs[2], verts_idxs[3], tr.source_triangle, old_state);
			tr.children[ichild++] = push_triangle(deserialzedata, verts_idxs[3], verts_idxs[4], verts_idxs[5], tr.source_triangle, old_state);
			tr.children[ichild] = push_triangle(deserialzedata, verts_idxs[1], verts_idxs[3], verts_idxs[5], tr.source_triangle, old_state);
			break;

		default:
			break;
		}

#ifndef NDEBUG
		//assert(this->verify_triangle_neighbors(tr, neighbors));
		//for (int i = 0; i <= tr.number_of_split_sides(); ++i) {
		//	Vec3i n = this->child_neighbors(tr, neighbors, i);
		//	assert(this->verify_triangle_neighbors(m_triangles[tr.children[i]], n));
		//}
#endif // NDEBUG
	}

	// Recover triangle splitting & state from string of hexadecimal values previously
// generated by get_triangle_as_string. Used to load from 3MF.
	void set_triangle_from_string(std::pair<std::vector<std::pair<int, int>>, std::vector<bool>>& m_data,int triangle_id, const std::string& str)
	{
		assert(!str.empty());
		assert(m_data.first.empty() || m_data.first.back().first < triangle_id);
		m_data.first.emplace_back(triangle_id, int(m_data.second.size()));

		for (auto it = str.crbegin(); it != str.crend(); ++it) {
			const char ch = *it;
			int dec = 0;
			if (ch >= '0' && ch <= '9')
				dec = int(ch - '0');
			else if (ch >= 'A' && ch <= 'F')
				dec = 10 + int(ch - 'A');
			else
				assert(false);

			// Convert to binary and append into code.
			for (int i = 0; i < 4; ++i)
				m_data.second.insert(m_data.second.end(), bool(dec & (1 << i)));
		}
	}

	void deserialize(
		DeserialzeData& deserialzedata
		, bool needs_reset = true
		, EnforcerBlockerType max_ebt = EnforcerBlockerType::ExtruderMax)
	{
		std::vector<Triangle>& m_triangles = deserialzedata.m_triangles;
		trimesh::TriMesh* sourceMesh = deserialzedata.mesh;
		std::pair<std::vector<std::pair<int, int>>, std::vector<bool>>& data = deserialzedata.m_data;
		std::vector<trimesh::point>& m_vertices = sourceMesh->vertices;
		std::vector<std::vector<int>>& m_neighbors = deserialzedata.m_neighbors;

		// Reserve number of triangles as if each triangle was saved with 4 bits.
		// With MMU painting this estimate may be somehow low, but better than nothing.
		m_triangles.reserve(std::max(sourceMesh->faces.size(), data.second.size() / 4));
		// Number of triangles is twice the number of vertices on a large manifold mesh of genus zero.
		// Here the triangles count account for both the nodes and leaves, thus the following line may overestimate.
		m_vertices.reserve(std::max(sourceMesh->vertices.size(), m_triangles.size() / 2));
	
		// Vector to store all parents that have offsprings.
		struct ProcessingInfo {
			int facet_id = 0;
			std::vector<int> neighbors{ -1, -1, -1 };
			int processed_children = 0;
			int total_children = 0;
		};

		// Depth-first queue of a source mesh triangle and its childern.
		// kept outside of the loop to avoid re-allocating inside the loop.
		std::vector<ProcessingInfo> parents;

		for (auto [triangle_id, ibit] : data.first) {
			//assert(triangle_id < int(m_triangles.size()));
			//assert(ibit < int(data.second.size()));
			auto next_nibble = [&data, &ibit = ibit]() {
				int n = 0;
				for (int i = 0; i < 4; ++i)
					if (ibit < data.second.size())
					{
						n |= data.second[ibit++] << i;
					}			
				return n;
			};

			parents.clear();
			while (true) {
				// Read next triangle info.
				int code = next_nibble();
				int num_of_split_sides = code & 0b11;
				int num_of_children = num_of_split_sides == 0 ? 0 : num_of_split_sides + 1;
				bool is_split = num_of_children != 0;
				// Only valid if not is_split. Value of the second nibble was subtracted by 3, so it is added back.
				auto state = is_split ? EnforcerBlockerType::NONE : EnforcerBlockerType((code & 0b1100) == 0b1100 ? next_nibble() + 3 : code >> 2);

				// BBS
				if (state > max_ebt)
					state = EnforcerBlockerType::NONE;

				// Only valid if is_split.
				int special_side = code >> 2;

				// Take care of the first iteration separately, so handling of the others is simpler.
				if (parents.empty()) {
					if (is_split) {
						// root is split, add it into list of parents and split it.
						// then go to the next.
						std::vector<int> neighbors = m_neighbors[triangle_id];
						parents.push_back({ triangle_id, neighbors, 0, num_of_children });
						m_triangles[triangle_id].set_division(num_of_split_sides, special_side);
						perform_split(deserialzedata,triangle_id, neighbors, EnforcerBlockerType::NONE);
						continue;
					}
					else {
						// root is not split. just set the state and that's it.
						m_triangles[triangle_id].set_state(state);
						break;
					}
				}

				// This is not the first iteration. This triangle is a child of last seen parent.
				assert(!parents.empty());
				assert(parents.back().processed_children < parents.back().total_children);

				if (ProcessingInfo& last = parents.back();  is_split) {
					// split the triangle and save it as parent of the next ones.
					const Triangle& tr = m_triangles[last.facet_id];
					int   child_idx = last.total_children - last.processed_children - 1;
					std::vector<int> neighbors = child_neighbors(deserialzedata,tr, last.neighbors, child_idx);
					int this_idx = tr.children[child_idx];
					m_triangles[this_idx].set_division(num_of_split_sides, special_side);
					perform_split(deserialzedata,this_idx, neighbors, EnforcerBlockerType::NONE);
					parents.push_back({ this_idx, neighbors, 0, num_of_children });
				}
				else {
					// this triangle belongs to last split one
					int child_idx = last.total_children - last.processed_children - 1;
					m_triangles[m_triangles[last.facet_id].children[child_idx]].set_state(state);
					++last.processed_children;
				}

				// If all children of the past parent triangle are claimed, move to grandparent.
				while (parents.back().processed_children == parents.back().total_children) {
					parents.pop_back();

					if (parents.empty())
						break;

					// And increment the grandparent children counter, because
					// we have just finished that branch and got back here.
					++parents.back().processed_children;
				}

				// In case we popped back the root, we should be done.
				if (parents.empty())
					break;
			}
		}

	}

	trimesh::TriMesh* mergeColorMeshes(trimesh::TriMesh* sourceMesh, const std::vector<std::string>& color2Facets,
		std::vector<int>& facet2Facets, bool onlyFlags, int state, ccglobal::Tracer* tracer)
	{
		if (!sourceMesh)
			return nullptr;

		DeserialzeData deserialzedata;
		trimesh::TriMesh* meshNew = new trimesh::TriMesh();
		meshNew->vertices = sourceMesh->vertices;
		meshNew->faces = sourceMesh->faces;

		if (sourceMesh->faces.size() != color2Facets.size())
			return meshNew;

		//init
		deserialzedata.mesh = meshNew;
		std::vector<int> ref_cnt(meshNew->vertices.size(),0);
		deserialzedata.m_ref_cnt = ref_cnt;
		deserialzedata.m_invalid_triangles = 0;

		//deserialzedata.m_neighbors;
		deserialzedata.mesh->need_normals();
		deserialzedata.mesh->need_across_edge();

		//deserialzedata.m_triangles;
		for (size_t i = 0; i < sourceMesh->faces.size(); i++)
		{
			trimesh::TriMesh::Face& face = deserialzedata.mesh->faces[i];
			if (!color2Facets[i].empty())
				set_triangle_from_string(deserialzedata.m_data, i, color2Facets[i]);

			trimesh::TriMesh::Face& neighbor =deserialzedata.mesh->across_edge[i];
			deserialzedata.m_neighbors.push_back(std::vector<int>{neighbor.z, neighbor.x, neighbor.y});
			deserialzedata.m_triangles.push_back(Triangle(face.at(0), face.at(1), face.at(2),i, EnforcerBlockerType::NONE));
		}

		deserialize(deserialzedata);
		
		// Lists of vertices and triangles, both original and new
		//deserialzedata.m_free_vertices_head;
		//deserialzedata.m_invalid_triangles;
		//deserialzedata.m_free_triangles_head;

 		deserialzedata.mesh->faces.clear(); 
		deserialzedata.mesh->flags.clear();
		facet2Facets.clear();
		deserialzedata.mesh->faces.reserve(deserialzedata.m_triangles.size());
		deserialzedata.mesh->flags.reserve(deserialzedata.m_triangles.size());
		facet2Facets.reserve(deserialzedata.m_triangles.size());
		for (auto& triangle : deserialzedata.m_triangles)
		{
			if (!triangle.is_split())
			{

				if (onlyFlags)
				{
					if ((int)triangle.get_state()  == state)
					{
						deserialzedata.mesh->faces.push_back(trimesh::TriMesh::Face(triangle.verts_idxs[0], triangle.verts_idxs[1], triangle.verts_idxs[2]));
						deserialzedata.mesh->flags.push_back((int)triangle.get_state());
					}
				}
				else
				{
					deserialzedata.mesh->faces.push_back(trimesh::TriMesh::Face(triangle.verts_idxs[0], triangle.verts_idxs[1], triangle.verts_idxs[2]));
					deserialzedata.mesh->flags.push_back((int)triangle.get_state());
				}

				facet2Facets.push_back(triangle.source_triangle);
			}
		}

		//deserialzedata.mesh->write("d:/deserial.stl");
		return deserialzedata.mesh;
	}

	void writePoly(trimesh::TriMesh* sourceMesh, const std::string& fileName)
	{
		if (!sourceMesh)
			return;

		int face = sourceMesh->faces.size();
		int v = sourceMesh->vertices.size();
		std::fstream out(fileName, std::ios::out | std::ios::binary | std::ios::app);
		if (out.is_open() && face > 0)
		{
			out.write((char*)&face, sizeof(int));
			if (face > 0)
			{
				for (int i = 0; i < face; ++i)
				{
					int num = 3;
					out.write((char*)&num, sizeof(int));
					for (int j = 0; j < num; j++)
					{
						out.write((char*)&sourceMesh->faces[i][j], sizeof(int));
					}
				}
			}

			out.write((char*)&v, sizeof(int));
			if (v > 0)
			{
				for (int i = 0; i < v; ++i)
				{
					int num = 3;
					out.write((char*)&num, sizeof(int));
					for (int j = 0; j < num; j++)
					{
						out.write((char*)&sourceMesh->vertices[i][j], sizeof(float));
					}
				}
			}

			out.close();
		}
	}


	//void resetFile(const std::string& fileName)
	//{
	//	std::ofstream file_writer(fileName, std::ios_base::out);
	//}


	bool getColorPloygon(trimesh::TriMesh* sourceMesh, const trimesh::xform& _xform, const std::vector<std::string>& color2Facets,
		const std::string& fileName, int state, ccglobal::Tracer* tracer)
	{
		if (!sourceMesh || color2Facets.empty())
			return false;

		std::vector<int> facet2Facets;
		std::shared_ptr<trimesh::TriMesh> _mesh(mergeColorMeshes(sourceMesh, color2Facets, facet2Facets,true, state));
		
		if (_mesh->vertices.size() == sourceMesh->vertices.size()
			&& _mesh->faces.size() == sourceMesh->faces.size())
			return false;

		//_mesh->write("d:/color.stl");
		_mesh->need_bbox();
		_mesh->need_normals();
		trimesh::apply_xform(_mesh.get(), _xform);

		writePoly(_mesh.get(), fileName);

		return true;
	}
}