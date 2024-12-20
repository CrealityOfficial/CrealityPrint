#include"mmeshVertex.h"


namespace topomesh {
	bool MMeshVertex::is_neighbor(MMeshVertex* v)
	{
		for (int i = 0; i < this->connected_vertex.size(); i++)
		{
			if (v->index==this->connected_vertex[i]->index)
			{
				return true;
			}
		}
		return false;
	}
};