#pragma once
#include "topomesh/interface/idata.h"

#include "internal/data/mmesht.h"
#include <list>

namespace topomesh {
	class SpectralClusteringCuts
	{
	public:
		SpectralClusteringCuts() {};
		SpectralClusteringCuts(float delte, float eta):_delta(delte), _eta(eta) {};
		SpectralClusteringCuts(MMeshT* mesh, float delte, float eta);
		~SpectralClusteringCuts() {};

		void BlockSpectralClusteringCuts(MMeshT* mesh);
		std::vector<std::vector<int>>* get_result() { return &result; }
		void test();
	private:
		std::vector<std::vector<int>> result;
		float _delta;
		float _eta;
	};

	class InteractionCuts
	{
	public:
		InteractionCuts() {};
		InteractionCuts(MMeshT* mesh,std::vector<int> indication,float alpth,float delta);
		~InteractionCuts() {};
		std::vector<int>& get_result() { return result; }
	private:
		std::vector<int> result;
		float _alpth;
		float _delta;
	};
	
	struct SegmentationParam
	{

	};

	/*enum class SegmentatStrategy
	{
		ss_angle,
		ss_sdf,
		ss_num
	};*/

	class ManualSegmentationDebugger
	{
	public:

	};

	class ManualSegmentation;
	class SegmentationGroup
	{
		friend class ManualSegmentation;
	public:
		SegmentationGroup(int id, ManualSegmentation* segmentation);
		~SegmentationGroup();

		bool addSeed(int index);
		bool addSeeds(const std::vector<int>& indices);
		bool removeSeed(int index);
	protected:
		ManualSegmentation* m_segmentation;
		int m_id;
	};

	class ManualSegmentation
	{
		friend class SegmentationGroup;
	public:
		ManualSegmentation(TopoTriMeshPtr mesh);
		~ManualSegmentation();

		void setDebugger(ManualSegmentationDebugger* debugger);

		//seed
		SegmentationGroup* addGroup();
		void removeGroup(SegmentationGroup* group);

		void segment();
		SegmentationGroup* checkIndex(int index);
	protected:
		bool checkOther(int index, int groupID);
		bool checkEmpty(int index);
		bool checkIn(int index, int groupID);
		bool checkValid(int index);

		void replaceSeeds(const std::vector<int>& indices, int groupID);
		void removeSeed(int index, int groupID);
	protected:
		int m_usedGroupId;
		TopoTriMeshPtr m_mesh;
		int m_faceSize;
		std::vector<int> m_groupMap;
		std::vector<SegmentationGroup*> m_groups;

		ManualSegmentationDebugger* m_debugger;
	};
}