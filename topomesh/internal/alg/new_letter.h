#pragma once
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "topomesh/interface.h"



namespace topomesh {

	


	struct TOPOMESH_API FontConfig 
	{
		int reliefTargetID { -1 };

		std::string text { "TEXT" };
		std::string font { "Arial" };
		int fontSize { 20 };
		int wordSpace { 0 };
		int lineSpace { 0 };
		float height { 1 };
		float distance { 0 }; // 0-2
		int embossType { 0 };
		bool bold { false };
		bool italic { false };
		float angle { 0.0 };
		int state { 0 }; //0:水平  1:环绕

		bool valid { false };
	};

	struct TOPOMESH_API FontMoveConfig 
	{
		int targetId { -1 };
		int faceId;
		trimesh::vec3 cross;
		trimesh::vec3 normal;
		int embossType;
	};


	class TOPOMESH_API FontMesh {
	public:
		FontMesh() {};
		FontMesh(float height,float depth,float angle);
		FontMesh(std::string& str, const std::vector<std::vector<std::vector<trimesh::vec2>>>& letter);
		FontMesh(const FontMesh& other);
		/*FontMesh(const std::vector<std::vector<std::vector<trimesh::vec2>>>& letter, float height,
			trimesh::vec3 face_to=trimesh::vec3(0,0,-1),trimesh::vec3 up=trimesh::vec3(0,-1,0));*/
		~FontMesh();


		void CreateFontMesh(const std::vector<std::vector<std::vector<trimesh::vec2>>>& letter,
			trimesh::vec3 face_to = trimesh::vec3(0, 0, -1), trimesh::vec3 up = trimesh::vec3(0, -1, 0),bool is_adjust=true ,bool is_init=true);
		void InitFontMesh(bool is_init=true);
		trimesh::TriMesh* getFontMesh();
		bool FontTransform(trimesh::TriMesh* traget_meshes, int face_id, trimesh::vec3 location, bool is_surround = false);
		void rotateFontMesh(trimesh::TriMesh* traget_mesh,float angle);
		void updateFontPoly(trimesh::TriMesh* traget_mesh,const std::vector<std::vector<std::vector<trimesh::vec2>>>& letter);
		void updateFontHeight(float height);
		void updateFontDepth(float depth);
		
		void updateModelXform(trimesh::xform xform);
		

		void setState(int state);
		void setText(const std::string& text);
		std::string text() const;

		float angle();
		float height();
		float depth();
		trimesh::xform xform();

		trimesh::vec3 currentFaceTo();

		FontConfig *config() { return &m_config; }

		void setMoveConfig(FontMoveConfig config) { m_moveConfig = config; }
		FontMoveConfig moveConfig() { return m_moveConfig; }

		int faceId() { return sel_faceid; }

		//-----------去除轮廓-------------
		std::string getDataToString(); //高、深、角、轮廓、状态、转盘面、faceid、click、水面朝向、up、围绕面朝向、围绕up、围绕位置
		

	private:

		bool checkMistakes(trimesh::TriMesh* traget_meshes, int face_id, trimesh::vec3 location);
		bool calRelativeCoord(trimesh::TriMesh* traget_meshes, int face_id, trimesh::vec3 location);
		trimesh::vec3 getRelativeCoord(trimesh::TriMesh* traget_meshes);


	private:
		std::vector<std::vector<std::vector<trimesh::vec2>>> _m_letter;
		std::vector<trimesh::TriMesh*> init_font_meshs;
		std::vector<trimesh::vec3> word_init_location;
		std::vector<trimesh::vec3> word_absolute_location;	
		std::pair<trimesh::vec3, trimesh::vec3> FaceTo;
		std::pair<trimesh::vec3, trimesh::vec3> Up;				

		std::vector<trimesh::vec3> word_FaceTo;
		std::vector<trimesh::vec3> word_Up;
		trimesh::vec3 click_location;
		trimesh::vec2 relative_coord;
		trimesh::vec3 current_faceto=trimesh::vec3(0,0,1);
		//int _m_state = 0;//0:水平  1:环绕

		trimesh::xform _m_xform= trimesh::xform::identity();
		trimesh::xform _m_scale= trimesh::xform::identity();
		trimesh::xform _m_rota = trimesh::xform::identity();
		//float Height;
		//float m_depth;
		//float _m_angle=0.f;
		
		int sel_faceid=-1;
		bool is_init_location = false;
		bool is_init_adjust = true;
		bool is_change_state = false;
		//trimesh::box3 bbx;
		trimesh::vec3 bbx_center;
		trimesh::TriMesh* _return_mesh;
		trimesh::TriMesh* _return_surround_mesh;

		FontConfig m_config;
		FontMoveConfig m_moveConfig;

	};

}