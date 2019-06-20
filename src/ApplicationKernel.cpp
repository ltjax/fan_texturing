
#include "ApplicationKernel.hpp"
#include "PageAtlas.hpp"
#include "TextureGenerator.hpp"
#include "TextureWriter.hpp"

#include <replay/bstream.hpp>
#include <replay/vector_math.hpp>
#include <replay/aabb.hpp>
#include <replay/pixbuf_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/assign.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "GLmm/GL.hpp"
#include "TSM.hpp"
#include "GLSLUtils.hpp"
#include "ImportObj.hpp"
//#include <SDL2/SDL_syswm.h>


namespace {
    const replay::matrix4 ClipspaceToTexturespaceMatrix(
        0.5f, 0.0f, 0.0f, 0.5f,
        0.0f, 0.5f, 0.0f, 0.5f,
        0.0f, 0.0f, 0.5f, 0.5f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

CApplicationKernel::CApplicationKernel(SDL_Window* Wnd, const boost::filesystem::path& Filename)
    : Window(Wnd), DrawCache(false), Width(1), Height(1), MouseX(0), MouseY(0),
    CacheScale(1.f / 16.f), ShowWireframe(false), DoDraw(false)
{
    std::cout << "Starting..\n" << std::endl;

    // Read the precreated testmesh
    namespace fs = boost::filesystem;

    fs::path CompleteFilename = fs::system_complete(Filename);
    std::vector<vec3> Vertices;
    std::vector<uint> Indices;
    LoadModel(CompleteFilename, Vertices, Indices);


    // Setup the cache drawing shader
    using namespace boost::assign;
    std::vector<float> QuadVertexAttrib;
    QuadVertexAttrib += 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f;
    QuadBuffer.SetData(GL_ARRAY_BUFFER, QuadVertexAttrib, GL_STATIC_DRAW);
    CompileShader(boost::filesystem::current_path() / "DrawCache.glsl", DrawCacheProgram);
    glBindAttribLocation(DrawCacheProgram.GetGLObject(), 0, "Vertex");
    DrawCacheProgram.Link();

    // Setup the camera
    Camera.SetAffinity(affinity(vec3(0.f, 0.f, 50.f)));

#ifdef RUNTIME_SHADOWS
    // Setup shadows
    ShadowTexture.SetImage(0, GL_DEPTH_COMPONENT24, 2048, 2048, 4,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
    ShadowTexture.SetFilter(GL_LINEAR, GL_LINEAR);
    ShadowTexture.SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    ShadowTexture.SetCompareMode(GL_COMPARE_R_TO_TEXTURE);
    ShadowTexture.SetCompareFunc(GL_LESS);

    ShadowFramebuffer.Bind();
    glReadBuffer(GL_NONE);
    glDrawBuffer(GL_NONE);
    ShadowFramebuffer.Attach(GL_DEPTH_ATTACHMENT, ShadowTexture, 0);
    ShadowFramebuffer.TestCompleteness();
    GLmm::Framebuffer::Unbind();


    GLMM_CHECK();
    CompileShader(fs::initial_path() / "Shadow.glsl", ShadowProgram);
    glBindAttribLocation(DrawCacheProgram.GetGLObject(), 0, "Coord");
    ShadowProgram.Link();
#endif

    Light.Setup(vec3(-0.8f, -0.2f, 0.7f), vec4(0.1f, 0.1f, 0.1f, 1.f), vec4(1.f));

    fs::path TextureFilename = CompleteFilename;
    TextureFilename.replace_extension(".fantex");
    // Setup the fantex mesh
    Mesh.Init(Vertices, Indices);
#if 1
    if (!boost::filesystem::exists(TextureFilename))
    {
        CTextureGenerator TextureGenerator(Mesh, Light);
        CTextureWriter Writer(Mesh, TextureGenerator, 1 << 7);
        Writer(TextureFilename);
    }
#endif

    Renderer.reset(new CFantexRenderer(Mesh, TextureFilename, 32));
}

CApplicationKernel::~CApplicationKernel()
{
}

namespace {
    void AddGroup(const std::string& GroupName, WavefrontObj::CModelGroup Group,
        std::vector<vec3>& Vertices, std::vector<uint>& Indices)
    {
        using std::size_t;
        std::cout << "Adding wavefront group: " << GroupName << std::endl;

        size_t BaseVertex = Vertices.size();

        Vertices.resize(BaseVertex + Group.VertexCount);

        for (size_t i = 0; i < Group.VertexCount; ++i)
        {
            Vertices[BaseVertex + i] = Group.VertexBuffer[i] * 10.f;
            Vertices[BaseVertex + i][0] = -Vertices[BaseVertex + i][0];
            std::swap(Vertices[BaseVertex + i][1], Vertices[BaseVertex + i][2]);
        }

        size_t BaseIndex = Indices.size();
        Indices.resize(BaseIndex + Group.IndexCount);
        /*for ( size_t j=0; j<Group.IndexCount; j+=3 )
        {
            Indices[BaseIndex+j]=Group.IndexBuffer[j]+BaseVertex;
            Indices[BaseIndex+j+1]=Group.IndexBuffer[j+2]+BaseVertex;
            Indices[BaseIndex+j+2]=Group.IndexBuffer[j+1]+BaseVertex;
        }*/
        for (size_t j = 0; j < Group.IndexCount; ++j)
        {
            Indices[BaseIndex + j] = Group.IndexBuffer[j] + BaseVertex;
        }

    }

}
void CApplicationKernel::LoadModel(const boost::filesystem::path& Filename,
    std::vector<vec3>& Vertices,
    std::vector<uint>& Indices)
{

    namespace fs = boost::filesystem;

    if (!fs::exists(Filename))
    {
        throw std::runtime_error("Unable to open requested mesh file: \"" + Filename.string() + '\"');
    }

    std::cout << "Loading model: " << Filename.string() << std::endl;

    // Raw format?
    if (Filename.extension() == ".mesh")
    {
        typedef replay::ibstream<fs::ifstream> BinaryStream;

        fs::ifstream _File(Filename, std::ios::binary);
        BinaryStream File(_File);

        uint VertexCount = 0, IndexCount = 0;
        File >> VertexCount >> IndexCount;

        Vertices.resize(VertexCount);
        vec3 DummyNormal;

        for (uint i = 0; i < VertexCount; ++i)
        {
            File.read(DummyNormal.ptr(), sizeof(vec3));
            File.read(Vertices[i].ptr(), sizeof(vec3));
        }

        Indices.resize(IndexCount);
        File.read(&(Indices[0]), IndexCount * sizeof(uint));
    }
    else if (Filename.extension() == ".obj")
    {
        WavefrontObj::Import(Filename,
            boost::bind(&AddGroup, _1, _2, boost::ref(Vertices), boost::ref(Indices)));

    }
    else
    {
        throw std::runtime_error("Unsupported model file type: " + Filename.extension().string());
    }

    std::cout << Vertices.size() << " vertices, " << (Indices.size() / 3) << " triangles." << std::endl;
}


void CApplicationKernel::OnMouseMove(SDL_MouseMotionEvent* msg)
{
    auto x = msg->x;
    auto y = Height - msg->y;

    Camera.OnMouseMove(x, y);

    MouseX = float(x);
    MouseY = float(y);


    if (Renderer->GetEditable())
    {
        // Invert the viewport transformation
        float x = (MouseX / Width) * 2.f - 1.f;
        float y = (MouseY / Height) * 2.f - 1.f;

        // 'Invert' the projection
        PointerDir[0] = x / Projection(0, 0);
        PointerDir[1] = y / Projection(1, 1);
        PointerDir[2] = -1.f;

        normalize(PointerDir);

        ray3 Ray(Camera.GetAffinity().position,
            matrix3(Camera.GetAffinity().orientation) * PointerDir);
        vec3 CursorPoint;

        // Check if we're hitting the mesh with our cursor
        if (Mesh.Trace(Ray, CursorPoint))
        {
            Cursor.SetPosition(CursorPoint);
            Cursor.SetVisible(true);
        }

        if (Cursor.GetVisible() && DoDraw)
        {
            bool Result = Renderer->DrawAt(Cursor.GetSphere(), Cursor.GetColor());

            if (!Result)
                std::cout << "Drawing failed!" << std::endl;
        }
    }
}

void CApplicationKernel::OnMouseButton(SDL_MouseButtonEvent* msg)
{
    if (msg->button != 1)
        Camera.OnMouseButton(msg->x, Height - msg->y, msg->button, msg->state == SDL_PRESSED);
    else
        DoDraw = msg->state == SDL_PRESSED;
}

void CApplicationKernel::OnKey(SDL_KeyboardEvent* Msg)
{
    auto Key = Msg->keysym.sym;
    auto Pressed = Msg->state == SDL_PRESSED;

    if (Key == SDL_GetKeyFromName("Space"))
    {
        DoDraw = Pressed;
        return;
    }

    // Ignore everything but KeyDown events
    if (!Pressed)
        return;

    // Quit the application?
    if (Key == SDL_GetKeyFromName("Escape"))
    {
        SDL_Quit();
        return;
    }

    if (Key == SDL_GetKeyFromName("Return"))
    {
        OnSelectColor();
        return;
    }

    // Draw the tile cache?
    if (Key == SDL_GetKeyFromName("c"))
    {
        DrawCache = !DrawCache;
    }

    if (Key == SDL_GetKeyFromName("e"))
    {
        bool IsEditable = !Renderer->GetEditable();
        Renderer->SetEditable(IsEditable);

        if (IsEditable)
        {
            std::cout << "Switched to editable mode." << std::endl;
        }
        else
        {
            std::cout << "Switched to render mode." << std::endl;
            Cursor.SetVisible(false);
        }
    }

    if (Key == SDL_GetKeyFromName("w"))
    {
        if (!WireframeRenderer)
            WireframeRenderer.reset(new CWireframeRenderer(Mesh));

        if (ShowWireframe)
        {
            if (!WireframeRenderer->GetOverlay())
            {
                WireframeRenderer->SetOverlay(true);
            }
            else
            {
                if (!WireframeRenderer->GetRenderTiles())
                    WireframeRenderer->SetRenderTiles(true);
                else
                    ShowWireframe = false;
            }
        }
        else
        {
            WireframeRenderer->SetOverlay(false);
            WireframeRenderer->SetRenderTiles(false);
            ShowWireframe = true;
        }
    }

    if (Key == SDL_GetKeyFromName("i"))
        CacheScale /= 2.f;

    if (Key == SDL_GetKeyFromName("k"))
        CacheScale *= 2.f;

    if (Key == SDL_GetKeyFromName("o"))
        Cursor.SetRadius(Cursor.GetRadius() * 1.2f);

    if (Key == SDL_GetKeyFromName("l"))
        Cursor.SetRadius(Cursor.GetRadius() / 1.2f);

    if (Key == SDL_GetKeyFromName("b"))
    {
        if (Renderer->GetSmoothBlend())
            std::cout << "Disabling smooth fan blending." << std::endl;
        else
            std::cout << "Enabling smooth fan blending." << std::endl;

        Renderer->SetSmoothBlend(!Renderer->GetSmoothBlend());
    }

    if (Key == SDL_GetKeyFromName("PageUp"))
    {
        float Bias = std::min(Renderer->GetBias() + 0.05f, 1.5f);
        Renderer->SetBias(Bias);

        std::cout << "Bias is: " << Bias << std::endl;

    }

    if (Key == SDL_GetKeyFromName("PageDown"))
    {
        float Bias = std::max(Renderer->GetBias() - 0.05f, 0.0f);
        Renderer->SetBias(Bias);

        std::cout << "Bias is: " << Bias << std::endl;
    }

    if (Key == SDL_GetKeyFromName("p"))
    {
        OnPrintScreen();
    }
}

void CApplicationKernel::OnResize(int w, int h)
{
    this->Width = w;
    this->Height = h;

    // Setup a vanilla perspective projection
    Projection = replay::math::make_perspective_matrix(
        70.f, float(w) / h, 1.0f, 4000.f);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(Projection.ptr());

    // Setup the viewport
    glViewport(0, 0, w, h);
    //glClearColor( 0.2f, 0.2f, 0.34f, 1.0f );
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

void CApplicationKernel::OnPrintScreen()
{
    using namespace boost::posix_time;
    using namespace boost::filesystem;

    // returns a time code formatted like YYYY-MM-DDTHH:MM:SS,fffffffff
    std::string DateCode = to_iso_extended_string(second_clock::local_time());

    // erase everything after the comma
    std::size_t i = DateCode.find(',');
    if (i != std::string::npos)
        DateCode.erase(i);

    // replace all the :
    boost::algorithm::replace_all(DateCode, ":", "-");


    GLmm::Texture2D Texture;
    Texture.CopyImage(0, GL_RGB, 0, 0, this->Width, this->Height);
    pixbuf_io::save_to_file(initial_path() / ("screenshot-" + DateCode + ".png"), *Texture.GetImage(0));
}

void CApplicationKernel::OnIdle()
{
    double StartTime = GetTime();

    matrix4 View = affinity::inverse(Camera.GetAffinity()).matrix();

    // Updates pages based on current position and the screen size
    CDisplayData DisplayData(Camera.GetAffinity().position, Projection * View, this->Width, this->Height);

    if (Cursor.GetVisible())
    {
        CSphere Sphere(Cursor.GetSphere());
        Renderer->UpdatePages(DisplayData, &Sphere);
    }
    else
    {
        Renderer->UpdatePages(DisplayData, 0);
    }
    GLMM_CHECK();

    // Clear the screen
    GLmm::Framebuffer::Unbind();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    //glClearColor( 0.2f, 0.2f, 0.34f, 1.0f );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup the view transformation
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(View.ptr());

#ifdef RUNTIME_SHADOWS
    // Render the fantex mesh to shadow map(s)
    matrix4 Warp = TSM::ComputeWarpingProjection(DisplayData.Frustum.data(),
        Light.GetViewMatrix(), 400.f, 0.8f);

    matrix4 ShadowRenderMatrix = Warp * Light.GetViewMatrix();
    ShadowFramebuffer.Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, 2048, 2048);
    ShadowProgram.Use();
    glPolygonOffset(2.0f, 8.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glUniformMatrix4fv(ShadowProgram.GetUniformLocation("Matrix"), 1, GL_FALSE,
        ShadowRenderMatrix.ptr());

    Renderer->RenderDepth(ShadowProgram);

    glDisable(GL_POLYGON_OFFSET_FILL);
    const matrix4 ShadowMatrix = ClipspaceToTexturespaceMatrix * ShadowRenderMatrix;
#else
    matrix4 ShadowMatrix;
    ShadowMatrix.set_identity();
#endif
    GLmm::Framebuffer::Unbind();
    glViewport(0, 0, Width, Height);

    GLMM_CHECK();

    if (ShowWireframe)
    {
        WireframeRenderer->Render(View, Projection);
    }
    else
    {
        // Render the fantex mesh
#ifdef RUNTIME_SHADOWS
        Renderer->Render(ShadowTexture, ShadowMatrix);
#else
        Renderer->Render();
#endif
    }

    GLmm::Program::Disable();

    // Render a cursor for an editable mesh
    Cursor.Render(View, Projection);

    // Optionally draw the cache for debugging
    if (DrawCache)
    {
        float o = 32.f;
        float w = 512.f;
        glDisable(GL_DEPTH_TEST);
        QuadBuffer.Bind(GL_ARRAY_BUFFER);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, NULL);

        DrawCacheProgram.Use();
        vec4 Box(o / this->Width, o / this->Height, (o + w) / this->Width, (o + w) / this->Height);
        glUniform4fv(DrawCacheProgram.GetUniformLocation("Range"), 1, Box.ptr());
        glUniform1i(DrawCacheProgram.GetUniformLocation("CacheTexture"), 0);
        glUniform3fv(DrawCacheProgram.GetUniformLocation("OffsetAndScale"), 1, vec3(0.f, 0.f, 1.f).ptr());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (MouseX > o && MouseX < o + w && MouseY > o && MouseY < o + w)
        {
            Box += vec4((o + w) / this->Width, 0.f, (o + w) / this->Width, 0.f);

            float ox = (MouseX - o) / w;
            float oy = (MouseY - o) / w;
            float s = CacheScale;

            glUniform3fv(DrawCacheProgram.GetUniformLocation("OffsetAndScale"), 1, vec3(ox - 0.5f * s, oy - 0.5f * s, s).ptr());
            glUniform4fv(DrawCacheProgram.GetUniformLocation("Range"), 1, Box.ptr());
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glDisableVertexAttribArray(0);
        GLmm::BufferObject::Unbind(GL_ARRAY_BUFFER);
        glEnable(GL_DEPTH_TEST);
    }

    double EndTime = GetTime();
    int FPS = (int)(1.0 / (EndTime - StartTime));
    std::ostringstream Str;
    Str << "Fan Texturing (bias: " << int(Renderer->GetBias() * 100.f) << " ,fps: " << FPS << ")" << std::endl;
    SDL_SetWindowTitle(Window, Str.str().c_str());
}

#define NOMINMAX
#include <windows.h>

void CApplicationKernel::OnSelectColor()
{
    //SDL_SysWMinfo wmInfo;
    //SDL_VERSION(&wmInfo.version);
    //SDL_GetWindowWMInfo(Window, &wmInfo);
    //HWND WindowHandle = wmInfo.info.win.window;

    //CHOOSECOLOR		DialogBox;
    //static COLORREF	CustomColors[16];

    //// Float component to byte component conversion
    //auto C = [](float x) { return static_cast<int>(x * 255.f); };
    //vec4 Color = Cursor.GetColor();

    //// Initialize CHOOSECOLOR 
    //ZeroMemory(&DialogBox, sizeof(DialogBox));
    //DialogBox.lStructSize = sizeof(DialogBox);
    //DialogBox.hwndOwner = WindowHandle;
    //DialogBox.lpCustColors = CustomColors;
    //DialogBox.rgbResult = RGB(C(Color[0]), C(Color[1]), C(Color[2]));
    //DialogBox.Flags = CC_FULLOPEN | CC_RGBINIT;

    //// Launch the color picking dialog
    //if (ChooseColor(&DialogBox) == TRUE)
    //{
    //    float s = 1.f / 255.f;
    //    Cursor.SetColor(vec4(
    //        GetRValue(DialogBox.rgbResult) * s,
    //        GetGValue(DialogBox.rgbResult) * s,
    //        GetBValue(DialogBox.rgbResult) * s, 1.f));
    //}
}