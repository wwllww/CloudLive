#pragma once

inline GSColorFormat ConvertGIBackBufferFormat(DXGI_FORMAT format)
{
    switch(format)
    {
    case DXGI_FORMAT_R10G10B10A2_UNORM: return GS_R10G10B10A2;
    case DXGI_FORMAT_R8G8B8A8_UNORM:    return GS_RGBA;
    case DXGI_FORMAT_B8G8R8A8_UNORM:    return GS_BGRA;
    case DXGI_FORMAT_B8G8R8X8_UNORM:    return GS_BGR;
    case DXGI_FORMAT_B5G5R5A1_UNORM:    return GS_B5G5R5A1;
    case DXGI_FORMAT_B5G6R5_UNORM:      return GS_B5G6R5;
    }
    return GS_UNKNOWNFORMAT;
}

std::string Wchar2Ansi(const std::wstring& strSrc);
class D3D10VertexShader;
class D3D10VertexBuffer 
{
    friend class D3D10System;
    friend class BLive;

    ID3D11Buffer *vertexBuffer = NULL;
    ID3D11Buffer *normalBuffer = NULL;
    ID3D11Buffer *colorBuffer = NULL;
	ID3D11Buffer *tangentBuffer = NULL;
    List<ID3D11Buffer*> UVBuffers;

    UINT vertexSize;
    UINT normalSize;
    UINT colorSize;
    UINT tangentSize;
    List<UINT> UVSizes;

    BOOL bDynamic;
    UINT numVerts;
    VBData *data;

	static D3D10VertexBuffer* CreateVertexBuffer(VBData *vbData, BOOL bStatic, D3D10System *D3DSystem);
    void MakeBufferList(D3D10VertexShader *vShader, List<ID3D11Buffer*> &bufferList, List<UINT> &strides) const;
	void FlushBuffers(D3D10System *D3DSystem);
	VBData* GetData();
public:
    virtual ~D3D10VertexBuffer();
};

class D3D10SamplerState : public SamplerState
{
    friend class D3D10System;
    friend class BLive;

	SamplerInfo info;
    ID3D11SamplerState *state;
	static SamplerState* CreateSamplerState(SamplerInfo &info, D3D10System *System);
public:
    ~D3D10SamplerState();
};

class D3D10Texture : public Texture
{
    friend class D3D10OutputDuplicator;
    friend class D3D10System;
	friend class CSLiveManager;

    ID3D11Texture2D          *texture = NULL;
    ID3D11ShaderResourceView *resource = NULL;
    ID3D11RenderTargetView   *renderTarget = NULL;
    UINT width, height;
	IDXGISwapChain      *SwapChain = NULL;
    GSColorFormat format;
	IDXGISurface1 *surface = NULL;
    bool bGDICompatible = false;
    bool bDynamic = false;

	static Texture* CreateTexture(unsigned int width, unsigned int height, GSColorFormat colorFormat, void *lpData, BOOL bGenMipMaps, BOOL bStatic, D3D10System *D3DSystem);
	static Texture* CreateTextureFromFile(CTSTR lpFile, BOOL bBuildMipMaps, UINT &Width, UINT& Height, D3D10System *D3DSystem);
	static Texture* CreateRenderTarget(unsigned int width, unsigned int height, GSColorFormat colorFormat, BOOL bGenMipMaps, D3D10System *D3DSystem);
	static Texture* CreateGDITexture(unsigned int width, unsigned int height, D3D10System *D3DSystem);
	static Texture* CreateTextureRead(unsigned int width, unsigned int height, D3D10System *D3DSystem);
public:
    ~D3D10Texture();
};

struct ShaderParam
{
    ShaderParameterType type;
    String name;
    UINT samplerID;
    UINT textureID;

    int arrayCount;
    List<BYTE> curValue;
    List<BYTE> defaultValue;
    BOOL bChanged;

    inline ~ShaderParam() {FreeData();}
    inline void FreeData()
    {
        name.Clear();
        curValue.Clear();
        defaultValue.Clear();
    }
};

struct ShaderSampler
{
    String name;
    SamplerState *sampler;
    inline ~ShaderSampler() {FreeData();}
    inline void FreeData()
    {
        name.Clear();
        delete sampler;
    }
};

struct ShaderProcessor : CodeTokenizer
{
	BOOL ProcessShader(CTSTR input, CTSTR filename, D3D10System *D3DSytem);
    BOOL AddState(SamplerInfo &info, String &stateName, String &stateVal);

    UINT nTextures;
    List<ShaderSampler> Samplers;
    List<ShaderParam>   Params;
    List<D3D11_INPUT_ELEMENT_DESC> generatedLayout;

    bool bHasNormals;
    bool bHasColors;
    bool bHasTangents;
    UINT numTextureCoords;

    inline ShaderProcessor()  {zero(this, sizeof(ShaderProcessor));}
    inline ~ShaderProcessor() {FreeData();}
    inline void FreeData()
    {
        UINT i;
        for(i=0; i<Samplers.Num(); i++)
            Samplers[i].FreeData();
        Samplers.Clear();
        for(i=0; i<Params.Num(); i++)
            Params[i].FreeData();
        Params.Clear();
    }
    inline UINT GetSamplerID(CTSTR lpSampler)
    {
        for(UINT i=0; i<Samplers.Num(); i++)
        {
            if(Samplers[i].name.Compare(lpSampler))
                return i;
        }
        return INVALID;
    }
};

class D3D10Shader : public Shader
{
    friend class D3D10System;
    friend class BLive;

    List<ShaderParam>   Params;
    List<ShaderSampler> Samplers;
    ID3D11Buffer *constantBuffer = NULL;
    UINT constantSize;
	D3D10System *System;
protected:
    bool ProcessData(ShaderProcessor &processor, CTSTR lpFileName);
    void UpdateParams();
	void LoadDefaults();
public:
	D3D10Shader(D3D10System *System);
    ~D3D10Shader();
    virtual HANDLE GetParameter(UINT parameter) const;
    virtual HANDLE GetParameterByName(CTSTR lpName) const;
    virtual void   SetBool(HANDLE hObject, BOOL bValue);
    virtual void   SetFloat(HANDLE hObject, float fValue);
    virtual void   SetInt(HANDLE hObject, int iValue);
    virtual void   SetMatrix(HANDLE hObject, float *matrix);
    virtual void   SetVector(HANDLE hObject, const Vect &value);
    virtual void   SetVector2(HANDLE hObject, const Vect2 &value);
    virtual void   SetVector4(HANDLE hObject, const Vect4 &value);
    virtual void   SetTexture(HANDLE hObject, Texture *texture);
    virtual void   SetValue(HANDLE hObject, const void *val, DWORD dwSize);
};

class D3D10VertexShader : public D3D10Shader
{
    friend class D3D10System;
    friend class D3D10VertexBuffer;
    friend class BLive;

    ID3D11VertexShader *vertexShader;
    ID3D11InputLayout  *inputLayout;
    bool bHasNormals;
    bool bHasColors;
    bool bHasTangents;
    UINT nTextureCoords;
    inline UINT NumBuffersExpected() const
    {
        UINT count = 1;
        if(bHasNormals)  count++;
        if(bHasColors)   count++;
        if(bHasTangents) count++;
        count += nTextureCoords;
        return count;
    }
	static Shader* CreateVertexShaderFromBlob(std::vector<char> const &blob, CTSTR lpShader, CTSTR lpFileName, D3D10System *D3DSytem);
	static Shader* CreateVertexShaderFromFile(CTSTR lpFileName, D3D10System *D3DSytem);
	static void CreateVertexShaderBlob(std::vector<char> &blob, CTSTR lpShader, CTSTR lpFileName, D3D10System *D3DSytem);
public:
	D3D10VertexShader(D3D10System *System) :D3D10Shader(System){};
    ~D3D10VertexShader();
};

class D3D10PixelShader : public D3D10Shader
{
    friend class D3D10System;

    ID3D11PixelShader *pixelShader;
	static Shader* CreatePixelShaderFromBlob(ShaderBlob const &blob, CTSTR lpShader, CTSTR lpFileName, D3D10System *System);
	static Shader* CreatePixelShaderFromFile(CTSTR lpFileName, D3D10System *System);
	static void CreatePixelShaderBlob(ShaderBlob &blob, CTSTR lpShader, CTSTR lpFileName, D3D10System *System);

public:
	D3D10PixelShader(D3D10System *System) :D3D10Shader(System){};
    ~D3D10PixelShader();
};

class D3D10OutputDuplicator : public OutputDuplicator
{
    IDXGIOutputDuplication *duplicator;
    Texture *copyTex;
    POINT cursorPos;
    Texture *cursorTex;
    BOOL bCursorVis;
	D3D10System *System;
public:
	D3D10OutputDuplicator(D3D10System *System);
	bool Init(UINT output);
    virtual ~D3D10OutputDuplicator();

    virtual DuplicatorInfo AcquireNextFrame(UINT timeout);
    virtual Texture* GetCopyTexture();
    virtual Texture* GetCursorTex(POINT* pos);
};

struct SavedBlendState
{
    GSBlendType srcFactor, destFactor;
    ID3D11BlendState *blendState;
};

class D3D10System : public GraphicsSystem
{
    friend class BLive;
    friend class D3D10VertexShader;
    friend class D3D10PixelShader;

    IDXGIFactory1           *factory = NULL;
    ID3D11Device            *d3d = NULL;
    ID3D11DeviceContext     *context = NULL;
    IDXGISwapChain          *swap = NULL;
	IDXGISwapChain			*projectorSwap = NULL;
    ID3D11RenderTargetView  *swapRenderView = NULL;
    ID3D11DepthStencilState *depthState = NULL;
    ID3D11RasterizerState   *rasterizerState = NULL;
    ID3D11RasterizerState   *scissorState = NULL;

    bool bDisableCompatibilityMode;

    D3D10Texture            *curRenderTarget = NULL;
	D3D10Texture            *curTextures[8];
	D3D10SamplerState       *curSamplers[8];
    D3D10VertexBuffer       *curVertexBuffer = NULL;
    D3D10VertexShader       *curVertexShader = NULL;
    D3D10PixelShader        *curPixelShader = NULL;

    D3D10_PRIMITIVE_TOPOLOGY curTopology;

    List<SavedBlendState>   blends;
    ID3D11BlendState        *curBlendState = NULL;
    ID3D11BlendState        *disabledBlend = NULL;
    BOOL                    bPreBlendEnabled;

	D3D10VertexBuffer            *spriteVertexBuffer = NULL, *boxVertexBuffer = NULL;

	float                   curProjMatrix[16];
	float                   curViewMatrix[16];
	float                   curViewProjMatrix[16];
	float                   curBlendFactor[4];
	float                   curCropping[4];

    virtual void ResetViewMatrix();

    virtual void CreateVertexShaderBlob(std::vector<char> &blob, CTSTR lpShader, CTSTR lpFileName);
    virtual void CreatePixelShaderBlob(std::vector<char> &blob, CTSTR lpShader, CTSTR lpFileName);
public:
    D3D10System();
    virtual ~D3D10System();

	virtual LPVOID GetDevice();
	virtual LPVOID GetContext();

    virtual Texture*        CreateTexture(unsigned int width, unsigned int height, GSColorFormat colorFormat, void *lpData, BOOL bBuildMipMaps, BOOL bStatic);
	virtual Texture*        CreateTextureFromFile(CTSTR lpFile, BOOL bBuildMipMaps,UINT &Width, UINT& Height);
    virtual Texture*        CreateRenderTarget(unsigned int width, unsigned int height, GSColorFormat colorFormat, BOOL bGenMipMaps);
    virtual Texture*        CreateGDITexture(unsigned int width, unsigned int height);
	virtual Texture*		CreateTextureRead(unsigned int width, unsigned int height);

    virtual SamplerState*   CreateSamplerState(SamplerInfo &info);

    virtual UINT            GetNumOutputs();
    virtual OutputDuplicator *CreateOutputDuplicator(UINT outputID);

    virtual Shader*         CreateVertexShaderFromBlob(ShaderBlob const &blob, CTSTR lpShader, CTSTR lpFileName) ;
    virtual Shader*         CreatePixelShaderFromBlob(ShaderBlob const &blob, CTSTR lpShader, CTSTR lpFileName) ;
	virtual Shader*         CreateVertexShaderFromFile(CTSTR lpFileName);
	virtual Shader*			CreatePixelShaderFromFile(CTSTR lpFileName);

	virtual D3D10VertexBuffer* CreateVertexBuffer(VBData *vbData, BOOL bStatic = 1);

	virtual void  LoadVertexBuffer(D3D10VertexBuffer* vb);
    virtual void  LoadTexture(Texture *texture, UINT idTexture=0);
    virtual void  LoadSamplerState(SamplerState *sampler, UINT idSampler=0);
    virtual void  LoadVertexShader(Shader *vShader);
    virtual void  LoadPixelShader(Shader *pShader);

    virtual Shader* GetCurrentPixelShader();
    virtual Shader* GetCurrentVertexShader();
	virtual void  initD3D(UINT adapterID);
	virtual void  initD3D(UINT renderFrameWidth, UINT renderFrameHeight, HWND hwndRenderFrame, UINT adapterID);
	virtual void  unInitD3D();
    virtual void  SetRenderTarget(Texture *texture);
    virtual void  Draw(GSDrawMode drawMode, DWORD startVert=0, DWORD nVerts=0);

    virtual void  EnableBlending(BOOL bEnable);
    virtual void  BlendFunction(GSBlendType srcFactor, GSBlendType destFactor, float fFactor);
    virtual void  ClearRenderTarget(DWORD color=0xFF000000);

    void  Ortho(float left, float right, float top, float bottom, float znear, float zfar);
    virtual void  SetViewport(float x, float y, float width, float height);
    virtual void  SetScissorRect(XRect *pRect=NULL);

    virtual void  DrawSpriteEx(Texture *texture, DWORD color, float x, float y, float x2, float y2, float u, float v, float u2, float v2);
	virtual void  DrawSpriteExRotate(Texture *texture, DWORD color, float x, float y, float x2, float y2, float degrees, float u, float v, float u2, float v2, float texDegrees);
    virtual void  DrawBox(const Vect2 &upperLeft, const Vect2 &size);
    virtual void  SetCropping(float left, float top, float right, float bottom);

    virtual void  CopyTexture(Texture *texDest, Texture *texSrc);
	virtual HRESULT  Map(Texture *tex, BYTE *&lpData, UINT &pitch, D3D11_MAP map_type = D3D11_MAP_READ);
	virtual void  Unmap(Texture *tex);
	virtual void  SetImage(Texture *tex, void *lpData, GSImageFormat imageFormat, UINT pitch);
	virtual void  GetTextureWH(Texture* tex, DWORD& width, DWORD& height);
	virtual bool  GetTextureDC(Texture* tex, HDC &hDC);
	virtual void  ReleaseTextureDC(Texture* tex);
	virtual void  Flush();
	virtual HRESULT GetDeviceRemovedReason();
	virtual void  Present(Texture *TextSwap);

	virtual void ResizeView(Texture *TextView);
	virtual void UnloadAllData();

    inline ID3D11Device *GetDeviceInline() const {return d3d;}
    inline ID3D11DeviceContext *GetContextInline() const {return context;}

	virtual Texture * CreateRenderTargetSwapChain(HWND Hwnd, UINT Width, UINT Height);
};


