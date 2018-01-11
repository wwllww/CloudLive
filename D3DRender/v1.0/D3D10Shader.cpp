
#include "D3DRender.h"

void D3D10Shader::LoadDefaults()
{
	for (UINT i = 0; i < Params.Num(); i++)
	{
		ShaderParam &param = Params[i];

		if (param.defaultValue.Num())
		{
			param.bChanged = TRUE;
			param.curValue.CopyList(param.defaultValue);
		}
	}
}

bool D3D10Shader::ProcessData(ShaderProcessor &processor, CTSTR lpFileName)
{
    Params.TransferFrom(processor.Params);
    Samplers.TransferFrom(processor.Samplers);

    constantSize = 0;
    for(UINT i=0; i<Params.Num(); i++)
    {
        ShaderParam &param = Params[i];

        switch(param.type)
        {
            case Parameter_Bool:
            case Parameter_Float:
            case Parameter_Int:         constantSize += sizeof(float); break;
            case Parameter_Vector2:     constantSize += sizeof(float)*2; break;
            case Parameter_Vector:      constantSize += sizeof(float)*3; break;
            case Parameter_Vector4:     constantSize += sizeof(float)*4; break;
            case Parameter_Matrix3x3:   constantSize += sizeof(float)*3*3; break;
            case Parameter_Matrix:      constantSize += sizeof(float)*4*4; break;
        }
    }

    if(constantSize)
    {
        D3D11_BUFFER_DESC bd;
        zero(&bd, sizeof(bd));

        bd.ByteWidth        = (constantSize+15)&0xFFFFFFF0; //align to 128bit boundry
        bd.Usage            = D3D11_USAGE_DYNAMIC;
        bd.BindFlags        = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;

        HRESULT err = GetD3D()->CreateBuffer(&bd, NULL, &constantBuffer);
        if(FAILED(err))
        {
            AppWarning(TEXT("Unable to create constant buffer for shader '%s', result = %08lX"), lpFileName, err);
            return false;
        }
    }

    LoadDefaults();

    return true;
}

void D3D10VertexShader::CreateVertexShaderBlob(ShaderBlob &blob, CTSTR lpShader, CTSTR lpFileName)
{
    D3D10System *d3d10Sys = static_cast<D3D10System*>(GS);
    LPCSTR lpVSType = d3d10Sys->bDisableCompatibilityMode ? "vs_4_0" : "vs_4_0_level_9_3";

    ComPtr<ID3D10Blob> errorMessages, shaderBlob;

    LPSTR lpAnsiShader = tstr_createUTF8(lpShader);
    LPSTR lpAnsiFileName = tstr_createUTF8(lpFileName);

    HRESULT err = D3DX11CompileFromMemory(lpAnsiShader, strlen(lpAnsiShader), lpAnsiFileName, NULL, NULL, "main", lpVSType, D3D10_SHADER_OPTIMIZATION_LEVEL3, 0, NULL, shaderBlob.Assign(), errorMessages.Assign(), NULL);

    Free(lpAnsiFileName);
    Free(lpAnsiShader);

    if (FAILED(err))
    {
        if (errorMessages)
        {
            if (errorMessages->GetBufferSize())
            {
                LPSTR lpErrors = (LPSTR)errorMessages->GetBufferPointer();
                Log(TEXT("Error compiling vertex shader '%s':\r\n\r\n%S\r\n"), lpFileName, lpErrors);
            }
        }

        CrashError(TEXT("Compilation of vertex shader '%s' failed, result = %08lX"), lpFileName, err);
        return;
    }

    blob.assign((char*)shaderBlob->GetBufferPointer(), (char*)shaderBlob->GetBufferPointer() + shaderBlob->GetBufferSize());
}

Shader* D3D10VertexShader::CreateVertexShaderFromBlob(ShaderBlob const &blob, CTSTR lpShader, CTSTR lpFileName)
{
    ShaderProcessor shaderProcessor;
    if (!shaderProcessor.ProcessShader(lpShader, lpFileName))
        AppWarning(TEXT("Unable to process vertex shader '%s'"), lpFileName); //don't exit, leave it to the actual shader compiler to tell the errors

    if (!blob.size())
        return nullptr;

    ComPtr<ID3D11VertexShader> vShader;
    ID3D11InputLayout *vShaderLayout;

    HRESULT err = GetD3D()->CreateVertexShader(&blob.front(), blob.size(), NULL, vShader.Assign());
    if (FAILED(err))
    {
        CrashError(TEXT("Unable to create vertex shader '%s', result = %08lX"), lpFileName, err);
        return NULL;
    }

    err = GetD3D()->CreateInputLayout(shaderProcessor.generatedLayout.Array(), shaderProcessor.generatedLayout.Num(), &blob.front(), blob.size(), &vShaderLayout);
    if (FAILED(err))
    {
        CrashError(TEXT("Unable to create vertex layout for vertex shader '%s', result = %08lX"), lpFileName, err);
        return NULL;
    }

    D3D10VertexShader *shader = new D3D10VertexShader;
    shader->vertexShader = vShader.Detach();
    shader->inputLayout = vShaderLayout;
    if (!shader->ProcessData(shaderProcessor, lpFileName))
    {
        delete shader;
        return NULL;
    }

    shader->bHasNormals = shaderProcessor.bHasNormals;
    shader->bHasColors = shaderProcessor.bHasColors;
    shader->bHasTangents = shaderProcessor.bHasTangents;
    shader->nTextureCoords = shaderProcessor.numTextureCoords;
    shader->hViewProj = shader->GetParameterByName(TEXT("ViewProj"));

    return shader;
}

Shader* D3D10VertexShader::CreateVertexShaderFromFile(CTSTR lpFileName)
{
	XFile ShaderFile;
	String fullPathFilename;

	String strDirectory;
// 	UINT dirSize = GetCurrentDirectory(0, 0);
// 	strDirectory.SetLength(dirSize);
// 	GetCurrentDirectory(dirSize, strDirectory.Array());

	strDirectory.SetLength(256);
	DWORD Len = GetModuleFileName(NULL, strDirectory.Array(), strDirectory.Length());
	strDirectory[Len] = '\0';

	while (Len > 0)
	{
		if (*(strDirectory.Array() + Len - 1) != '\\')
		{
			*(strDirectory.Array() + Len - 1) = '\0';
		}
		else
		{
			*(strDirectory.Array() + Len - 1) = '\0';
			break;
		}
		Len--;
	}

	if ((lpFileName[0] != '.' && lpFileName[0] != '/' && lpFileName[0] != '\\') && !(lpFileName[0] && lpFileName[1] == ':'))
		fullPathFilename << strDirectory << L"\\" << lpFileName;
	else
		fullPathFilename << lpFileName;

	if (!ShaderFile.Open(fullPathFilename, XFILE_READ | XFILE_SHARED, XFILE_OPENEXISTING))
		CrashError(TEXT("CreateVertexShaderFromFile: Couldn't open %s: %d"), lpFileName, GetLastError());

	String strShader;
	ShaderFile.ReadFileToString(strShader);
	if (strShader.IsEmpty())
		CrashError(TEXT("CreateVertexShaderFromFile: Couldn't read %s: %d"), lpFileName, GetLastError());

	ShaderBlob blob;
	CreateVertexShaderBlob(blob, strShader, lpFileName);
	return CreateVertexShaderFromBlob(blob, strShader, lpFileName);
}

void D3D10PixelShader::CreatePixelShaderBlob(ShaderBlob &blob, CTSTR lpShader, CTSTR lpFileName)
{
    D3D10System *d3d10Sys = static_cast<D3D10System*>(GS);
    LPCSTR lpPSType = d3d10Sys->bDisableCompatibilityMode ? "ps_4_0" : "ps_4_0_level_9_3";

    ComPtr<ID3D10Blob> errorMessages, shaderBlob;

    LPSTR lpAnsiShader = tstr_createUTF8(lpShader);
    LPSTR lpAnsiFileName = tstr_createUTF8(lpFileName);

    HRESULT err = D3DX11CompileFromMemory(lpAnsiShader, strlen(lpAnsiShader), lpAnsiFileName, NULL, NULL, "main", lpPSType, D3D10_SHADER_OPTIMIZATION_LEVEL3, 0, NULL, shaderBlob.Assign(), errorMessages.Assign(), NULL);

    Free(lpAnsiFileName);
    Free(lpAnsiShader);

    if (FAILED(err))
    {
        if (errorMessages)
        {
            if (errorMessages->GetBufferSize())
            {
                LPSTR lpErrors = (LPSTR)errorMessages->GetBufferPointer();
                Log(TEXT("Error compiling pixel shader '%s':\r\n\r\n%S\r\n"), lpFileName, lpErrors);
            }
        }

        CrashError(TEXT("Compilation of pixel shader '%s' failed, result = %08lX"), lpFileName, err);
        return;
    }

    blob.assign((char*)shaderBlob->GetBufferPointer(), (char*)shaderBlob->GetBufferPointer() + shaderBlob->GetBufferSize());
}

Shader *D3D10PixelShader::CreatePixelShaderFromBlob(ShaderBlob const &blob, CTSTR lpShader, CTSTR lpFileName)
{
    ShaderProcessor shaderProcessor;
    if (!shaderProcessor.ProcessShader(lpShader, lpFileName))
        AppWarning(TEXT("Unable to process pixel shader '%s'"), lpFileName); //don't exit, leave it to the actual shader compiler to tell the errors

    if (!blob.size())
        return nullptr;

    ID3D11PixelShader *pShader;
    HRESULT err = GetD3D()->CreatePixelShader(&blob.front(), blob.size(), NULL, &pShader);
    if (FAILED(err))
    {
        CrashError(TEXT("Unable to create pixel shader '%s', result = %08lX"), lpFileName, err);
        return NULL;
    }

    D3D10PixelShader *shader = new D3D10PixelShader;
    shader->pixelShader = pShader;
    if (!shader->ProcessData(shaderProcessor, lpFileName))
    {
        delete shader;
        return NULL;
    }

    return shader;
}

Shader* D3D10PixelShader::CreatePixelShaderFromFile(CTSTR lpFileName)
{
	XFile ShaderFile;
	String fullPathFilename;

	String strDirectory;
// 	UINT dirSize = GetCurrentDirectory(0, 0);
// 	strDirectory.SetLength(dirSize);
// 	GetCurrentDirectory(dirSize, strDirectory.Array());
	strDirectory.SetLength(256);
	DWORD Len = GetModuleFileName(NULL, strDirectory.Array(), strDirectory.Length());
	strDirectory[Len] = '\0';

	while (Len > 0)
	{
		if (*(strDirectory.Array() + Len - 1) != '\\')
		{
			*(strDirectory.Array() + Len - 1) = '\0';
		}
		else
		{
			*(strDirectory.Array() + Len - 1) = '\0';
			break;
		}
		Len--;
	}
	
	if ((lpFileName[0] != '.' && lpFileName[0] != '/' && lpFileName[0] != '\\') && !(lpFileName[0] && lpFileName[1] == ':'))
		fullPathFilename << strDirectory << L"\\" << lpFileName;
	else
		fullPathFilename << lpFileName;

	if (!ShaderFile.Open(fullPathFilename, XFILE_READ | XFILE_SHARED, XFILE_OPENEXISTING))
		CrashError(TEXT("CreatePixelShaderFromFile: Couldn't open %s: %d"), lpFileName, GetLastError());

	String strShader;
	ShaderFile.ReadFileToString(strShader);
	if (strShader.IsEmpty())
		CrashError(TEXT("CreatePixelShaderFromFile: Couldn't read %s: %d"), lpFileName, GetLastError());

	ShaderBlob blob;
	CreatePixelShaderBlob(blob, strShader, lpFileName);
	return CreatePixelShaderFromBlob(blob, strShader, lpFileName);
}

D3D10Shader::~D3D10Shader()
{
    for(UINT i=0; i<Samplers.Num(); i++)
        Samplers[i].FreeData();
    for(UINT i=0; i<Params.Num(); i++)
        Params[i].FreeData();

    SafeRelease(constantBuffer);
}

D3D10VertexShader::~D3D10VertexShader()
{
    SafeRelease(vertexShader);
    SafeRelease(inputLayout);
}

D3D10PixelShader::~D3D10PixelShader()
{
    SafeRelease(pixelShader);
}

HANDLE D3D10Shader::GetParameter(UINT parameter) const
{
    if(parameter >= Params.Num())
        return NULL;
    return (HANDLE)(Params+parameter);
}

HANDLE D3D10Shader::GetParameterByName(CTSTR lpName) const
{
    for(UINT i=0; i<Params.Num(); i++)
    {
        ShaderParam &param = Params[i];
        if(param.name == lpName)
            return (HANDLE)&param;
    }

    return NULL;
}

#define GetValidHandle() \
    ShaderParam *param = (ShaderParam*)hObject; \
    if(!hObject) \
        return;

void   D3D10Shader::SetBool(HANDLE hObject, BOOL bValue)
{
    GetValidHandle();

    BOOL bSizeChanged = param->curValue.SetSize(sizeof(BOOL));
    BOOL &curVal = *(BOOL*)param->curValue.Array();

    if(bSizeChanged || curVal != bValue)
    {
        curVal = bValue;
        param->bChanged = TRUE;
    }
}

void   D3D10Shader::SetFloat(HANDLE hObject, float fValue)
{
    GetValidHandle();

    BOOL bSizeChanged = param->curValue.SetSize(sizeof(float));
    float &curVal = *(float*)param->curValue.Array();

    if(bSizeChanged || curVal != fValue)
    {
        curVal = fValue;
        param->bChanged = TRUE;
    }
}

void   D3D10Shader::SetInt(HANDLE hObject, int iValue)
{
    GetValidHandle();

    BOOL bSizeChanged = param->curValue.SetSize(sizeof(int));
    int &curVal = *(int*)param->curValue.Array();

    if(bSizeChanged || curVal != iValue)
    {
        curVal = iValue;
        param->bChanged = TRUE;
    }
}

void   D3D10Shader::SetMatrix(HANDLE hObject, float *matrix)
{
    SetValue(hObject, matrix, sizeof(float)*4*4);
}

void   D3D10Shader::SetVector(HANDLE hObject, const Vect &value)
{
    SetValue(hObject, value.ptr, sizeof(float)*3);
}

void   D3D10Shader::SetVector2(HANDLE hObject, const Vect2 &value)
{
    SetValue(hObject, value.ptr, sizeof(Vect2));
}

void   D3D10Shader::SetVector4(HANDLE hObject, const Vect4 &value)
{
    SetValue(hObject, value.ptr, sizeof(Vect4));
}

void   D3D10Shader::SetTexture(HANDLE hObject, Texture *texture)
{
    GetValidHandle();

    BOOL bSizeChanged = param->curValue.SetSize(sizeof(const Texture*));
    const Texture *&curVal = *(const Texture**)param->curValue.Array();

    if(bSizeChanged || curVal != texture)
    {
        curVal = texture;
        param->bChanged = TRUE;
    }
}

void   D3D10Shader::SetValue(HANDLE hObject, const void *val, DWORD dwSize)
{
    GetValidHandle();

    BOOL bSizeChanged = param->curValue.SetSize(dwSize);

    if(bSizeChanged || !mcmp(param->curValue.Array(), val, dwSize))
    {
        mcpy(param->curValue.Array(), val, dwSize);
        param->bChanged = TRUE;
    }
}

void  D3D10Shader::UpdateParams()
{
    List<BYTE> shaderConstantData;
    bool bUpload = false;

    for(UINT i=0; i<Params.Num(); i++)
    {
        ShaderParam &param = Params[i];

        if(param.type != Parameter_Texture)
        {
            if(!param.curValue.Num())
            {
                AppWarning(TEXT("D3D10Shader::UpdateParams: shader parameter '%s' not set"), param.name.Array());
                bUpload = false;
                break;
            }

            shaderConstantData.AppendList(param.curValue);

            if(param.bChanged)
            {
                bUpload = true;
                param.bChanged = false;
            }
        }
        else
        {
            if(param.curValue.Num())
            {
                Texture *texture = *(Texture**)param.curValue.Array();
                LoadTexture(texture, param.textureID);
            }
        }
    }

    if(shaderConstantData.Num() != constantSize)
    {
        AppWarning(TEXT("D3D10Shader::UpdateParams: invalid parameter specifications, constant size given: %d, constant size expected: %d"), shaderConstantData.Num(), constantSize);
        bUpload = false;
    }

    if(bUpload)
    {
        D3D11_MAPPED_SUBRESOURCE map;

        HRESULT err;
        if(FAILED(err = GetD3DCtx()->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
        {
            AppWarning(TEXT("D3D10Shader::UpdateParams: could not map constant buffer, result = %08lX"), err);
            return;
        }

        mcpy(map.pData, shaderConstantData.Array(), shaderConstantData.Num());
        GetD3DCtx()->Unmap(constantBuffer, 0);
    }
}
