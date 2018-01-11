#pragma  once

#pragma warning(disable: 4530)

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <d3d11.h>

enum GSDrawMode {GS_POINTS, GS_LINES, GS_LINESTRIP, GS_TRIANGLES, GS_TRIANGLESTRIP};

enum GSColorFormat {GS_UNKNOWNFORMAT, GS_ALPHA, GS_GRAYSCALE, GS_RGB, GS_RGBA, GS_BGR, GS_BGRA, GS_RGBA16F, GS_RGBA32F, GS_B5G5R5A1, GS_B5G6R5, GS_R10G10B10A2, GS_DXT1, GS_DXT3, GS_DXT5};

enum GSIndexType {GS_UNSIGNED_SHORT, GS_UNSIGNED_LONG};

enum GSBlendType {GS_BLEND_ZERO, GS_BLEND_ONE, GS_BLEND_SRCCOLOR, GS_BLEND_INVSRCCOLOR, GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, GS_BLEND_DSTCOLOR, GS_BLEND_INVDSTCOLOR, GS_BLEND_DSTALPHA, GS_BLEND_INVDSTALPHA, GS_BLEND_FACTOR, GS_BLEND_INVFACTOR};

enum GSSampleFilter
{
    GS_FILTER_LINEAR,
    GS_FILTER_POINT,
    GS_FILTER_ANISOTROPIC,
    GS_FILTER_MIN_MAG_POINT_MIP_LINEAR,
    GS_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
    GS_FILTER_MIN_POINT_MAG_MIP_LINEAR,
    GS_FILTER_MIN_LINEAR_MAG_MIP_POINT,
    GS_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    GS_FILTER_MIN_MAG_LINEAR_MIP_POINT,

    GS_FILTER_MIN_MAG_MIP_POINT=GS_FILTER_POINT,
    GS_FILTER_MIN_MAG_MIP_LINEAR=GS_FILTER_LINEAR
};

enum GSAddressMode
{
    GS_ADDRESS_CLAMP,
    GS_ADDRESS_WRAP,
    GS_ADDRESS_MIRROR,
    GS_ADDRESS_BORDER,
    GS_ADDRESS_MIRRORONCE,

    GS_ADDRESS_NONE=GS_ADDRESS_CLAMP,
    GS_ADDRESS_REPEAT=GS_ADDRESS_WRAP
};

enum GSImageFormat
{
	GS_IMAGEFORMAT_A8,
	GS_IMAGEFORMAT_L8,
	GS_IMAGEFORMAT_RGB,
	GS_IMAGEFORMAT_RGBX,
	GS_IMAGEFORMAT_RGBA,
	GS_IMAGEFORMAT_RGBA16F,
	GS_IMAGEFORMAT_RGBA32F,
	GS_IMAGEFORMAT_BGR,
	GS_IMAGEFORMAT_BGRX,
	GS_IMAGEFORMAT_BGRA,
};

enum ShaderParameterType
{
	Parameter_Unknown,
	Parameter_Bool,
	Parameter_Float,
	Parameter_Int,
	Parameter_String,
	Parameter_Vector2,
	Parameter_Vector3,
	Parameter_Vector = Parameter_Vector3,
	Parameter_Vector4,
	Parameter_Matrix3x3,
	Parameter_Matrix,
	Parameter_Texture
};

enum ShaderType
{
	ShaderType_Vertex,
	ShaderType_Pixel,
	ShaderType_Geometry
};

enum DuplicatorInfo
{
	DuplicatorInfo_Error,
	DuplicatorInfo_Timeout,
	DuplicatorInfo_Lost,
	DuplicatorInfo_Acquired
};

struct VBData
{
    List<Vect>      VertList;
    List<Vect>      NormalList;
    List<DWORD>     ColorList;
    List<Vect>      TangentList;
	//List<List<UVCoord>> UVList;
	List<List<UVWCoord>> UVList;

    inline VBData() {}
    inline ~VBData() {Clear();}
    inline void Clear()
    {
        VertList.Clear();
        NormalList.Clear();
        ColorList.Clear();
        TangentList.Clear();
        for(DWORD i=0;i<UVList.Num();i++)
            UVList[i].Clear();
        UVList.Clear();
    }
    inline void Serialize(Serializer &s)
    {
        Vect::SerializeList(s, VertList);
        Vect::SerializeList(s, NormalList);
        s << ColorList;
        Vect::SerializeList(s, TangentList);
        DWORD dwSize;
        if(s.IsLoading())
        {
            s << dwSize;
            UVList.SetSize(dwSize);
        }
        else
        {
            dwSize = UVList.Num();
            s << dwSize;
        }
        for(DWORD i=0; i<dwSize; i++)
            s << UVList[i];
    }
};

struct SamplerInfo
{
    inline SamplerInfo()
    {
        zero(this, sizeof(SamplerInfo));
        maxAnisotropy = 16;
    }
    GSSampleFilter filter;
    GSAddressMode addressU;
    GSAddressMode addressV;
    GSAddressMode addressW;
    UINT maxAnisotropy;
    Color4 borderColor;
};

struct ShaderParameterInfo
{
	String name;
	ShaderParameterType type;
};

class  SamplerState
{
public:
    virtual ~SamplerState() {}
};

class  Texture
{
public:
	virtual ~Texture() {}
};

class  Shader
{
protected:
    HANDLE hViewProj;
    inline HANDLE  GetViewProj() const {return hViewProj;}
public:
    virtual ~Shader() {}
    virtual HANDLE GetParameter(UINT parameter) const=0;
    virtual HANDLE GetParameterByName(CTSTR lpName) const=0;

    virtual void   SetBool(HANDLE hObject, BOOL bValue)=0;
    virtual void   SetFloat(HANDLE hObject, float fValue)=0;
    virtual void   SetInt(HANDLE hObject, int iValue)=0;
    virtual void   SetMatrix(HANDLE hObject, float *matrix)=0;
    virtual void   SetVector(HANDLE hObject, const Vect &value)=0;
    virtual void   SetVector2(HANDLE hObject, const Vect2 &value)=0;
    virtual void   SetVector4(HANDLE hObject, const Vect4 &value)=0;
    virtual void   SetTexture(HANDLE hObject, Texture *texture)=0;
    virtual void   SetValue(HANDLE hObject, const void *val, DWORD dwSize)=0;

    inline  void   SetColor(HANDLE hObject, const Color4 &value)
    {
        SetVector4(hObject, value);
    }
    inline  void   SetColor(HANDLE hObject, float fR, float fB, float fG, float fA=1.0f)
    {
        SetVector4(hObject, Color4(fR, fB, fG, fA));
    }
    inline  void   SetColor(HANDLE hObject, DWORD color)
    {
        SetVector4(hObject, RGBA_to_Vect4(color));
    }
    inline  void   SetColor3(HANDLE hObject, const Color3 &value)
    {
        SetVector(hObject, value);
    }
    inline  void   SetColor3(HANDLE hObject, float fR, float fB, float fG)
    {
        SetVector(hObject, Color3(fR, fB, fG));
    }
    inline  void   SetColor3(HANDLE hObject, DWORD color)
    {
        SetVector(hObject, RGB_to_Vect(color));
    }
    inline  void   SetVector4(HANDLE hObject, float fX, float fY, float fZ, float fW)
    {
        SetVector4(hObject, Vect4(fX, fY, fZ, fW));
    }
    inline  void   SetVector(HANDLE hObject, float fX, float fY, float fZ)
    {
        SetVector(hObject, Vect(fX, fY, fZ));
    }
    inline void SetMatrix(HANDLE hObject, const Matrix &mat)
    {
        float out[16];
        Matrix4x4Convert(out, mat);
        SetMatrix(hObject, out);
    }
    inline void SetMatrixIdentity(HANDLE hObject)
    {
        float out[16];
        Matrix4x4Identity(out);
        SetMatrix(hObject, out);
    }
};

using ShaderBlob = std::vector<char>;
template <Shader *CreateShader(ShaderBlob const&, CTSTR lpShader, CTSTR lpFileName)>
class  FutureShader
{
	friend class GraphicsSystem;

    ShaderBlob const *shaderData = nullptr;
    std::wstring const *fileData = nullptr;
    std::wstring fileName;
    std::unique_ptr<Shader> shader;
    HANDLE shaderReadyEvent;
    bool isReady = false;

    FutureShader(FutureShader const&) = delete;
    FutureShader &operator=(FutureShader const&) = delete;
    FutureShader(HANDLE event_, ShaderBlob const &data, std::wstring const &fileData, std::wstring fileName) : shaderData(&data), fileData(&fileData), fileName(fileName), shaderReadyEvent(event_) {}
public:
    FutureShader() {}
    FutureShader(FutureShader &&other)
        : shaderData(other.shaderData), fileData(other.fileData), fileName(std::move(other.fileName)), shader(std::move(other.shader)), shaderReadyEvent(other.shaderReadyEvent),
          isReady(other.isReady)
    {
        other.shaderData = nullptr;
        other.isReady = false;
    }
    FutureShader &operator=(FutureShader &&other)
    {
        shaderData = other.shaderData;
        other.shaderData = nullptr;
        fileData = other.fileData;
        fileName = other.fileName;
        shader = std::move(other.shader);
        shaderReadyEvent = other.shaderReadyEvent;
        isReady = other.isReady;
        return *this;
    }
    inline Shader *Shader()
    {
        if (!shaderData)
            return nullptr;
        if (!isReady)
        {
            if (WaitForSingleObject(shaderReadyEvent, 0) != WAIT_OBJECT_0)
                return shader.get();

            shader.reset(CreateShader(*shaderData, fileData->c_str(), fileName.c_str()));
            isReady = true;
        }
        return shader.get();
    }
};

inline Shader* CreateVertexShaderFromBlob(ShaderBlob const& blob, CTSTR lpShader, CTSTR lpFileName);
inline Shader* CreatePixelShaderFromBlob(ShaderBlob const& blob, CTSTR lpShader, CTSTR lpFileName);

using FutureVertexShader = FutureShader<CreateVertexShaderFromBlob>;
using FuturePixelShader = FutureShader<CreatePixelShaderFromBlob>;

struct FutureShaderContext
{
	ShaderBlob shaderData;
	std::unique_ptr<void, EventDeleter> readyEvent;
	std::unique_ptr<void, ThreadDeleter<>> thread;
	std::wstring fileData, fileName;
};

struct FutureShaderContainer
{
	std::map<std::wstring, FutureShaderContext> contexts;
	std::unique_ptr<void, MutexDeleter> lock;
	FutureShaderContainer() : lock(OSCreateMutex()) {}
};

class  OutputDuplicator
{
public:
    virtual ~OutputDuplicator() {}
    virtual DuplicatorInfo AcquireNextFrame(UINT timeout)=0;
    virtual Texture* GetCopyTexture()=0;
    virtual Texture* GetCursorTex(POINT* pos)=0;
};

class  GraphicsSystem
{
    friend class BLive;
public:
    GraphicsSystem();
    virtual ~GraphicsSystem();

    virtual LPVOID GetDevice()=0;
    virtual LPVOID GetContext()=0;

    void  MatrixRotate(float x, float y, float z, float a);
    void  MatrixRotate(const AxisAngle &aa);
    void  MatrixRotate(const Quat &q);
    void  MatrixTranslate(float x, float y);
    void  MatrixTranslate(const Vect2 &pos);
    void  MatrixScale(const Vect2 &scale);
    void  MatrixScale(float x, float y);
    void  MatrixTranspose();
	virtual void ResetViewMatrix() = 0;

    virtual Texture*        CreateTexture(unsigned int width, unsigned int height, GSColorFormat colorFormat, void *lpData, BOOL bBuildMipMaps, BOOL bStatic)=0;
	virtual Texture*        CreateTextureFromFile(CTSTR lpFile, BOOL bBuildMipMaps,UINT &Width, UINT& Height) = 0;
    virtual Texture*        CreateRenderTarget(unsigned int width, unsigned int height, GSColorFormat colorFormat, BOOL bGenMipMaps)=0;
    virtual Texture*        CreateGDITexture(unsigned width, unsigned int height)=0;
	virtual Texture*        CreateTextureRead(unsigned width, unsigned int height) = 0;

    virtual SamplerState*   CreateSamplerState(SamplerInfo &info)=0;
    virtual UINT            GetNumOutputs()=0;
    virtual OutputDuplicator *CreateOutputDuplicator(UINT outputID)=0;

    virtual Shader*         CreateVertexShaderFromFile(CTSTR lpFileName) = 0;
    virtual Shader*         CreatePixelShaderFromFile(CTSTR lpFileName) = 0;
    FuturePixelShader       CreatePixelShaderFromFileAsync(CTSTR fileName);

    virtual void  LoadTexture(Texture *texture, UINT idTexture=0)=0;
    virtual void  LoadSamplerState(SamplerState *sampler, UINT idSampler=0)=0;
    virtual void  LoadVertexShader(Shader *vShader)=0;
    virtual void  LoadPixelShader(Shader *pShader)=0;
    virtual Shader* GetCurrentPixelShader()=0;
    virtual Shader* GetCurrentVertexShader()=0;

	virtual void  initD3D(UINT renderFrameWidth, UINT renderFrameHeight, HWND hwndRenderFrame, UINT adapterID) = 0;
	virtual void  unInitD3D() = 0;
    virtual void  SetRenderTarget(Texture *texture)=0;
    virtual void  Draw(GSDrawMode drawMode, DWORD startVert=0, DWORD nVerts=0)=0;

    virtual void  EnableBlending(BOOL bEnable)=0;
    virtual void  BlendFunction(GSBlendType srcFactor, GSBlendType destFactor, float fFactor=1.0f)=0;

    virtual void  ClearRenderTarget(DWORD color=0xFF000000)=0;

    void DrawSprite(Texture *texture, DWORD color, float x, float y, float x2, float y2);
    virtual void DrawSpriteEx(Texture *texture, DWORD color, float x, float y, float x2, float y2, float u, float v, float u2, float v2)=0;
	virtual void DrawSpriteExRotate(Texture *texture, DWORD color, float x, float y, float x2, float y2, float degrees, float u, float v, float u2, float v2, float texDegrees) = 0;
    virtual void DrawBox(const Vect2 &upperLeft, const Vect2 &size)=0;
    virtual void SetCropping(float top, float left, float bottom, float right)=0;

    virtual void  Ortho(float left, float right, float top, float bottom, float znear, float zfar)=0;
    virtual void  SetViewport(float x, float y, float width, float height)=0;
    virtual void  SetScissorRect(XRect *pRect=NULL)=0;
protected:
    List<Matrix> MatrixStack;
    int curMatrix;
	FutureShaderContainer futureShaders;

public:
	virtual void CopyTexture(Texture *texDest, Texture *texSrc) = 0;
	virtual HRESULT Map(Texture *tex, BYTE *&lpData, UINT &pitch, D3D11_MAP map_type) = 0;
	virtual void Unmap(Texture *tex) = 0;
	virtual void SetImage(Texture *tex, void *lpData, GSImageFormat imageFormat, UINT pitch) = 0;
	virtual void GetTextureWH(Texture* tex, DWORD& width, DWORD& height) = 0;
	virtual bool GetTextureDC(Texture* tex, HDC &hDC) = 0;
	virtual void ReleaseTextureDC(Texture* tex) = 0;
	virtual void Flush() = 0;
	virtual HRESULT GetDeviceRemovedReason() = 0;
	virtual void Present(Texture *TextSwap) = 0;

	virtual void ResizeView(Texture *TextView) = 0;
	virtual void UnloadAllData() = 0;

	virtual Texture * CreateRenderTargetSwapChain(HWND Hwnd, UINT Width, UINT Height) = 0;

    virtual Shader *CreateVertexShaderFromBlob(ShaderBlob const &blob, CTSTR lpShader, CTSTR lpFileName) = 0;
    virtual Shader *CreatePixelShaderFromBlob(ShaderBlob const &blob, CTSTR lpShader, CTSTR lpFileName) = 0;
private:
    virtual void CreateVertexShaderBlob(ShaderBlob &blob, CTSTR lpShader, CTSTR lpFileName) = 0;
    virtual void CreatePixelShaderBlob(ShaderBlob &blob, CTSTR lpShader, CTSTR lpFileName) = 0;
};

 extern GraphicsSystem *GS;

inline void  MatrixRotate(float x, float y, float z, float a)   {GS->MatrixRotate(x, y, z, a);}  //axis angle
inline void  MatrixRotate(const AxisAngle &aa)                  {GS->MatrixRotate(aa);}
inline void  MatrixRotate(const Quat &q)                        {GS->MatrixRotate(q);}
inline void  MatrixTranslate(float x, float y)                  {GS->MatrixTranslate(x, y);}
inline void  MatrixTranslate(const Vect2 &pos)                  {GS->MatrixTranslate(pos);}
inline void  MatrixScale(const Vect2 &scale)                    {GS->MatrixScale(scale);}
inline void  MatrixScale(float x, float y)                      {GS->MatrixScale(x, y);}
inline void  MatrixTranspose()                                  {GS->MatrixTranspose();}

inline Shader* CreateVertexShaderFromBlob(ShaderBlob const& blob, CTSTR lpShader, CTSTR lpFileName) {return GS->CreateVertexShaderFromBlob(blob, lpShader, lpFileName);}
inline Shader* CreatePixelShaderFromBlob(ShaderBlob const& blob, CTSTR lpShader, CTSTR lpFileName)  {return GS->CreatePixelShaderFromBlob(blob, lpShader, lpFileName);}
inline FuturePixelShader CreatePixelShaderFromFileAsync(CTSTR fileName) {return GS->CreatePixelShaderFromFileAsync(fileName);}

inline void  Draw(GSDrawMode drawMode, DWORD StartVert=0, DWORD nVerts=0) {GS->Draw(drawMode, StartVert, nVerts);}

