#include "fstream"
#include "STEP.hpp"
#include <string>

#include "STEPCAFControl_Reader.hxx"
#include "BRepMesh_IncrementalMesh.hxx"
#include "Interface_Static.hxx"
#include "XCAFDoc_DocumentTool.hxx"
#include "XCAFDoc_ShapeTool.hxx"
#include "XCAFApp_Application.hxx"
#include "TopoDS_Solid.hxx"
#include "TopoDS_Compound.hxx"
#include "TopoDS_Builder.hxx"
#include "TopoDS.hxx"
#include "TDataStd_Name.hxx"
#include "BRepBuilderAPI_Transform.hxx"
#include "TopExp_Explorer.hxx"
#include "TopExp_Explorer.hxx"
#include "BRep_Tool.hxx"

#include "XCAFDoc_ColorType.hxx"
#include "Quantity_Color.hxx"
#include "XCAFDoc_DocumentTool.hxx"
#include "XCAFDoc_ColorTool.hxx"
#include "Quantity_ColorRGBA.hxx"
#include "TopTools_IndexedMapOfShape.hxx"
#include <map>

const double STEP_TRANS_CHORD_ERROR = 0.0025;
const double STEP_TRANS_ANGLE_RES = 0.5;


namespace cadcore {

    class  Color
    {
    public:
        /**
         * Defines the color as (R,G,B,A) whereas all values are in the range [0,1].
         * \a A defines the transparency.
         */
        explicit Color(float R = 0.0, float G = 0.0, float B = 0.0, float A = 0.0)
            :r(R), g(G), b(B), a(A) {}
        /**
         * Does basically the same as the constructor above unless that (R,G,B,A) is
         * encoded as an unsigned int.
         */
        Color(uint32_t rgba)
        {
            setPackedValue(rgba);
        }
        /** Copy constructor. */
        Color(const Color& c)
            :r(c.r), g(c.g), b(c.b), a(c.a) {}
        /** Returns true if both colors are equal. Therefore all components must be equal. */
        bool operator==(const Color& c) const
        {
            return getPackedValue() == c.getPackedValue();
            //return (c.r==r && c.g==g && c.b==b && c.a==a);
        }
        bool operator!=(const Color& c) const
        {
            return !operator==(c);
        }
        /**
         * Defines the color as (R,G,B,A) whereas all values are in the range [0,1].
         * \a A defines the transparency, 0 means complete opaque and 1 invisible.
         */
        void set(float R, float G, float B, float A = 0.0)
        {
            r = R; g = G; b = B; a = A;
        }
        Color& operator=(const Color& c)
        {
            r = c.r; g = c.g; b = c.b; a = c.a;
            return *this;
        }
        /**
         * Sets the color value as a 32 bit combined red/green/blue/alpha value.
         * Each component is 8 bit wide (i.e. from 0x00 to 0xff), and the red
         * value should be stored leftmost, like this: 0xRRGGBBAA.
         *
         * \sa getPackedValue().
         */
        Color& setPackedValue(uint32_t rgba)
        {
            this->set((rgba >> 24) / 255.0f,
                ((rgba >> 16) & 0xff) / 255.0f,
                ((rgba >> 8) & 0xff) / 255.0f,
                (rgba & 0xff) / 255.0f);
            return *this;
        }
        /**
         * Returns color as a 32 bit packed unsigned int in the form 0xRRGGBBAA.
         *
         *  \sa setPackedValue().
         */
        uint32_t getPackedValue() const
        {
            return ((uint32_t)(r * 255.0f + 0.5f) << 24 |
                (uint32_t)(g * 255.0f + 0.5f) << 16 |
                (uint32_t)(b * 255.0f + 0.5f) << 8 |
                (uint32_t)(a * 255.0f + 0.5f));
        }
        /**
         * creates FC Color from template type, e.g. Qt QColor
         */
        template <typename T>
        void setValue(const T& q)
        {
            set(q.redF(), q.greenF(), q.blueF());
        }
        /**
         * returns a template type e.g. Qt color equivalent to FC color
         *
         */
        template <typename T>
        inline T asValue(void) const {
            return(T(int(r * 255.0), int(g * 255.0), int(b * 255.0)));
        }
        /**
         * returns color as hex color "#RRGGBB"
         *
         */
        std::string asHexString() const {
            std::stringstream ss;
            ss << "#" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << int(r * 255.0)
                << std::setw(2) << int(g * 255.0)
                << std::setw(2) << int(b * 255.0);
            return ss.str();
        }
        /**
         * \deprecated
         */
        std::string asCSSString() const {
            return asHexString();
        }
        /**
         * gets color from hex color "#RRGGBB"
         *
         */
        bool fromHexString(const std::string& hex) {
            if (hex.size() < 7 || hex[0] != '#')
                return false;
            // #RRGGBB
            if (hex.size() == 7) {
                std::stringstream ss(hex);
                unsigned int rgb;
                char c;

                ss >> c >> std::hex >> rgb;
                int rc = (rgb >> 16) & 0xff;
                int gc = (rgb >> 8) & 0xff;
                int bc = rgb & 0xff;

                r = rc / 255.0f;
                g = gc / 255.0f;
                b = bc / 255.0f;

                return true;
            }
            // #RRGGBBAA
            if (hex.size() == 9) {
                std::stringstream ss(hex);
                unsigned int rgba;
                char c;

                ss >> c >> std::hex >> rgba;
                int rc = (rgba >> 24) & 0xff;
                int gc = (rgba >> 16) & 0xff;
                int bc = (rgba >> 8) & 0xff;
                int ac = rgba & 0xff;

                r = rc / 255.0f;
                g = gc / 255.0f;
                b = bc / 255.0f;
                a = ac / 255.0f;

                return true;
            }

            return false;
        }

        /// color values, public accessible
        float r, g, b, a;
    };

struct NamedSolid {
    NamedSolid(const TopoDS_Shape& s,
               const std::string& n) : solid{s}, name{n} {}
    const TopoDS_Shape solid;
    const std::string  name;
};

static void getNamedSolids(const TopLoc_Location& location, const std::string& prefix,
                           unsigned int& id, const Handle(XCAFDoc_ShapeTool) shapeTool,
                           const TDF_Label label, std::vector<NamedSolid>& namedSolids) {
    TDF_Label referredLabel{label};
    if (shapeTool->IsReference(label))
        shapeTool->GetReferredShape(label, referredLabel);

    std::string name;
    Handle(TDataStd_Name) shapeName;
    if (referredLabel.FindAttribute(TDataStd_Name::GetID(), shapeName))
        name = TCollection_AsciiString(shapeName->Get()).ToCString();

    if (name == "")
        name = std::to_string(id++);
    std::string fullName{name};

    TopLoc_Location localLocation = location * shapeTool->GetLocation(label);
    TDF_LabelSequence components;
    if (shapeTool->GetComponents(referredLabel, components)) {
        for (Standard_Integer compIndex = 1; compIndex <= components.Length(); ++compIndex) {
            getNamedSolids(localLocation, fullName, id, shapeTool, components.Value(compIndex), namedSolids);
        }
    } else {
        TopoDS_Shape shape;
        shapeTool->GetShape(referredLabel, shape);
        TopAbs_ShapeEnum shape_type = shape.ShapeType();
        BRepBuilderAPI_Transform transform(shape, localLocation, Standard_True);
        switch (shape_type) {
        case TopAbs_COMPOUND:
            namedSolids.emplace_back(TopoDS::Compound(transform.Shape()), fullName);
            break;
        case TopAbs_COMPSOLID:
            namedSolids.emplace_back(TopoDS::CompSolid(transform.Shape()), fullName);
            break;
        case TopAbs_SOLID:
            namedSolids.emplace_back(TopoDS::Solid(transform.Shape()), fullName);
            break;
        default:
            break;
        }
    }
}

trimesh::TriMesh* load_step(const std::string& fileName,  ccglobal::Tracer* tracer)
{
    bool cb_cancel = false;
    std::string file_after_preprocess = std::string(fileName);

    std::vector<NamedSolid> namedSolids;
    std::vector<TopLoc_Location> namedLables;
    Handle(TDocStd_Document) document;
    Handle(XCAFApp_Application) application = XCAFApp_Application::GetApplication();
    application->NewDocument(file_after_preprocess.c_str(), document);
    STEPCAFControl_Reader reader;
    reader.SetNameMode(true);
    //BBS: Todo, read file is slow which cause the progress_bar no update and gui no response
    IFSelect_ReturnStatus stat = reader.ReadFile(file_after_preprocess.c_str());
    if (stat != IFSelect_RetDone || !reader.Transfer(document)) {
        application->Close(document);

        return nullptr;
    }
    bool interuptted = false;


    Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(document->Main());
    TDF_LabelSequence topLevelShapes;
    shapeTool->GetFreeShapes(topLevelShapes);
    //int AXXX = topLevelShapes.Length();
    unsigned int id{1};
    Standard_Integer topShapeLength = topLevelShapes.Length() + 1;

    Handle(XCAFDoc_ColorTool) rootColorTool = XCAFDoc_DocumentTool::ColorTool(document->Main());
    TDF_Label  lab(document->Main().FindChild(0,Standard_True));
    //Quantity_ColorRGBA col();
    //(const Quantity_Color & col, TDF_Label & lab) const;
    Quantity_ColorRGBA colRGBA;
    Quantity_Color col;
    //lab = rootColorTool->FindColor(col);
    //rootColorTool.GetColor();


    TDF_LabelSequence tdfLabels;
    shapeTool->GetShapes(tdfLabels);   //获取装配体和组件对应名称
    int Roots = tdfLabels.Length();
    Color color0(0.8f, 0.8f, 0.8f);
    Color color1(0.8f, 0.8f, 0.8f);
    std::map <std::string, Color> shapesColors;
    std::map<std::string, std::vector< Color>> faceColors;
    bool is_shapeColor = false;  //目前分两种状态，体颜色（整个模型由许多子模型构成，每个子模型由许多颜色组成），面颜色（即一个体的面颜色不一样，这个由yi添加，后续可能需要调整）
    //std::vector<Color> faceColors;
    for (int i = 1; i <= Roots; i++)
    {
        TDF_Label label = tdfLabels.Value(i);
        bool colorcolor = rootColorTool->IsSet(label, XCAFDoc_ColorGen);
        bool colorcolor1 = rootColorTool->IsSet(label, XCAFDoc_ColorSurf);
        bool colorcolor2 = rootColorTool->IsSet(label, XCAFDoc_ColorCurv);
        if (colorcolor || colorcolor2 || colorcolor2)
            is_shapeColor = true;
        //Standard_Boolean GetColor(const TopoDS_Shape & S, const XCAFDoc_ColorType type, TDF_Label & colorL);
        Standard_Boolean xxsss5 = rootColorTool->GetColor(label, XCAFDoc_ColorGen, col);
        Standard_Boolean xxsss6 = rootColorTool->GetColor(label, XCAFDoc_ColorSurf, col);
        Standard_Boolean xxsss7 = rootColorTool->GetColor(label, XCAFDoc_ColorCurv, col);
        color0.r = (float)col.Red();
        color0.g = (float)col.Green();
        color0.b = (float)col.Blue();
        //TopoDS_Shape subshape =  shapeTool->GetShape(label);
        //subshape.TShape();
        TDF_Label referredLabel{ label };
        if (shapeTool->IsReference(label))
            shapeTool->GetReferredShape(label, referredLabel);

        std::string name;
        Handle(TDataStd_Name) shapeName;
        if (referredLabel.FindAttribute(TDataStd_Name::GetID(), shapeName))
            name = TCollection_AsciiString(shapeName->Get()).ToCString();

        if (name == "")
            name = std::to_string(id);
        
        shapesColors.emplace (name, color0);


        TopoDS_Shape shape;
        shapeTool->GetShape(referredLabel, shape);
        TopTools_IndexedMapOfShape faces;
        TopExp_Explorer xp(shape, TopAbs_FACE);
        while (xp.More()) {
            const TopoDS_Shape& af = xp.Current();
            faces.Add(af);
            xp.Next();
        }

        bool found_face_color = false;

        //faceColors.resize(faces.Extent());
        xp.Init(shape, TopAbs_FACE);
        std::vector< Color> clos;
        while (xp.More()) {

            //Standard_Boolean xxsss50 = rootColorTool->GetColor(xp.Current(), XCAFDoc_ColorGen, col);
            //Standard_Boolean xxsss60 = rootColorTool->GetColor(xp.Current(), XCAFDoc_ColorSurf, col);
            //Standard_Boolean xxsss70 = rootColorTool->GetColor(xp.Current(), XCAFDoc_ColorCurv, col);
            if (
                rootColorTool->GetColor(xp.Current(), XCAFDoc_ColorGen, col) ||
                rootColorTool->GetColor(xp.Current(), XCAFDoc_ColorSurf, col) ||
                rootColorTool->GetColor(xp.Current(), XCAFDoc_ColorCurv, col)) {
                is_shapeColor = false;
                int index = faces.FindIndex(xp.Current());
                color1.r = (float)col.Red();
                color1.g = (float)col.Green();
                color1.b = (float)col.Blue();
                clos.emplace_back(color1);
                found_face_color = true;
            }
            xp.Next();
        }

        if (clos.size() < faces.Extent())
        {
            while (clos.size() < faces.Extent())
            {
                clos.emplace_back(color0);
            }
        }



        faceColors.emplace(name, clos);
        clos.clear();
    }

    for (Standard_Integer iLabel = 1; iLabel < topShapeLength; ++iLabel) {
        getNamedSolids(TopLoc_Location{}, "", id, shapeTool, topLevelShapes.Value(iLabel), namedSolids);
    }

    std::vector<Color> color_submodule;
    color_submodule.resize(namedSolids.size());
    if (tracer)
        tracer->progress(0.01f);
    trimesh::TriMesh* tm = new  trimesh::TriMesh();
    long long face_all = 0;
    for (size_t i = 0; i < namedSolids.size(); i++) {
        for (TopExp_Explorer anExpSF(namedSolids[i].solid, TopAbs_FACE); anExpSF.More(); anExpSF.Next())
        {
            face_all ++;
        }
    }
    //tbb::parallel_for(tbb::blocked_range<size_t>(0, namedSolids.size()), [&](const tbb::blocked_range<size_t> &range) {
    for (size_t i = 0; i < namedSolids.size(); i++) {
        BRepMesh_IncrementalMesh mesh(namedSolids[i].solid, STEP_TRANS_CHORD_ERROR, false, STEP_TRANS_ANGLE_RES, true);
        // BBS: calculate total number of the nodes and triangles
        int aNbNodes     = 0;
        int aNbTriangles = 0;
        for (TopExp_Explorer anExpSF(namedSolids[i].solid, TopAbs_FACE); anExpSF.More(); anExpSF.Next()) {
            TopLoc_Location aLoc;
            TopoDS_Face face = TopoDS::Face(anExpSF.Current());
            Handle(Poly_Triangulation) aTriangulation = BRep_Tool::Triangulation(face, aLoc);
            if (!aTriangulation.IsNull()) {
                aNbNodes += aTriangulation->NbNodes();
                aNbTriangles += aTriangulation->NbTriangles();
            }
        }

        if (aNbTriangles == 0 || aNbNodes == 0)
            // BBS: No triangulation on the shape.
            continue;

        std::vector<trimesh::vec3> points;
        points.reserve(aNbNodes);
        // BBS: count faces missing triangulation
        Standard_Integer aNbFacesNoTri = 0;
        // BBS: fill temporary triangulation
        Standard_Integer aNodeOffset    = 0;
        Standard_Integer aTriangleOffet = 0; 
            
        std::vector< Color> fcs = faceColors.at(namedSolids[i].name);
        int  face_index = 0;
        for (TopExp_Explorer anExpSF(namedSolids[i].solid, TopAbs_FACE); anExpSF.More(); anExpSF.Next()) 
        {
            const TopoDS_Shape& aFace = anExpSF.Current();

            TopLoc_Location     aLoc;
            Handle(Poly_Triangulation) aTriangulation = BRep_Tool::Triangulation(TopoDS::Face(aFace), aLoc);
            if (aTriangulation.IsNull()) {
                ++aNbFacesNoTri;
                continue;
            }
            // BBS: copy nodes
            gp_Trsf aTrsf = aLoc.Transformation();
            for (Standard_Integer aNodeIter = 1; aNodeIter <= aTriangulation->NbNodes(); ++aNodeIter) {
                gp_Pnt aPnt = aTriangulation->Node(aNodeIter);
                aPnt.Transform(aTrsf);
                points.emplace_back(std::move(trimesh::vec3(aPnt.X(), aPnt.Y(), aPnt.Z())));
            }
            // BBS: copy triangles
            const TopAbs_Orientation anOrientation = anExpSF.Current().Orientation();
            Standard_Integer anId[3];
            for (Standard_Integer aTriIter = 1; aTriIter <= aTriangulation->NbTriangles(); ++aTriIter) {
                Poly_Triangle aTri = aTriangulation->Triangle(aTriIter);

                aTri.Get(anId[0], anId[1], anId[2]);
                if (anOrientation == TopAbs_REVERSED)
                    std::swap(anId[1], anId[2]);

                for (int j = 0; j < 3; j++)
                {
                    tm->vertices.emplace_back(points[anId[j] + aNodeOffset - 1]);

                }
                trimesh::Color c;
                if (is_shapeColor)  c = { shapesColors.at(namedSolids[i].name).r, shapesColors.at(namedSolids[i].name).g, shapesColors.at(namedSolids[i].name).b };
                else if (fcs.size() > 0)
                {
                    if (face_index >= fcs.size()) face_index = fcs.size() - 1;
                    c = { fcs.at(face_index).r, fcs.at(face_index).g, fcs.at(face_index).b };
                }
                tm->colors.push_back(c);

            }
            aNodeOffset += aTriangulation->NbNodes();
            aTriangleOffet += aTriangulation->NbTriangles();
            
            face_index++;
            if (tracer)
                tracer->formatMessage("range %d", face_index / face_all);
        }
    }
   // });

    if (tracer && tm->vertices.size() > 0)
    {

        tracer->progress(1.0f);
        tracer->success();
    }
    if (tracer && tm->vertices.size() == 0)
        tracer->failed("Parse File Error.");

    int fNum = (int)tm->vertices.size() / 3;
    for (int i = 0; i < fNum; ++i)
    {
        trimesh::TriMesh::Face face;
        face.x = 3 * i;
        face.y = 3 * i + 1;
        face.z = 3 * i + 2;
        tm->faces.push_back(face);
    }

    return tm;

}

}; // namespace Slic3r
