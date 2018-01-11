
#include "common.h"

#pragma warning(disable: 4530)

GraphicsSystem *GS = NULL;

GraphicsSystem::GraphicsSystem()
:   curMatrix(0)
{
    MatrixStack << Matrix().SetIdentity();
}

GraphicsSystem::~GraphicsSystem()
{
}

template class  FutureShader<CreateVertexShaderFromBlob>;
template class  FutureShader<CreatePixelShaderFromBlob>;

FuturePixelShader GraphicsSystem::CreatePixelShaderFromFileAsync(CTSTR fileName)
{
    using namespace std;
    String fullPathFilename;

    if ((fileName[0] != '.' && fileName[0] != '/' && fileName[0] != '\\') && !(fileName[0] && fileName[1] == ':'))
        fullPathFilename << /*API->GetAppPath() <<*/ L"\\" << fileName;
    else
        fullPathFilename << fileName;

    wstring const fn = fullPathFilename.Array();
    auto &cs = futureShaders.contexts;

    ScopedLock m(futureShaders.lock);

    bool initialized = cs.find(fn) != end(cs);

	FutureShaderContext &c = cs[fn];

    if (!initialized)
    {
        c.readyEvent.reset(CreateEvent(nullptr, true, false, nullptr));
        c.fileName = fn;
        c.thread.reset(OSCreateThread(static_cast<XTHREAD>([](void *arg) -> DWORD
        {
			FutureShaderContext &c = *(FutureShaderContext*)arg;
            XFile ShaderFile;

            if (!ShaderFile.Open(c.fileName.c_str(), XFILE_READ | XFILE_SHARED, XFILE_OPENEXISTING))
                return 1;

            String strShader;
            ShaderFile.ReadFileToString(strShader);

            c.fileData = strShader.Array();

            GS->CreatePixelShaderBlob(c.shaderData, strShader.Array(), c.fileName.c_str());

            SetEvent(c.readyEvent.get());
            return 0;
        }), &c));
    }

    if (c.thread && WaitForSingleObject(c.readyEvent.get(), 0) == WAIT_OBJECT_0)
        c.thread.reset();

    return FuturePixelShader(c.readyEvent.get(), c.shaderData, c.fileData, c.fileName);
}


void GraphicsSystem::DrawSprite(Texture *texture, DWORD color, float x, float y, float x2, float y2)
{
	assert(texture);

	DrawSpriteEx(texture, color, x, y, x2, y2, 0.0f, 0.0f, 1.0f, 1.0f);
}

Matrix MatrixRotationZ(float z)
{
	float sinR = sinf(z);
	float cosR = cosf(z);
	Matrix m;
	memset(&m, 0, sizeof(m));
	m.X.x = cosR;
	m.X.y = sinR;
	m.Y.x = -sinR;
	m.Y.y = cosR;
	m.Z.z = 1.0f;
	m.T.w = 1.0f;
	return m;
}

inline void  GraphicsSystem::MatrixRotate(float x, float y, float z, float a)
{
	MatrixStack[curMatrix].X.x = 1.0f;
	MatrixStack[curMatrix].X.y = 0.0f;
	MatrixStack[curMatrix].Y.x = 0.0f;
	MatrixStack[curMatrix].Y.y = 1.0f;
	MatrixStack[curMatrix].Z.z = 1.0f;
// 	MatrixStack[curMatrix] *= Quat(AxisAngle(x, y, z, a));
	MatrixStack[curMatrix] *= MatrixRotationZ(z);
    ResetViewMatrix();
}

inline void  GraphicsSystem::MatrixRotate(const AxisAngle &aa)
{
    MatrixStack[curMatrix] *= Quat(aa);
    ResetViewMatrix();
}

inline void  GraphicsSystem::MatrixRotate(const Quat &q)
{
    MatrixStack[curMatrix] *= q;
    ResetViewMatrix();
}

inline void  GraphicsSystem::MatrixTranslate(float x, float y)
{
    MatrixStack[curMatrix] *= Vect(x, y, 0.0f);
    ResetViewMatrix();
}

inline void  GraphicsSystem::MatrixTranslate(const Vect2 &pos)
{
    MatrixStack[curMatrix] *= Vect(pos);
    ResetViewMatrix();
}

inline void  GraphicsSystem::MatrixScale(const Vect2 &scale)
{
    MatrixStack[curMatrix].Scale(scale.x, scale.y, 1.0f);
    ResetViewMatrix();
}

inline void  GraphicsSystem::MatrixScale(float x, float y)
{
	MatrixStack[curMatrix].X.x = 1.0f;
	MatrixStack[curMatrix].Y.y = 1.0f;
    MatrixStack[curMatrix].Scale(x, y, 1.0f);
    ResetViewMatrix();
}

inline void  GraphicsSystem::MatrixTranspose()
{
    MatrixStack[curMatrix].Transpose();
    ResetViewMatrix();
}

